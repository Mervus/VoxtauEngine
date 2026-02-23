//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_VECTOR4_H
#define DIRECTX11_VECTOR4_H


#pragma once
#include <cmath>
#include <EngineApi.h>

namespace Math {
    class ENGINE_API Vector4 {
    public:
        float x, y, z, w;

        // Constructors
        Vector4();
        Vector4(float x, float y, float z, float w);
        Vector4(float value);
        Vector4(const class Vector3& v, float w);

        // Static predefined vectors
        static const Vector4 Zero;
        static const Vector4 One;
        static const Vector4 UnitX;
        static const Vector4 UnitY;
        static const Vector4 UnitZ;
        static const Vector4 UnitW;

        // Arithmetic operators
        Vector4 operator+(const Vector4& other) const;
        Vector4 operator-(const Vector4& other) const;
        Vector4 operator*(float scalar) const;
        Vector4 operator/(float scalar) const;

        Vector4& operator+=(const Vector4& other);
        Vector4& operator-=(const Vector4& other);
        Vector4& operator*=(float scalar);
        Vector4& operator/=(float scalar);

        // Comparison
        bool operator==(const Vector4& other) const;
        bool operator!=(const Vector4& other) const;

        // Unary operators
        Vector4 operator-() const;

        // Vector operations
        float Length() const;
        float LengthSquared() const;
        void Normalize();
        Vector4 Normalized() const;

        float Dot(const Vector4& other) const;

        //Helper
        Vector3 ToVector3();

        // Static functions
        static float Dot(const Vector4& a, const Vector4& b);
        static Vector4 Lerp(const Vector4& a, const Vector4& b, float t);
        static Vector4 Normalize(const Vector4& v);
    };

    Vector4 operator*(float scalar, const Vector4& v);
}


#endif //DIRECTX11_VECTOR4_H