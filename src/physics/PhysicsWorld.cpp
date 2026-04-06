#include "PhysicsWorld.h"
#include <algorithm>
#include <cmath>

PhysicsWorld::PhysicsWorld() : m_grid(0.8f) {}
PhysicsWorld::~PhysicsWorld() {}

void PhysicsWorld::MassToColor(float mass, float minMass, float maxMass,
                               float &r, float &g, float &b) {
  float t = 0.5f;
  if (maxMass > minMass) {
    float logMin = std::log(minMass + 0.01f);
    float logMax = std::log(maxMass + 0.01f);
    float logMass = std::log(mass + 0.01f);
    if (logMax > logMin) {
      t = (logMass - logMin) / (logMax - logMin);
    }
  }
  t = std::max(0.0f, std::min(1.0f, t));

  // Cool-to-warm gradient: deep blue → cyan → green → yellow → orange → red
  if (t < 0.2f) {
    float s = t / 0.2f;
    r = 0.05f;
    g = 0.2f + 0.5f * s;
    b = 0.95f - 0.15f * s;
  } else if (t < 0.4f) {
    float s = (t - 0.2f) / 0.2f;
    r = 0.05f + 0.25f * s;
    g = 0.7f + 0.2f * s;
    b = 0.8f - 0.6f * s;
  } else if (t < 0.6f) {
    float s = (t - 0.4f) / 0.2f;
    r = 0.3f + 0.5f * s;
    g = 0.9f;
    b = 0.2f - 0.15f * s;
  } else if (t < 0.8f) {
    float s = (t - 0.6f) / 0.2f;
    r = 0.8f + 0.15f * s;
    g = 0.9f - 0.5f * s;
    b = 0.05f;
  } else {
    float s = (t - 0.8f) / 0.2f;
    r = 0.95f + 0.05f * s;
    g = 0.4f - 0.25f * s;
    b = 0.05f;
  }
}

void PhysicsWorld::SpawnOrb(Vec3 pos, float radius, float mass) {
  VerletBody body = {};
  body.position = pos;
  body.previousPosition = pos;
  body.radius = radius;
  body.mass = mass;
  body.isPinned = 0;
  body.highlighted = 0;

  if (m_bodies.empty()) {
    m_minMass = mass;
    m_maxMass = mass;
  } else {
    m_minMass = std::min(m_minMass, mass);
    m_maxMass = std::max(m_maxMass, mass);
  }

  MassToColor(mass, m_minMass, m_maxMass, body.colorR, body.colorG,
              body.colorB);
  m_bodies.push_back(body);

  if (radius > m_maxRadius) {
    m_maxRadius = radius;
  }

  // Only recolor everything when the range actually expanded
  if (m_bodies.size() > 1 && (mass <= m_minMass || mass >= m_maxMass)) {
    for (auto &b : m_bodies) {
      MassToColor(b.mass, m_minMass, m_maxMass, b.colorR, b.colorG, b.colorB);
    }
  }
}

int PhysicsWorld::RayPick(Vec3 rayOrigin, Vec3 rayDir) const {
  float bestT = 1e30f;
  int bestIdx = -1;

  for (size_t i = 0; i < m_bodies.size(); ++i) {
    Vec3 oc = rayOrigin - m_bodies[i].position;
    float a = Vec3::dot(rayDir, rayDir);
    float halfB = Vec3::dot(oc, rayDir);
    float c = Vec3::dot(oc, oc) - m_bodies[i].radius * m_bodies[i].radius;
    float disc = halfB * halfB - a * c;

    if (disc >= 0.0f) {
      float t = (-halfB - std::sqrt(disc)) / a;
      if (t > 0.0f && t < bestT) {
        bestT = t;
        bestIdx = (int)i;
      }
    }
  }
  return bestIdx;
}

void PhysicsWorld::UpdateGridCellSize() {
  float idealCell = m_maxRadius * 4.0f;
  if (idealCell < 0.4f)
    idealCell = 0.4f;
  if (idealCell > 4.0f)
    idealCell = 4.0f;
  m_grid = SpatialGrid(idealCell);
}

