#pragma once
#include "Vec3.h"
#include <DirectXMath.h>

struct Mat4 {
    float m[4][4];

    Mat4() {
        DirectX::XMStoreFloat4x4((DirectX::XMFLOAT4X4*)m, DirectX::XMMatrixIdentity());
    }
    
    Mat4(DirectX::FXMMATRIX mat) {
        DirectX::XMStoreFloat4x4((DirectX::XMFLOAT4X4*)m, mat);
    }
    
    Mat4 operator*(const Mat4& o) const {
        DirectX::XMMATRIX m1 = DirectX::XMLoadFloat4x4((const DirectX::XMFLOAT4X4*)m);
        DirectX::XMMATRIX m2 = DirectX::XMLoadFloat4x4((const DirectX::XMFLOAT4X4*)o.m);
        return Mat4(DirectX::XMMatrixMultiply(m1, m2));
    }

    static Mat4 Identity() { return Mat4(); }

    static Mat4 Translation(const Vec3& t) {
        return Mat4(DirectX::XMMatrixTranslation(t.x_val, t.y_val, t.z_val));
    }
    
    static Mat4 Scale(const Vec3& s) {
        return Mat4(DirectX::XMMatrixScaling(s.x_val, s.y_val, s.z_val));
    }
    
    static Mat4 RotationY(float angle) {
        return Mat4(DirectX::XMMatrixRotationY(angle));
    }
    
    static Mat4 RotationX(float angle) {
        return Mat4(DirectX::XMMatrixRotationX(angle));
    }

    static Mat4 LookAtLH(const Vec3& eye, const Vec3& focus, const Vec3& up) {
        DirectX::XMVECTOR vEye = DirectX::XMVectorSet(eye.x_val, eye.y_val, eye.z_val, 1.0f);
        DirectX::XMVECTOR vFocus = DirectX::XMVectorSet(focus.x_val, focus.y_val, focus.z_val, 1.0f);
        DirectX::XMVECTOR vUp = DirectX::XMVectorSet(up.x_val, up.y_val, up.z_val, 0.0f);
        return Mat4(DirectX::XMMatrixLookAtLH(vEye, vFocus, vUp));
    }

    static Mat4 PerspectiveFovLH(float fovY, float aspect, float zn, float zf) {
        return Mat4(DirectX::XMMatrixPerspectiveFovLH(fovY, aspect, zn, zf));
    }

    Mat4 Inverse() const {
        DirectX::XMMATRIX xm = DirectX::XMLoadFloat4x4((const DirectX::XMFLOAT4X4*)m);
        DirectX::XMVECTOR det;
        return Mat4(DirectX::XMMatrixInverse(&det, xm));
    }
    
    Mat4 Transpose() const {
        DirectX::XMMATRIX xm = DirectX::XMLoadFloat4x4((const DirectX::XMFLOAT4X4*)m);
        return Mat4(DirectX::XMMatrixTranspose(xm));
    }
};
