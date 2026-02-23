//
// Created by Marvin on 28/01/2026.
//

#include "Vector2.h"
#include "MathTypes.h"

namespace Math {
    // Static members
    const Vector2 Vector2::Zero(0.0f, 0.0f);
    const Vector2 Vector2::One(1.0f, 1.0f);
    const Vector2 Vector2::UnitX(1.0f, 0.0f);
    const Vector2 Vector2::UnitY(0.0f, 1.0f);

    // Constructors
    Vector2::Vector2() : x(0.0f), y(0.0f) {}

    Vector2::Vector2(float x, float y) : x(x), y(y) {}

    Vector2::Vector2(float value) : x(value), y(value) {}

    // Arithmetic operators
    Vector2 Vector2::operator+(const Vector2& other) const {
        return Vector2(x + other.x, y + other.y);
    }

    Vector2 Vector2::operator-(const Vector2& other) const {
        return Vector2(x - other.x, y - other.y);
    }

    Vector2 Vector2::operator*(float scalar) const {
        return Vector2(x * scalar, y * scalar);
    }

    Vector2 Vector2::operator/(float scalar) const {
        return Vector2(x / scalar, y / scalar);
    }

    Vector2& Vector2::operator+=(const Vector2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vector2& Vector2::operator-=(const Vector2& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    Vector2& Vector2::operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    Vector2& Vector2::operator/=(float scalar) {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    // Comparison
    bool Vector2::operator==(const Vector2& other) const {
        return std::abs(x - other.x) < EPSILON &&
               std::abs(y - other.y) < EPSILON;
    }

    bool Vector2::operator!=(const Vector2& other) const {
        return !(*this == other);
    }

    // Unary operators
    Vector2 Vector2::operator-() const {
        return Vector2(-x, -y);
    }

    // Vector operations
    float Vector2::Length() const {
        return std::sqrt(x * x + y * y);
    }

    float Vector2::LengthSquared() const {
        return x * x + y * y;
    }

    void Vector2::Normalize() {
        float len = Length();
        if (len > EPSILON) {
            x /= len;
            y /= len;
        }
    }

    Vector2 Vector2::Normalized() const {
        Vector2 result = *this;
        result.Normalize();
        return result;
    }

    float Vector2::Dot(const Vector2& other) const {
        return x * other.x + y * other.y;
    }

    float Vector2::Distance(const Vector2& other) const {
        return (*this - other).Length();
    }

    float Vector2::DistanceSquared(const Vector2& other) const {
        return (*this - other).LengthSquared();
    }

    // Static functions
    float Vector2::Dot(const Vector2& a, const Vector2& b) {
        return a.x * b.x + a.y * b.y;
    }

    float Vector2::Distance(const Vector2& a, const Vector2& b) {
        return (a - b).Length();
    }

    Vector2 Vector2::Lerp(const Vector2& a, const Vector2& b, float t) {
        return a + (b - a) * t;
    }

    Vector2 Vector2::Normalize(const Vector2& v) {
        return v.Normalized();
    }

    // Non-member operator
    Vector2 operator*(float scalar, const Vector2& v) {
        return v * scalar;
    }
}