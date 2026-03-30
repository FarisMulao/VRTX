#pragma once
#include "../math/Vec3.h"
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>

class SpatialGrid {
public:
  SpatialGrid(float cellSize = 1.0f)
      : m_cellSize(cellSize), m_invCellSize(1.0f / cellSize) {}

  void Clear() { m_cells.clear(); }

  void Insert(uint32_t index, Vec3 pos) {
    int64_t key = HashPos(pos);
    m_cells[key].push_back(index);
  }

  void GetNeighbors(Vec3 pos, std::vector<uint32_t> &out) const {
    int cx = PosToCell(pos.x_val);
    int cy = PosToCell(pos.y_val);
    int cz = PosToCell(pos.z_val);

    for (int dx = -1; dx <= 1; ++dx) {
      for (int dy = -1; dy <= 1; ++dy) {
        for (int dz = -1; dz <= 1; ++dz) {
          int64_t key = CellHash(cx + dx, cy + dy, cz + dz);
          auto it = m_cells.find(key);
          if (it != m_cells.end()) {
            for (uint32_t idx : it->second) {
              out.push_back(idx);
            }
          }
        }
      }
    }
  }

private:
  float m_cellSize;
  float m_invCellSize;
  std::unordered_map<int64_t, std::vector<uint32_t>> m_cells;

  int PosToCell(float v) const { return (int)std::floor(v * m_invCellSize); }

  int64_t CellHash(int x, int y, int z) const {
    return ((int64_t)x * 73856093LL) ^ ((int64_t)y * 19349663LL) ^
           ((int64_t)z * 83492791LL);
  }

  int64_t HashPos(Vec3 pos) const {
    return CellHash(PosToCell(pos.x_val), PosToCell(pos.y_val),
                    PosToCell(pos.z_val));
  }
};
