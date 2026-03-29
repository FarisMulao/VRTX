#include "Renderer.h"
#include "../physics/VerletBody.h"
#include "../physics/Constraint.h"
#include <d3dcompiler.h>
#include <string>
#include <cmath>

#pragma comment(lib, "d3dcompiler.lib")

static std::wstring ToWide(const std::string& s) {
    return std::wstring(s.begin(), s.end());
}

static HRESULT CompileShader(const char* filename, const char* entry, const char* target,
    ID3DBlob** blob, ID3DBlob** errorBlob) 
{
    std::wstring absPath = ToWide(std::string(VRTX_SOURCE_DIR) + "/src/shaders/" + filename);
    std::wstring relPath = ToWide(std::string(filename));
    
    HRESULT hr = D3DCompileFromFile(absPath.c_str(), nullptr, nullptr, entry, target, D3DCOMPILE_DEBUG, 0, blob, errorBlob);
    if (FAILED(hr)) hr = D3DCompileFromFile(relPath.c_str(), nullptr, nullptr, entry, target, D3DCOMPILE_DEBUG, 0, blob, errorBlob);
    return hr;
}

Renderer::Renderer(std::shared_ptr<D3D12Context> context) 
    : m_context(context), m_indexCount(0), m_maxInstances(10000), m_currentInstanceCount(0),
      m_containerPointCount(0), m_bondVertexCount(0), m_maxBondVertices(20000)
{}

Renderer::~Renderer() {}

void Renderer::Init() {
    BuildRootSignature();
    BuildPSO();
    BuildGeometry();
    BuildInstanceBuffer();
    RebuildContainerSphere(10.0f);
    BuildContainerPSO();
    BuildBondPSO();
    BuildBondBuffer();
}

void Renderer::BuildRootSignature() {
    D3D12_ROOT_PARAMETER rootParameters[2] = {};
    
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParameters[0].Constants.ShaderRegister = 0;
    rootParameters[0].Constants.RegisterSpace = 0;
    rootParameters[0].Constants.Num32BitValues = 16;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    rootParameters[1].Descriptor.ShaderRegister = 0;
    rootParameters[1].Descriptor.RegisterSpace = 0;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 2;
    rootSigDesc.pParameters = rootParameters;
    rootSigDesc.NumStaticSamplers = 0;
    rootSigDesc.pStaticSamplers = nullptr;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    Microsoft::WRL::ComPtr<ID3DBlob> signature;
    Microsoft::WRL::ComPtr<ID3DBlob> error;
    D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    m_context->GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
}

void Renderer::BuildPSO() {
    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;
    
    HRESULT hr = CompileShader("InstancedRenderer.hlsl", "VSMain", "vs_5_0", &vsBlob, &errorBlob);
    if (FAILED(hr)) {
        OutputDebugStringA("[VRTX] FATAL: Failed to compile InstancedRenderer.hlsl (VSMain)\n");
        if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        return;
    }
    
    errorBlob.Reset();
    hr = CompileShader("InstancedRenderer.hlsl", "PSMain", "ps_5_0", &psBlob, &errorBlob);
    if (FAILED(hr)) {
        OutputDebugStringA("[VRTX] FATAL: Failed to compile InstancedRenderer.hlsl (PSMain)\n");
        if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        return;
    }
    
    OutputDebugStringA("[VRTX] Shader InstancedRenderer.hlsl compiled successfully!\n");
    
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, 2 };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
    
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
    psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;

    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;

    m_context->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
}

void Renderer::BuildContainerPSO() {
    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;
    
    HRESULT hr = CompileShader("ContainerShader.hlsl", "VSMain", "vs_5_0", &vsBlob, &errorBlob);
    if (FAILED(hr)) {
        OutputDebugStringA("[VRTX] WARN: Failed to compile ContainerShader.hlsl (VSMain)\n");
        if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        return;
    }
    errorBlob.Reset();
    hr = CompileShader("ContainerShader.hlsl", "PSMain", "ps_5_0", &psBlob, &errorBlob);
    if (FAILED(hr)) {
        OutputDebugStringA("[VRTX] WARN: Failed to compile ContainerShader.hlsl (PSMain)\n");
        if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        return;
    }
    
    OutputDebugStringA("[VRTX] Shader ContainerShader.hlsl compiled successfully!\n");
    
    D3D12_INPUT_ELEMENT_DESC inputDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputDesc, 1 };
    psoDesc.pRootSignature = m_rootSignature.Get(); // Reuse — same cbuffer layout
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
    
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;
    
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;
    
    m_context->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_containerPSO));
}

