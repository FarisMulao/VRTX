#pragma once
#include "D3D12Context.h"
#include "Camera.h"
#include "Mesh.h"
#include "d3dx12_minimal.h"
#include <wrl/client.h>
#include <memory>
#include <vector>

class Renderer {
public:
    Renderer(std::shared_ptr<D3D12Context> context);
    ~Renderer();

    void Init();
    void UpdateInstances(const std::vector<struct VerletBody>& bodies);
    void UpdateBonds(const std::vector<struct Constraint>& constraints, const std::vector<struct VerletBody>& bodies);
    void Render(const Camera& camera);

private:
    std::shared_ptr<D3D12Context> m_context;
    
    // --- ORB PIPELINE ---
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
    
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
    uint32_t m_indexCount;
    
    Microsoft::WRL::ComPtr<ID3D12Resource> m_instanceBuffer;
    uint32_t m_maxInstances;
    uint32_t m_currentInstanceCount;
    
    // --- CONTAINER PIPELINE ---
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_containerPSO;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_containerVB;
    D3D12_VERTEX_BUFFER_VIEW m_containerVBView;
    uint32_t m_containerPointCount;
    
    // --- BOND PIPELINE ---
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_bondPSO;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_bondVB;
    D3D12_VERTEX_BUFFER_VIEW m_bondVBView;
    uint32_t m_bondVertexCount;
    uint32_t m_maxBondVertices;
    
    void BuildRootSignature();
    void BuildPSO();
    void BuildGeometry();
    void BuildInstanceBuffer();
    void BuildContainerPSO();
    void BuildBondPSO();
    void BuildBondBuffer();

public:
    void RebuildContainerSphere(float radius, int latDiv = 30, int lonDiv = 40);
    void RebuildContainerCube(float halfSize, int subdivisions = 15);
};
