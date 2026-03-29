#pragma once
#include "../math/Mat4.h"
#include "../math/Vec3.h"

class Camera {
public:
  Camera(Vec3 pos = Vec3(0, 0, 0), Vec3 target = Vec3(0, 0, 1),
         float fov = 1.0472f, float aspect = 1.777f);

  void Update(float dt);

  // Free cam controls
  void SetFreeCam(bool enabled);
  bool IsFreeCam() const { return m_freeCam; }
  void ProcessMouseLook(float dx, float dy);
  void ProcessMovement(float dt, bool forward, bool back, bool left, bool right,
                       bool up, bool down);

  Mat4 GetViewMatrix() const;
  Mat4 GetProjectionMatrix() const;

  Vec3 m_position;
  Vec3 m_target;
  float m_fov;
  float m_aspect;
  float m_zn = 0.1f;
  float m_zf = 1000.0f;

  // Free cam state
  bool m_freeCam = false;
  float m_yaw = 0.0f;   // Radians, around Y
  float m_pitch = 0.0f; // Radians, around X
  float m_moveSpeed = 15.0f;
  float m_lookSensitivity = 0.003f;
};
