#pragma once
#include "../math/Vec3.h"
#include <d3d12.h>
#include <vector>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

struct Vertex {
  Vec3 position;
  Vec3 normal;
};

class Mesh {
public:
  Mesh() = default;

  // Procedural generation helpers
  static Mesh CreateSphere(int latitude, int longitude);
  static Mesh CreateCylinder(int subdivisions);

  std::vector<Vertex> m_vertices;
  std::vector<uint32_t> m_indices;
};
