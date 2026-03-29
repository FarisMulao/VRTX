#include "PhysicsWorld.h"
#include <cmath>

PhysicsWorld::PhysicsWorld() : m_grid(0.8f) {} 
PhysicsWorld::~PhysicsWorld() {}

void PhysicsWorld::SpawnOrb(Vec3 pos, float radius) {
    VerletBody body = {};
    body.position = pos;
    body.previousPosition = pos;
    body.radius = radius;
    body.isPinned = 0;
    m_bodies.push_back(body);
}

int PhysicsWorld::RayPick(Vec3 rayOrigin, Vec3 rayDir) const {
    float bestT = 1e30f;
    int bestIdx = -1;
    
    for (size_t i = 0; i < m_bodies.size(); ++i) {
        Vec3 oc = rayOrigin - m_bodies[i].position;
        float a = Vec3::dot(rayDir, rayDir);
        float b = 2.0f * Vec3::dot(oc, rayDir);
        float c = Vec3::dot(oc, oc) - m_bodies[i].radius * m_bodies[i].radius;
        float disc = b * b - 4.0f * a * c;
        
        if (disc >= 0.0f) {
            float t = (-b - std::sqrt(disc)) / (2.0f * a);
            if (t > 0.0f && t < bestT) {
                bestT = t;
                bestIdx = (int)i;
            }
        }
    }
    return bestIdx;
}

void PhysicsWorld::ApplyContainerBounds(VerletBody& body) {
    if (m_containerType == ContainerType::Sphere) {
        Vec3 toBody = body.position - m_containerCenter;
        float dist = toBody.length();
        float maxDist = m_containerRadius - body.radius;
        
        if (dist > maxDist && dist > 0.0001f) {
            Vec3 normal = toBody / dist;
            body.position = m_containerCenter + normal * maxDist;
        }
    } else {
        float hs = m_containerHalfSize - body.radius;
        Vec3& p = body.position;
        Vec3& c = m_containerCenter;
        
        if (p.x_val < c.x_val - hs) p.x_val = c.x_val - hs;
        if (p.x_val > c.x_val + hs) p.x_val = c.x_val + hs;
        if (p.y_val < c.y_val - hs) p.y_val = c.y_val - hs;
        if (p.y_val > c.y_val + hs) p.y_val = c.y_val + hs;
        if (p.z_val < c.z_val - hs) p.z_val = c.z_val - hs;
        if (p.z_val > c.z_val + hs) p.z_val = c.z_val + hs;
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
            if (j <= i) continue; 
            
            VerletBody& a = m_bodies[i];
            VerletBody& b = m_bodies[j];
            
            Vec3 delta = b.position - a.position;
            float distSq = delta.lengthSq();
            float minDist = a.radius + b.radius;
            
            if (distSq < minDist * minDist && distSq > 0.000001f) {
                float dist = std::sqrt(distSq);
                float overlap = (minDist - dist) * 0.5f;
                Vec3 normal = delta / dist;
                
                if (!a.isPinned) a.position -= normal * overlap;
                if (!b.isPinned) b.position += normal * overlap;
            }
        }
    }
}

void PhysicsWorld::Update(float dt) {
    if (m_bodies.empty()) return;
    
    if (m_isPaused) {
        if (m_grabbedOrb >= 0 && m_grabbedOrb < (int)m_bodies.size()) {
            m_bodies[m_grabbedOrb].position = m_grabTarget;
            m_bodies[m_grabbedOrb].previousPosition = m_grabTarget;
        }
        
        
        for (auto& c : m_constraints) {
            if (!c.active) continue;
            VerletBody& a = m_bodies[c.bodyA];
            VerletBody& b = m_bodies[c.bodyB];
            Vec3 delta = b.position - a.position;
            float dist = delta.length();
            if (dist < 0.0001f) continue;
            if (m_bondType == BondType::Tether && dist <= c.restLength) continue;
            
            float diff = (dist - c.restLength) / dist;
            Vec3 correction = delta * (0.5f * diff);
            if (!a.isPinned && m_grabbedOrb != c.bodyA) a.position += correction;
            if (!b.isPinned && m_grabbedOrb != c.bodyB) b.position = b.position - correction;
        }
        
        
        for (auto& body : m_bodies) {
            body.previousPosition = body.position;
        }
        return;
    }
    
    if (dt > 0.033f) dt = 0.033f;
    
    const int substeps = 4;
    float subDt = dt / substeps;
    
    for (int step = 0; step < substeps; ++step) {
        for (auto& body : m_bodies) {
            if (body.isPinned) continue;
            
            Vec3 velocity = body.position - body.previousPosition;
            body.previousPosition = body.position;
            velocity *= 0.999f;
            body.position = body.position + velocity + m_gravity * (subDt * subDt);
        }
        
        
        if (m_grabbedOrb >= 0 && m_grabbedOrb < (int)m_bodies.size()) {
            m_bodies[m_grabbedOrb].position = m_grabTarget;
            m_bodies[m_grabbedOrb].previousPosition = m_grabTarget;
        }
        
        
        int iterations = (m_bondType == BondType::Rod) ? 15 : 5;
        
        for (int iter = 0; iter < iterations; ++iter) {
            for (auto& c : m_constraints) {
                if (!c.active) continue;
                if (c.bodyA >= m_bodies.size() || c.bodyB >= m_bodies.size()) continue;
                
                VerletBody& a = m_bodies[c.bodyA];
                VerletBody& b = m_bodies[c.bodyB];
                
                Vec3 delta = b.position - a.position;
                float dist = delta.length();
                if (dist < 0.0001f) continue;
                
                
                if (m_bondType == BondType::Tether && dist <= c.restLength) continue;
                
                float diff = (dist - c.restLength) / dist;
                Vec3 correction = delta * (0.5f * diff);
                
                if (!a.isPinned) a.position += correction;
                if (!b.isPinned) b.position = b.position - correction;
            }
        }
        
        
        for (auto& body : m_bodies) {
            if (body.isPinned) continue;
            ApplyContainerBounds(body);
        }
        
        
        SolveCollisionsSpatial();
    }
}
