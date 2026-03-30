#include "Mesh.h"
#include <cmath>

Mesh Mesh::CreateSphere(int latitude, int longitude) {
  Mesh mesh;
  const float PI = 3.14159265359f;

  for (int i = 0; i <= latitude; ++i) {
    float theta = i * PI / latitude;
    float sinTheta = std::sin(theta);
    float cosTheta = std::cos(theta);

    for (int j = 0; j <= longitude; ++j) {
      float phi = j * 2 * PI / longitude;
      float sinPhi = std::sin(phi);
      float cosPhi = std::cos(phi);

      float x = cosPhi * sinTheta;
      float y = cosTheta;
      float z = sinPhi * sinTheta;

      mesh.m_vertices.push_back({{x, y, z}, {x, y, z}});
    }
  }

  for (int i = 0; i < latitude; ++i) {
    for (int j = 0; j < longitude; ++j) {
      uint32_t first = (i * (longitude + 1)) + j;
      uint32_t second = first + longitude + 1;

      mesh.m_indices.push_back(first);
      mesh.m_indices.push_back(second);
      mesh.m_indices.push_back(first + 1);

      mesh.m_indices.push_back(second);
      mesh.m_indices.push_back(second + 1);
      mesh.m_indices.push_back(first + 1);
    }
  }

  return mesh;
}

Mesh Mesh::CreateCylinder(int subdivisions) {
  // Placeholder - implement later
  return Mesh();
}
