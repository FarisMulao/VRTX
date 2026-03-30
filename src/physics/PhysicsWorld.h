#pragma once
#include "Constraint.h"
#include "SpatialGrid.h"
#include "VerletBody.h"
#include <vector>

enum class ContainerType { Sphere, Cube };

class PhysicsWorld {
public:
  PhysicsWorld();
  ~PhysicsWorld();

  void Update(float dt);
  void SpawnOrb(Vec3 pos, float radius = 0.3f);

  // Ray picking: returns index of nearest hit orb, or -1
  int RayPick(Vec3 rayOrigin, Vec3 rayDir) const;

  std::vector<VerletBody> m_bodies;
  std::vector<Constraint> m_constraints;

  Vec3 m_gravity = Vec3(0.0f, -9.8f, 0.0f);

  // Container settings
  ContainerType m_containerType = ContainerType::Sphere;
  float m_containerRadius = 10.0f;   // For sphere
  float m_containerHalfSize = 10.0f; // For cube (half-extent)
  Vec3 m_containerCenter = Vec3(0.0f, 0.0f, 0.0f);

  // Pause state and bonds
  bool m_isPaused = false;
  enum class BondType { Tether, Rod };
  BondType m_bondType = BondType::Tether;

  // Grabbed orb
  int m_grabbedOrb = -1;
  Vec3 m_grabTarget;

private:
  SpatialGrid m_grid;
  std::vector<uint32_t> m_neighborBuf; // Reusable buffer to avoid allocs

  void SolveCollisionsSpatial();
  void ApplyContainerBounds(VerletBody &body);
};