static void UploadContainerPoints(ID3D12Device* device, std::vector<Vec3>& points,
    Microsoft::WRL::ComPtr<ID3D12Resource>& vb, D3D12_VERTEX_BUFFER_VIEW& view, uint32_t& count)
{
    count = (uint32_t)points.size();
    size_t bufSize = points.size() * sizeof(Vec3);
    
    auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto bufDesc = CD3DX12_RESOURCE_DESC::Buffer(bufSize);
    vb.Reset();
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vb));
    
    void* mapped;
    vb->Map(0, nullptr, &mapped);
    memcpy(mapped, points.data(), bufSize);
    vb->Unmap(0, nullptr);
    
    view.BufferLocation = vb->GetGPUVirtualAddress();
    view.StrideInBytes = sizeof(Vec3);
    view.SizeInBytes = (UINT)bufSize;
}

void Renderer::RebuildContainerSphere(float radius, int latDiv, int lonDiv) {
    const float PI = 3.14159265359f;
    std::vector<Vec3> points;
    
    for (int i = 0; i <= latDiv; ++i) {
        float theta = i * PI / latDiv;
        float sinT = std::sin(theta);
        float cosT = std::cos(theta);
        for (int j = 0; j < lonDiv; ++j) {
            float phi = j * 2.0f * PI / lonDiv;
            float x = radius * sinT * std::cos(phi);
            float y = radius * cosT;
            float z = radius * sinT * std::sin(phi);
            points.push_back(Vec3(x, y, z));
        }
    }
    
    // Wait for GPU to finish rendering before we swap the vertex buffer
    m_context->Flush();
    UploadContainerPoints(m_context->GetDevice(), points, m_containerVB, m_containerVBView, m_containerPointCount);
    
    char msg[128];
    sprintf_s(msg, "[VRTX] Container sphere: %u points (r=%.1f)\n", m_containerPointCount, radius);
    OutputDebugStringA(msg);
}

void Renderer::RebuildContainerCube(float halfSize, int subdivisions) {
    std::vector<Vec3> points;
    float step = (2.0f * halfSize) / subdivisions;
    
    for (int i = 0; i <= subdivisions; ++i) {
        for (int j = 0; j <= subdivisions; ++j) {
            float u = -halfSize + i * step;
            float v = -halfSize + j * step;
            
            // +X and -X faces
            points.push_back(Vec3( halfSize, u, v));
            points.push_back(Vec3(-halfSize, u, v));
            // +Y and -Y faces
            points.push_back(Vec3(u,  halfSize, v));
            points.push_back(Vec3(u, -halfSize, v));
            // +Z and -Z faces
            points.push_back(Vec3(u, v,  halfSize));
            points.push_back(Vec3(u, v, -halfSize));
        }
    }
    
    // Wait for GPU to finish rendering before we swap the vertex buffer
    m_context->Flush();
    UploadContainerPoints(m_context->GetDevice(), points, m_containerVB, m_containerVBView, m_containerPointCount);
    
    char msg[128];
    sprintf_s(msg, "[VRTX] Container cube: %u points (hs=%.1f)\n", m_containerPointCount, halfSize);
    OutputDebugStringA(msg);
}

// === BONDS ===

void Renderer::BuildBondPSO() {
    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;
    
    HRESULT hr = CompileShader("ContainerShader.hlsl", "VSMain", "vs_5_0", &vsBlob, &errorBlob);
    if (FAILED(hr)) return;
    errorBlob.Reset();
    hr = CompileShader("ContainerShader.hlsl", "PSMain", "ps_5_0", &psBlob, &errorBlob);
    if (FAILED(hr)) return;
    
    D3D12_INPUT_ELEMENT_DESC inputDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputDesc, 1 };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
    
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;
    
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;
    
    m_context->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_bondPSO));
}

void Renderer::BuildBondBuffer() {
    auto device = m_context->GetDevice();
    size_t bufSize = m_maxBondVertices * sizeof(Vec3);
    auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto bufDesc = CD3DX12_RESOURCE_DESC::Buffer(bufSize);
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_bondVB));
    
    m_bondVBView.BufferLocation = m_bondVB->GetGPUVirtualAddress();
    m_bondVBView.StrideInBytes = sizeof(Vec3);
    m_bondVBView.SizeInBytes = (UINT)bufSize;
}

