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
  void SpawnOrb(Vec3 pos, float radius, float mass);

  int RayPick(Vec3 rayOrigin, Vec3 rayDir) const;

  static void MassToColor(float mass, float minMass, float maxMass, float &r,
                          float &g, float &b);

  std::vector<VerletBody> m_bodies;
  std::vector<Constraint> m_constraints;

  Vec3 m_gravity = Vec3(0.0f, -9.8f, 0.0f);
  bool m_gravityEnabled = true;

  ContainerType m_containerType = ContainerType::Sphere;
  float m_containerRadius = 10.0f;
  float m_containerHalfSize = 10.0f;
  Vec3 m_containerCenter = Vec3(0.0f, 0.0f, 0.0f);

  bool m_isPaused = false;
  enum class BondType { Tether, Rod };
  BondType m_bondType = BondType::Tether;

  int m_grabbedOrb = -1;
  Vec3 m_grabTarget;

  float m_minMass = 1.0f;
  float m_maxMass = 1.0f;

private:
  SpatialGrid m_grid;
  float m_maxRadius = 0.3f;
  std::vector<uint32_t> m_neighborBuf;

  // Fixed timestep accumulator
  float m_accumulator = 0.0f;
  static constexpr float FIXED_DT = 1.0f / 120.0f;

  void StepFixed(float fixedDt);
  void SolveCollisionsSpatial();
  void ApplyContainerBounds(VerletBody &body);
  void UpdateGridCellSize();
};
