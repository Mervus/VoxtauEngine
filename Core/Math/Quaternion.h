//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_QUATERNION_H
#define DIRECTX11_QUATERNION_H


#pragma once
#include "Vector3.h"
#include <EngineApi.h>

namespace Math {
    class Matrix4x4;

    class Quaternion {
    public:
        float x, y, z, w;

        // Constructors
        Quaternion();
        Quaternion(float x, float y, float z, float w);
        Quaternion(const Vector3& axis, float angle);

        // Static predefined quaternions
        static const Quaternion Identity;

        // Arithmetic operators
        Quaternion operator+(const Quaternion& other) const;
        Quaternion operator-(const Quaternion& other) const;
        Quaternion operator*(const Quaternion& other) const;
        Quaternion operator*(float scalar) const;

        Quaternion& operator+=(const Quaternion& other);
        Quaternion& operator-=(const Quaternion& other);
        Quaternion& operator*=(const Quaternion& other);
        Quaternion& operator*=(float scalar);

        // Comparison
        bool operator==(const Quaternion& other) const;
        bool operator!=(const Quaternion& other) const;

        // Quaternion operations
        float Length() const;
        float LengthSquared() const;
        void Normalize();
        Quaternion Normalized() const;

        Quaternion Conjugate() const;
        Quaternion Inverse() const;

        float Dot(const Quaternion& other) const;

        // Rotation operations
        Vector3 RotateVector(const Vector3& v) const;
        Matrix4x4 ToMatrix() const;
        Vector3 ToEulerAngles() const; // Pitch, Yaw, Roll

        // Static functions
        static Quaternion FromAxisAngle(const Vector3& axis, float angle);
        static Quaternion FromEulerAngles(float pitch, float yaw, float roll);
        static Quaternion FromEulerAngles(const Vector3& euler);
        static Quaternion FromMatrix(const Matrix4x4& m);

        static float Dot(const Quaternion& a, const Quaternion& b);
        static Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t);
        static Quaternion Lerp(const Quaternion& a, const Quaternion& b, float t);
        static Quaternion LookRotation(const Vector3& forward, const Vector3& up);
    };

    Quaternion operator*(float scalar, const Quaternion& q);
}

#endif //DIRECTX11_QUATERNION_H