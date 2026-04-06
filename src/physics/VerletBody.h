#pragma once
#include "../math/Vec3.h"
#include <cstdint>

struct VerletBody {
  Vec3 position;
  Vec3 previousPosition;
  float radius;
  uint32_t isPinned; // Using uint32_t for HLSL compatibility
  float mass;
  float colorR, colorG, colorB;
  uint32_t highlighted; // 1 = source highlight, 2 = hover highlight
};
