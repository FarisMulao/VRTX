#pragma once
#include "../math/Vec3.h"

struct VerletBody {
  Vec3 position;
  Vec3 previousPosition;
  float radius;
  uint32_t isPinned; // Using uint32_t for HLSL compatibility
};
