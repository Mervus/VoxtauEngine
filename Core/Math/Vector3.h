//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_VECTOR3_H
#define DIRECTX11_VECTOR3_H
#pragma once
#include <EngineApi.h>
#include "MathTypes.h"

namespace Math {
    class Vector3 {
    public:
        float x, y, z;

        // Constructors
        Vector3();
        Vector3(float x, float y, float z);
        Vector3(float value);

        static const Vector3 Zero;
        static const Vector3 One;
        static const Vector3 UnitX;
        static const Vector3 UnitY;
        static const Vector3 UnitZ;
        static const Vector3 Up;
        static const Vector3 Down;
        static const Vector3 Left;
        static const Vector3 Right;
        static const Vector3 Forward;
        static const Vector3 Backward;

        // Arithmetic operators
        Vector3 operator+(const Vector3& other) const;
        Vector3 operator-(const Vector3& other) const;
        Vector3 operator*(float scalar) const;
        Vector3 operator/(float scalar) const;

        Vector3& operator+=(const Vector3& other);
        Vector3& operator-=(const Vector3& other);
        Vector3& operator*=(float scalar);
        Vector3& operator/=(float scalar);

        // Comparison
        bool operator==(const Vector3& other) const;
        bool operator!=(const Vector3& other) const;

        // Unary operators
        Vector3 operator-() const;

        // Vector operations
        float Length() const;
        float LengthSquared() const;
        void Normalize();
        Vector3 Normalized() const;

        float Dot(const Vector3& other) const;
        Vector3 Cross(const Vector3& other) const;

        float Distance(const Vector3& other) const;
        float DistanceSquared(const Vector3& other) const;

        void toConsole() const;

        // Static functions
        static float Dot(const Vector3& a, const Vector3& b);
        static Vector3 Cross(const Vector3& a, const Vector3& b);
        static float Distance(const Vector3& a, const Vector3& b);
        static Vector3 Lerp(const Vector3& a, const Vector3& b, float t);
        static Vector3 Normalize(const Vector3& v);
        static Vector3 Reflect(const Vector3& v, const Vector3& normal);
        static Vector3 Project(const Vector3& v, const Vector3& onto);
    };



    // Static predefined vectors


    Vector3 operator*(float scalar, const Vector3& v);
}


#endif //DIRECTX11_VECTOR3_H