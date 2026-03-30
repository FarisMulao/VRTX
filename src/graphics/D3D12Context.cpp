#include "D3D12Context.h"
#include "d3dx12_minimal.h"
#include <algorithm>
#include <stdexcept>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

D3D12Context::D3D12Context(HWND hwnd, int width, int height)
    : m_hwnd(hwnd), m_width(width), m_height(height), m_frameIndex(0) {
  InitD3D();
  CreateCommandObjects();
  CreateSwapChain();
  CreateRtvAndDsvDescriptorHeaps();
}

D3D12Context::~D3D12Context() {
  Flush();
  CloseHandle(m_fenceEvent);
}

void D3D12Context::InitD3D() {
  UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
  ComPtr<ID3D12Debug> debugController;
  if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
    debugController->EnableDebugLayer();
    dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
  }
#endif

  CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_dxgiFactory));

  ComPtr<IDXGIAdapter1> adapter;
  for (UINT i = 0;
       DXGI_ERROR_NOT_FOUND != m_dxgiFactory->EnumAdapters1(i, &adapter); ++i) {
    DXGI_ADAPTER_DESC1 desc;
    adapter->GetDesc1(&desc);
    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
      continue;
    if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0,
                                    IID_PPV_ARGS(&m_device))))
      break;
  }

  m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
  for (int i = 0; i < FrameCount; ++i)
    m_fenceValues[i] = 0;
  m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

  m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
  m_cbvSrvUavDescriptorSize = m_device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void D3D12Context::CreateCommandObjects() {
  D3D12_COMMAND_QUEUE_DESC queueDesc = {};
  queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));

  for (int i = 0; i < FrameCount; ++i) {
    m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                     IID_PPV_ARGS(&m_commandAllocator[i]));
  }

  m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                              m_commandAllocator[0].Get(), nullptr,
                              IID_PPV_ARGS(&m_commandList));
  m_commandList->Close();
}

void D3D12Context::CreateSwapChain() {
  m_swapChain.Reset();

  DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
  swapChainDesc.BufferCount = FrameCount;
  swapChainDesc.Width = m_width;
  swapChainDesc.Height = m_height;
  swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swapChainDesc.SampleDesc.Count = 1;

  ComPtr<IDXGISwapChain1> swapChain;
  m_dxgiFactory->CreateSwapChainForHwnd(m_commandQueue.Get(), m_hwnd,
                                        &swapChainDesc, nullptr, nullptr,
                                        &swapChain);
  swapChain.As(&m_swapChain);
  m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void D3D12Context::CreateRtvAndDsvDescriptorHeaps() {
  D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
  rtvHeapDesc.NumDescriptors = FrameCount;
  rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));

  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
      m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
  for (int i = 0; i < FrameCount; ++i) {
    m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_swapChainBuffers[i]));
    m_device->CreateRenderTargetView(m_swapChainBuffers[i].Get(), nullptr,
                                     rtvHandle);
    rtvHandle.ptr += m_rtvDescriptorSize;
  }

  D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
  dsvHeapDesc.NumDescriptors = 1;
  dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
  m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap));

  D3D12_RESOURCE_DESC depthDesc = {};
  depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  depthDesc.Width = m_width;
  depthDesc.Height = m_height;
  depthDesc.DepthOrArraySize = 1;
  depthDesc.MipLevels = 1;
  depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
  depthDesc.SampleDesc.Count = 1;
  depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

  D3D12_CLEAR_VALUE optClear = {};
  optClear.Format = DXGI_FORMAT_D32_FLOAT;
  optClear.DepthStencil.Depth = 1.0f;

  auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
                                    &depthDesc,
                                    D3D12_RESOURCE_STATE_DEPTH_WRITE, &optClear,
                                    IID_PPV_ARGS(&m_depthStencilBuffer));
  m_device->CreateDepthStencilView(
      m_depthStencilBuffer.Get(), nullptr,
      m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

  D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
  srvHeapDesc.NumDescriptors = 128;
  srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap));
}

void D3D12Context::Flush() {
  m_commandQueue->Signal(m_fence.Get(), ++m_fenceValues[m_frameIndex]);
  m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent);
  WaitForSingleObject(m_fenceEvent, INFINITE);
}

void D3D12Context::ResetCommandList() {
  m_commandAllocator[m_frameIndex]->Reset();
  m_commandList->Reset(m_commandAllocator[m_frameIndex].Get(), nullptr);
}

void D3D12Context::ExecuteCommandList() {
  m_commandList->Close();
  ID3D12CommandList *lists[] = {m_commandList.Get()};
  m_commandQueue->ExecuteCommandLists(1, lists);
}

void D3D12Context::Present() {
  m_swapChain->Present(1, 0);
  MoveToNextFrame();
}

void D3D12Context::MoveToNextFrame() {
  const UINT64 currentFenceValue = m_fenceValues[m_frameIndex];
  m_commandQueue->Signal(m_fence.Get(), currentFenceValue);

  m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

  if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex]) {
    m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent);
    WaitForSingleObject(m_fenceEvent, INFINITE);
  }

  m_fenceValues[m_frameIndex] = currentFenceValue + 1;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12Context::GetCurrentBackBufferView() const {
  auto handle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
  handle.ptr += m_frameIndex * m_rtvDescriptorSize;
  return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12Context::GetDepthStencilView() const {
  return m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void D3D12Context::Resize(int width, int height) {
  if (width == m_width && height == m_height)
    return;
  Flush();
  m_width = width;
  m_height = height;
  for (int i = 0; i < FrameCount; ++i)
    m_swapChainBuffers[i].Reset();
  m_swapChain->ResizeBuffers(FrameCount, width, height,
                             DXGI_FORMAT_R8G8B8A8_UNORM, 0);
  m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
  CreateRtvAndDsvDescriptorHeaps();
}
