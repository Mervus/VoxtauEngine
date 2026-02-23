//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_MATRIX4X4_H
#define DIRECTX11_MATRIX4X4_H


#pragma once
#include "Vector4.h"
#include <EngineApi.h>

namespace Math {
    class ENGINE_API Matrix4x4 {
    public:
        // Row-major order: m[row][column]
        float m[4][4];

        // Constructors
        Matrix4x4();
        Matrix4x4(float diagonal);

        // Static matrices
        static const Matrix4x4 Identity;
        static const Matrix4x4 Zero;

        // Matrix operations
        Matrix4x4 operator*(const Matrix4x4& other) const;
        Vector4 operator*(const Vector4& v) const;
        Matrix4x4& operator*=(const Matrix4x4& other);

        // Transform operations
        static Matrix4x4 Translation(const Vector3& position);
        static Matrix4x4 Rotation(const Vector3& axis, float angle);
        static Matrix4x4 RotationX(float angle);
        static Matrix4x4 RotationY(float angle);
        static Matrix4x4 RotationZ(float angle);
        static Matrix4x4 Scale(const Vector3& scale);
        static Matrix4x4 Scale(float uniformScale);

        // View/Projection matrices
        static Matrix4x4 LookAt(const Vector3& eye, const Vector3& target, const Vector3& up);
        static Matrix4x4 Perspective(float fov, float aspect, float near, float far);
        static Matrix4x4 Orthographic(float left, float right, float bottom, float top, float near, float far);

        // Utility
        [[nodiscard]] Matrix4x4 Transposed() const;
        [[nodiscard]] Matrix4x4 Inverted() const;
        float Determinant() const;
    };
}


#endif //DIRECTX11_MATRIX4X4_H