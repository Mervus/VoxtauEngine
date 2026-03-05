//
// Created by Marvin on 28/01/2026.
//

#include "Vector3.h"
#include "MathTypes.h"
#include <iostream>
#include <string>

namespace Math {
    // Static members
    const Vector3 Vector3::Zero(0.0f, 0.0f, 0.0f);
    const Vector3 Vector3::One(1.0f, 1.0f, 1.0f);
    const Vector3 Vector3::UnitX(1.0f, 0.0f, 0.0f);
    const Vector3 Vector3::UnitY(0.0f, 1.0f, 0.0f);
    const Vector3 Vector3::UnitZ(0.0f, 0.0f, 1.0f);
    const Vector3 Vector3::Up(0.0f, 1.0f, 0.0f);
    const Vector3 Vector3::Down(0.0f, -1.0f, 0.0f);
    const Vector3 Vector3::Left(-1.0f, 0.0f, 0.0f);
    const Vector3 Vector3::Right(1.0f, 0.0f, 0.0f);
    const Vector3 Vector3::Forward (0.0f, 0.0f, 1.0f);
    const Vector3 Vector3::Backward(0.0f, 0.0f,-1.0f);

    // Constructors
    Vector3::Vector3() : x(0.0f), y(0.0f), z(0.0f) {}

    Vector3::Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vector3::Vector3(float value) : x(value), y(value), z(value) {}

    // Arithmetic operators
    Vector3 Vector3::operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }

    Vector3 Vector3::operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }

    Vector3 Vector3::operator*(float scalar) const {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }

    Vector3 Vector3::operator/(float scalar) const {
        return Vector3(x / scalar, y / scalar, z / scalar);
    }

    Vector3& Vector3::operator+=(const Vector3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vector3& Vector3::operator-=(const Vector3& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    Vector3& Vector3::operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    Vector3& Vector3::operator/=(float scalar) {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }

    // Comparison
    bool Vector3::operator==(const Vector3& other) const {
        return std::abs(x - other.x) < EPSILON &&
               std::abs(y - other.y) < EPSILON &&
               std::abs(z - other.z) < EPSILON;
    }

    bool Vector3::operator!=(const Vector3& other) const {
        return !(*this == other);
    }

    // Unary operators
    Vector3 Vector3::operator-() const {
        return Vector3(-x, -y, -z);
    }

    // Vector operations
    float Vector3::Length() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    float Vector3::LengthSquared() const {
        return x * x + y * y + z * z;
    }

    void Vector3::Normalize() {
        float len = Length();
        if (len > EPSILON) {
            x /= len;
            y /= len;
            z /= len;
        }
    }

    Vector3 Vector3::Normalized() const {
        Vector3 result = *this;
        result.Normalize();
        return result;
    }

    float Vector3::Dot(const Vector3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    Vector3 Vector3::Cross(const Vector3& other) const {
        return Vector3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }

    float Vector3::Distance(const Vector3& other) const {
        return (*this - other).Length();
    }

    float Vector3::DistanceSquared(const Vector3& other) const {
        return (*this - other).LengthSquared();
    }

    void Vector3::toConsole() const
    {
        std::cout << "Vector3(" << x << ", " << y << ", " << z << ")" << std::endl;
    }

    // Static functions
    float Vector3::Dot(const Vector3& a, const Vector3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    Vector3 Vector3::Cross(const Vector3& a, const Vector3& b) {
        return a.Cross(b);
    }

    float Vector3::Distance(const Vector3& a, const Vector3& b) {
        return (a - b).Length();
    }

    Vector3 Vector3::Lerp(const Vector3& a, const Vector3& b, float t) {
        return a + (b - a) * t;
    }

    Vector3 Vector3::Normalize(const Vector3& v) {
        return v.Normalized();
    }

    Vector3 Vector3::Reflect(const Vector3& v, const Vector3& normal) {
        return v - 2.0f * Dot(v, normal) * normal;
    }

    Vector3 Vector3::Project(const Vector3& v, const Vector3& onto) {
        float dot = Dot(v, onto);
        float lenSq = onto.LengthSquared();
        return onto * (dot / lenSq);
    }

    // Non-member operator
    Vector3 operator*(float scalar, const Vector3& v) {
        return v * scalar;
    }
}