void Renderer::UpdateBonds(const std::vector<Constraint>& constraints, const std::vector<VerletBody>& bodies) {
    if (constraints.empty() || bodies.empty()) {
        m_bondVertexCount = 0;
        return;
    }
    
    std::vector<Vec3> lineVerts;
    lineVerts.reserve(constraints.size() * 2);
    
    for (auto& c : constraints) {
        if (!c.active) continue;
        if (c.bodyA >= bodies.size() || c.bodyB >= bodies.size()) continue;
        lineVerts.push_back(bodies[c.bodyA].position);
        lineVerts.push_back(bodies[c.bodyB].position);
    }
    
    m_bondVertexCount = (uint32_t)lineVerts.size();
    if (m_bondVertexCount == 0) return;
    if (m_bondVertexCount > m_maxBondVertices) m_bondVertexCount = m_maxBondVertices;
    
    void* mapped;
    m_bondVB->Map(0, nullptr, &mapped);
    memcpy(mapped, lineVerts.data(), m_bondVertexCount * sizeof(Vec3));
    m_bondVB->Unmap(0, nullptr);
}

// === GEOMETRY ===

void Renderer::BuildGeometry() {
    Mesh sphere = Mesh::CreateSphere(16, 16);
    m_indexCount = (uint32_t)sphere.m_indices.size();

    auto device = m_context->GetDevice();
    
    size_t vbSize = sphere.m_vertices.size() * sizeof(Vertex);
    size_t ibSize = sphere.m_indices.size() * sizeof(uint32_t);
    
    auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto vbDesc = CD3DX12_RESOURCE_DESC::Buffer(vbSize);
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &vbDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_vertexBuffer));
    
    void* pVertexDataBegin;
    m_vertexBuffer->Map(0, nullptr, &pVertexDataBegin);
    memcpy(pVertexDataBegin, sphere.m_vertices.data(), vbSize);
    m_vertexBuffer->Unmap(0, nullptr);

    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.StrideInBytes = sizeof(Vertex);
    m_vertexBufferView.SizeInBytes = (UINT)vbSize;

    auto ibDesc = CD3DX12_RESOURCE_DESC::Buffer(ibSize);
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &ibDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_indexBuffer));

    void* pIndexDataBegin;
    m_indexBuffer->Map(0, nullptr, &pIndexDataBegin);
    memcpy(pIndexDataBegin, sphere.m_indices.data(), ibSize);
    m_indexBuffer->Unmap(0, nullptr);

    m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    m_indexBufferView.SizeInBytes = (UINT)ibSize;
}

void Renderer::BuildInstanceBuffer() {
    auto device = m_context->GetDevice();
    size_t instanceBufferSize = m_maxInstances * sizeof(VerletBody);
    
    auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(instanceBufferSize);
    
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_instanceBuffer));
}

void Renderer::UpdateInstances(const std::vector<VerletBody>& bodies) {
    if (bodies.empty()) {
        m_currentInstanceCount = 0;
        return;
    }
    
    m_currentInstanceCount = (uint32_t)(bodies.size() < m_maxInstances ? bodies.size() : m_maxInstances);
    
    void* mappedData;
    m_instanceBuffer->Map(0, nullptr, &mappedData);
    memcpy(mappedData, bodies.data(), m_currentInstanceCount * sizeof(VerletBody));
    m_instanceBuffer->Unmap(0, nullptr);
}

// === RENDER ===

void Renderer::Render(const Camera& camera) {
    auto cmdList = m_context->GetCommandList();
    
    Mat4 viewProj = camera.GetViewMatrix() * camera.GetProjectionMatrix();
    Mat4 viewProjT = viewProj.Transpose();
    
    if (m_containerPSO && m_containerPointCount > 0) {
        cmdList->SetPipelineState(m_containerPSO.Get());
        cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
        cmdList->IASetVertexBuffers(0, 1, &m_containerVBView);
        cmdList->SetGraphicsRoot32BitConstants(0, 16, viewProjT.m, 0);
        cmdList->DrawInstanced(m_containerPointCount, 1, 0, 0);
    }
    
    if (m_bondPSO && m_bondVertexCount > 0) {
        cmdList->SetPipelineState(m_bondPSO.Get());
        cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
        cmdList->IASetVertexBuffers(0, 1, &m_bondVBView);
        cmdList->SetGraphicsRoot32BitConstants(0, 16, viewProjT.m, 0);
        cmdList->DrawInstanced(m_bondVertexCount, 1, 0, 0);
    }
    
    if (m_pipelineState && m_currentInstanceCount > 0) {
        cmdList->SetPipelineState(m_pipelineState.Get());
        cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        cmdList->IASetIndexBuffer(&m_indexBufferView);
        cmdList->SetGraphicsRoot32BitConstants(0, 16, viewProjT.m, 0);
        cmdList->SetGraphicsRootShaderResourceView(1, m_instanceBuffer->GetGPUVirtualAddress());
        cmdList->DrawIndexedInstanced(m_indexCount, m_currentInstanceCount, 0, 0, 0);
    }
}