void PhysicsWorld::ApplyContainerBounds(VerletBody &body) {
  if (m_containerType == ContainerType::Sphere) {
    Vec3 toBody = body.position - m_containerCenter;
    float distSq = toBody.lengthSq();
    float maxDist = m_containerRadius - body.radius;
    if (maxDist < 0.0f)
      maxDist = 0.0f;

    if (distSq > maxDist * maxDist) {
      float dist = std::sqrt(distSq);
      if (dist > 0.0001f) {
        body.position = m_containerCenter + toBody * (maxDist / dist);
      }
    }
  } else {
    float hs = m_containerHalfSize - body.radius;
    if (hs < 0.0f)
      hs = 0.0f;
    Vec3 &p = body.position;
    Vec3 &c = m_containerCenter;

    if (p.x_val < c.x_val - hs)
      p.x_val = c.x_val - hs;
    if (p.x_val > c.x_val + hs)
      p.x_val = c.x_val + hs;
    if (p.y_val < c.y_val - hs)
      p.y_val = c.y_val - hs;
    if (p.y_val > c.y_val + hs)
      p.y_val = c.y_val + hs;
    if (p.z_val < c.z_val - hs)
      p.z_val = c.z_val - hs;
    if (p.z_val > c.z_val + hs)
      p.z_val = c.z_val + hs;
  }
}

void PhysicsWorld::SolveCollisionsSpatial() {
  m_grid.Clear();
  for (uint32_t i = 0; i < (uint32_t)m_bodies.size(); ++i) {
    m_grid.Insert(i, m_bodies[i].position);
  }

  for (uint32_t i = 0; i < (uint32_t)m_bodies.size(); ++i) {
    m_neighborBuf.clear();
    m_grid.GetNeighbors(m_bodies[i].position, m_neighborBuf);

    for (uint32_t j : m_neighborBuf) {
      if (j <= i)
        continue;

      VerletBody &a = m_bodies[i];
      VerletBody &b = m_bodies[j];

      Vec3 delta = b.position - a.position;
      float distSq = delta.lengthSq();
      float minDist = a.radius + b.radius;
      float minDistSq = minDist * minDist;

      if (distSq < minDistSq && distSq > 0.000001f) {
        float dist = std::sqrt(distSq);
        float overlap = minDist - dist;
        Vec3 normal = delta / dist;

        float invMassA = 1.0f / a.mass;
        float invMassB = 1.0f / b.mass;
        float totalInvMass = invMassA + invMassB;

        // Position correction
        if (!a.isPinned)
          a.position -= normal * (overlap * invMassA / totalInvMass);
        if (!b.isPinned)
          b.position += normal * (overlap * invMassB / totalInvMass);
      }
    }
  }
}

