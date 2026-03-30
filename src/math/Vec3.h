#pragma once
#include <cmath>

struct Vec3 {
  float x_val, y_val, z_val; // Named to avoid union issues

  float &x() { return x_val; }
  float &y() { return y_val; }
  float &z() { return z_val; }
  const float &x() const { return x_val; }
  const float &y() const { return y_val; }
  const float &z() const { return z_val; }

  Vec3() : x_val(0), y_val(0), z_val(0) {}
  Vec3(float x, float y, float z) : x_val(x), y_val(y), z_val(z) {}

  Vec3 operator+(const Vec3 &o) const {
    return {x_val + o.x_val, y_val + o.y_val, z_val + o.z_val};
  }
  Vec3 operator-(const Vec3 &o) const {
    return {x_val - o.x_val, y_val - o.y_val, z_val - o.z_val};
  }
  Vec3 operator*(float s) const { return {x_val * s, y_val * s, z_val * s}; }
  Vec3 operator/(float s) const { return {x_val / s, y_val / s, z_val / s}; }
  Vec3 operator-() const { return {-x_val, -y_val, -z_val}; }

  Vec3 &operator+=(const Vec3 &o) {
    x_val += o.x_val;
    y_val += o.y_val;
    z_val += o.z_val;
    return *this;
  }
  Vec3 &operator-=(const Vec3 &o) {
    x_val -= o.x_val;
    y_val -= o.y_val;
    z_val -= o.z_val;
    return *this;
  }
  Vec3 &operator*=(float s) {
    x_val *= s;
    y_val *= s;
    z_val *= s;
    return *this;
  }

  static float dot(const Vec3 &a, const Vec3 &b) {
    return a.x_val * b.x_val + a.y_val * b.y_val + a.z_val * b.z_val;
  }

  static Vec3 cross(const Vec3 &a, const Vec3 &b) {
    return {a.y_val * b.z_val - a.z_val * b.y_val,
            a.z_val * b.x_val - a.x_val * b.z_val,
            a.x_val * b.y_val - a.y_val * b.x_val};
  }

  float lengthSq() const {
    return x_val * x_val + y_val * y_val + z_val * z_val;
  }
  float length() const { return std::sqrt(lengthSq()); }

  Vec3 normalized() const {
    float len = length();
    if (len > 0.00001f) {
      return *this / len;
    }
    return {0, 0, 0};
  }
};
