#include "Camera.h"
#include <cmath>

Camera::Camera(Vec3 pos, Vec3 target, float fov, float aspect)
    : m_position(pos), m_target(target), m_fov(fov), m_aspect(aspect) {
  Vec3 dir = (target - pos).normalized();
  m_yaw = std::atan2(dir.x_val, dir.z_val);
  m_pitch = std::asin(dir.y_val);
}

Mat4 Camera::GetViewMatrix() const {
  return Mat4::LookAtLH(m_position, m_target, Vec3(0, 1, 0));
}

Mat4 Camera::GetProjectionMatrix() const {
  return Mat4::PerspectiveFovLH(m_fov, m_aspect, m_zn, m_zf);
}

void Camera::SetFreeCam(bool enabled) {
  m_freeCam = enabled;
  if (enabled) {
    Vec3 dir = (m_target - m_position).normalized();
    m_yaw = std::atan2(dir.x_val, dir.z_val);
    m_pitch = std::asin(dir.y_val);
  }
}

void Camera::ProcessMouseLook(float dx, float dy) {
  if (!m_freeCam)
    return;

  m_yaw += dx * m_lookSensitivity;
  m_pitch -= dy * m_lookSensitivity;

  // Clamp pitch to avoid gimbal lock
  const float maxPitch = 1.5f;
  if (m_pitch > maxPitch)
    m_pitch = maxPitch;
  if (m_pitch < -maxPitch)
    m_pitch = -maxPitch;

  Vec3 forward;
  forward.x_val = std::sin(m_yaw) * std::cos(m_pitch);
  forward.y_val = std::sin(m_pitch);
  forward.z_val = std::cos(m_yaw) * std::cos(m_pitch);
  m_target = m_position + forward;
}

void Camera::ProcessMovement(float dt, bool fwd, bool back, bool left,
                             bool right, bool up, bool down) {
  if (!m_freeCam)
    return;

  Vec3 forward = (m_target - m_position).normalized();
  Vec3 worldUp(0, 1, 0);
  Vec3 rightDir = Vec3::cross(worldUp, forward).normalized();

  float speed = m_moveSpeed * dt;
  Vec3 move(0, 0, 0);

  if (fwd)
    move += forward * speed;
  if (back)
    move -= forward * speed;
  if (left)
    move -= rightDir * speed;
  if (right)
    move += rightDir * speed;
  if (up)
    move += worldUp * speed;
  if (down)
    move -= worldUp * speed;

  m_position += move;
  m_target += move;
}

void Camera::Update(float dt) {}
