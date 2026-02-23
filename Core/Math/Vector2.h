//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_VECTOR2_H
#define DIRECTX11_VECTOR2_H


#pragma once
#include <cmath>
#include <EngineApi.h>

namespace Math {
    class ENGINE_API Vector2 {
    public:
        float x, y;

        // Constructors
        Vector2();
        Vector2(float x, float y);
        Vector2(float value); // Both x and y = value

        // Static predefined vectors
        static const Vector2 Zero;
        static const Vector2 One;
        static const Vector2 UnitX;
        static const Vector2 UnitY;

        // Arithmetic operators
        Vector2 operator+(const Vector2& other) const;
        Vector2 operator-(const Vector2& other) const;
        Vector2 operator*(float scalar) const;
        Vector2 operator/(float scalar) const;

        Vector2& operator+=(const Vector2& other);
        Vector2& operator-=(const Vector2& other);
        Vector2& operator*=(float scalar);
        Vector2& operator/=(float scalar);

        // Comparison
        bool operator==(const Vector2& other) const;
        bool operator!=(const Vector2& other) const;

        // Unary operators
        Vector2 operator-() const;

        // Vector operations
        float Length() const;
        float LengthSquared() const;
        void Normalize();
        Vector2 Normalized() const;

        float Dot(const Vector2& other) const;
        float Distance(const Vector2& other) const;
        float DistanceSquared(const Vector2& other) const;

        // Static functions
        static float Dot(const Vector2& a, const Vector2& b);
        static float Distance(const Vector2& a, const Vector2& b);
        static Vector2 Lerp(const Vector2& a, const Vector2& b, float t);
        static Vector2 Normalize(const Vector2& v);
    };

    // Non-member operator (for scalar * vector)
    Vector2 operator*(float scalar, const Vector2& v);
}

#endif //DIRECTX11_VECTOR2_H