void PhysicsWorld::StepFixed(float fixedDt) {
  Vec3 gravityAccel = m_gravityEnabled ? m_gravity : Vec3(0, 0, 0);

  // === Verlet Integration ===
  for (auto &body : m_bodies) {
    if (body.isPinned)
      continue;

    Vec3 velocity = body.position - body.previousPosition;
    body.previousPosition = body.position;

    // Damping for stability
    velocity *= 0.999f;

    body.position =
        body.position + velocity + gravityAccel * (fixedDt * fixedDt);
  }

  // Apply grabbed orb
  if (m_grabbedOrb >= 0 && m_grabbedOrb < (int)m_bodies.size()) {
    m_bodies[m_grabbedOrb].position = m_grabTarget;
    m_bodies[m_grabbedOrb].previousPosition = m_grabTarget;
  }

  // === Constraint solving with velocity damping ===
  // Rigid constraints get more iterations for stability
  int iterations = (m_bondType == BondType::Rod) ? 8 : 3;

  for (int iter = 0; iter < iterations; ++iter) {
    for (auto &c : m_constraints) {
      if (!c.active)
        continue;
      if (c.bodyA >= m_bodies.size() || c.bodyB >= m_bodies.size())
        continue;

      VerletBody &a = m_bodies[c.bodyA];
      VerletBody &b = m_bodies[c.bodyB];

      Vec3 delta = b.position - a.position;
      float distSq = delta.lengthSq();

      // For tethers, skip if under rest length (fast distSq check)
      if (m_bondType == BondType::Tether &&
          distSq <= c.restLength * c.restLength)
        continue;

      float dist = std::sqrt(distSq);
      if (dist < 0.0001f)
        continue;

      float diff = (dist - c.restLength) / dist;

      float invMassA = 1.0f / a.mass;
      float invMassB = 1.0f / b.mass;
      float totalInvMass = invMassA + invMassB;

      Vec3 correction = delta * diff;

      if (!a.isPinned)
        a.position += correction * (invMassA / totalInvMass);
      if (!b.isPinned)
        b.position = b.position - correction * (invMassB / totalInvMass);
    }
  }

  // === Rigid constraint velocity damping ===
  // After position correction, dampen the relative velocity along the
  // constraint axis. This prevents rigid structures from accumulating
  // rotational energy and spinning out of control.
  if (m_bondType == BondType::Rod) {
    for (auto &c : m_constraints) {
      if (!c.active)
        continue;
      if (c.bodyA >= m_bodies.size() || c.bodyB >= m_bodies.size())
        continue;

      VerletBody &a = m_bodies[c.bodyA];
      VerletBody &b = m_bodies[c.bodyB];

      Vec3 delta = b.position - a.position;
      float dist = delta.length();
      if (dist < 0.0001f)
        continue;

      Vec3 axis = delta / dist;

      // Compute relative velocity along constraint axis
      Vec3 velA = a.position - a.previousPosition;
      Vec3 velB = b.position - b.previousPosition;
      Vec3 relVel = velB - velA;
      float relVelAlongAxis = Vec3::dot(relVel, axis);

      // Dampen relative velocity along the axis (0.3 = 30% damping per step)
      float dampingFactor = 0.3f;
      Vec3 dampImpulse = axis * (relVelAlongAxis * dampingFactor);

      float invMassA = 1.0f / a.mass;
      float invMassB = 1.0f / b.mass;
      float totalInvMass = invMassA + invMassB;

      if (!a.isPinned) {
        a.previousPosition =
            a.previousPosition - dampImpulse * (invMassA / totalInvMass);
      }
      if (!b.isPinned) {
        b.previousPosition =
            b.previousPosition + dampImpulse * (invMassB / totalInvMass);
      }
    }
  }

  // === Container bounds ===
  for (auto &body : m_bodies) {
    if (body.isPinned)
      continue;
    ApplyContainerBounds(body);
  }

  // === Collision detection & response ===
  SolveCollisionsSpatial();
}

void PhysicsWorld::Update(float dt) {
  if (m_bodies.empty())
    return;

  if (m_isPaused) {
    if (m_grabbedOrb >= 0 && m_grabbedOrb < (int)m_bodies.size()) {
      m_bodies[m_grabbedOrb].position = m_grabTarget;
      m_bodies[m_grabbedOrb].previousPosition = m_grabTarget;
    }

    for (auto &c : m_constraints) {
      if (!c.active)
        continue;
      if (c.bodyA >= m_bodies.size() || c.bodyB >= m_bodies.size())
        continue;
      VerletBody &a = m_bodies[c.bodyA];
      VerletBody &b = m_bodies[c.bodyB];
      Vec3 delta = b.position - a.position;
      float dist = delta.length();
      if (dist < 0.0001f)
        continue;
      if (m_bondType == BondType::Tether && dist <= c.restLength)
        continue;

      float diff = (dist - c.restLength) / dist;
      float invMassA = 1.0f / a.mass;
      float invMassB = 1.0f / b.mass;
      float totalInvMass = invMassA + invMassB;
      Vec3 correction = delta * diff;
      if (!a.isPinned && m_grabbedOrb != (int)c.bodyA)
        a.position += correction * (invMassA / totalInvMass);
      if (!b.isPinned && m_grabbedOrb != (int)c.bodyB)
        b.position = b.position - correction * (invMassB / totalInvMass);
    }

    for (auto &body : m_bodies) {
      body.previousPosition = body.position;
    }
    return;
  }

  // Clamp dt
  if (dt > 0.05f)
    dt = 0.05f;

  // Update grid cell size
  UpdateGridCellSize();

  // Fixed timestep accumulator
  m_accumulator += dt;

  // Adaptive max steps based on body count
  int maxSteps = 6;
  if (m_bodies.size() > 2000)
    maxSteps = 2;
  else if (m_bodies.size() > 1000)
    maxSteps = 3;
  else if (m_bodies.size() > 500)
    maxSteps = 4;

  int steps = 0;
  while (m_accumulator >= FIXED_DT && steps < maxSteps) {
    StepFixed(FIXED_DT);
    m_accumulator -= FIXED_DT;
    steps++;
  }

  // Drain excess accumulator
  if (m_accumulator > FIXED_DT * 3.0f) {
    m_accumulator = 0.0f;
  }
}
