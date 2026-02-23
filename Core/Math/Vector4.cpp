//
// Created by Marvin on 28/01/2026.
//

#include "Vector4.h"
#include "Vector3.h"
#include "MathTypes.h"

namespace Math {
    // Static members
    const Vector4 Vector4::Zero(0.0f, 0.0f, 0.0f, 0.0f);
    const Vector4 Vector4::One(1.0f, 1.0f, 1.0f, 1.0f);
    const Vector4 Vector4::UnitX(1.0f, 0.0f, 0.0f, 0.0f);
    const Vector4 Vector4::UnitY(0.0f, 1.0f, 0.0f, 0.0f);
    const Vector4 Vector4::UnitZ(0.0f, 0.0f, 1.0f, 0.0f);
    const Vector4 Vector4::UnitW(0.0f, 0.0f, 0.0f, 1.0f);

    // Constructors
    Vector4::Vector4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}

    Vector4::Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    Vector4::Vector4(float value) : x(value), y(value), z(value), w(value) {}

    Vector4::Vector4(const Vector3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}

    // Arithmetic operators (similar to Vector3)
    Vector4 Vector4::operator+(const Vector4& other) const {
        return Vector4(x + other.x, y + other.y, z + other.z, w + other.w);
    }

    Vector4 Vector4::operator-(const Vector4& other) const {
        return Vector4(x - other.x, y - other.y, z - other.z, w - other.w);
    }

    Vector4 Vector4::operator*(float scalar) const {
        return Vector4(x * scalar, y * scalar, z * scalar, w * scalar);
    }

    Vector4 Vector4::operator/(float scalar) const {
        return Vector4(x / scalar, y / scalar, z / scalar, w / scalar);
    }

    Vector3 Vector4::ToVector3()
    {
        return Vector3(x, y, z);
    }

    // ... (rest of implementation similar to Vector3)

    float Vector4::Dot(const Vector4& other) const {
        return x * other.x + y * other.y + z * other.z + w * other.w;
    }

    // Non-member operator
    Vector4 operator*(float scalar, const Vector4& v) {
        return v * scalar;
    }
}