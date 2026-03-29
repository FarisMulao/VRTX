#pragma once
#include <cstdint>

struct Constraint {
    uint32_t bodyA, bodyB;
    float restLength;
    uint32_t active;
};
