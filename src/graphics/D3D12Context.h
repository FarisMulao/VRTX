#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <vector>

using Microsoft::WRL::ComPtr;

class D3D12Context {
public:
    D3D12Context(HWND hwnd, int width, int height);
    ~D3D12Context();

    void Flush();
    void Resize(int width, int height);

    ID3D12Device* GetDevice() const { return m_device.Get(); }
    ID3D12GraphicsCommandList* GetCommandList() const { return m_commandList.Get(); }
    ID3D12CommandQueue* GetCommandQueue() const { return m_commandQueue.Get(); }
    ID3D12CommandAllocator* GetCommandAllocator() const { return m_commandAllocator[m_frameIndex].Get(); }
    
    void ResetCommandList();
    void ExecuteCommandList();
    void Present();

    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;
    ID3D12Resource* GetCurrentBackBuffer() const { return m_swapChainBuffers[m_frameIndex].Get(); }

    ID3D12DescriptorHeap* GetSrvHeap() const { return m_srvHeap.Get(); }
    UINT GetCbvSrvUavDescriptorSize() const { return m_cbvSrvUavDescriptorSize; }

private:
    void InitD3D();
    void CreateCommandObjects();
    void CreateSwapChain();
    void CreateRtvAndDsvDescriptorHeaps();
    void MoveToNextFrame();

    HWND m_hwnd;
    int m_width;
    int m_height;

    static const int FrameCount = 3; 
    
    ComPtr<IDXGIFactory4> m_dxgiFactory;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValues[FrameCount];
    HANDLE m_fenceEvent;
    
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12CommandAllocator> m_commandAllocator[FrameCount];
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    ComPtr<IDXGISwapChain3> m_swapChain;

    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
    ComPtr<ID3D12DescriptorHeap> m_srvHeap;

    ComPtr<ID3D12Resource> m_swapChainBuffers[FrameCount];
    ComPtr<ID3D12Resource> m_depthStencilBuffer;

    UINT m_rtvDescriptorSize;
    UINT m_dsvDescriptorSize;
    UINT m_cbvSrvUavDescriptorSize;

    int m_frameIndex;
};
