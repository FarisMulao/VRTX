#pragma once
#include <d3d12.h>

struct CD3DX12_HEAP_PROPERTIES : public D3D12_HEAP_PROPERTIES {
  CD3DX12_HEAP_PROPERTIES() = default;
  explicit CD3DX12_HEAP_PROPERTIES(const D3D12_HEAP_PROPERTIES &o) noexcept
      : D3D12_HEAP_PROPERTIES(o) {}
  CD3DX12_HEAP_PROPERTIES(
      D3D12_HEAP_TYPE type,
      UINT cpuPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
      UINT memoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
      UINT creationNodeMask = 1, UINT visibleNodeMask = 1) noexcept {
    Type = type;
    CPUPageProperty = (D3D12_CPU_PAGE_PROPERTY)cpuPageProperty;
    MemoryPoolPreference = (D3D12_MEMORY_POOL)memoryPoolPreference;
    CreationNodeMask = creationNodeMask;
    VisibleNodeMask = visibleNodeMask;
  }
};

struct CD3DX12_RESOURCE_DESC : public D3D12_RESOURCE_DESC {
  CD3DX12_RESOURCE_DESC() = default;
  explicit CD3DX12_RESOURCE_DESC(const D3D12_RESOURCE_DESC &o) noexcept
      : D3D12_RESOURCE_DESC(o) {}
  static inline CD3DX12_RESOURCE_DESC
  Buffer(UINT64 width, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
         UINT64 alignment = 0) noexcept {
    CD3DX12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = alignment;
    desc.Width = width;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = flags;
    return desc;
  }
};
