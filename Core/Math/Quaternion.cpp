//
// Created by Marvin on 28/01/2026.
//
#include "Quaternion.h"
#include "Matrix4x4.h"
#include "MathTypes.h"
#include <cmath>

namespace Math {
    // Static members
    const Quaternion Quaternion::Identity = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);

    // Constructors
    Quaternion::Quaternion() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}

    Quaternion::Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    Quaternion::Quaternion(const Vector3& axis, float angle) {
        float halfAngle = angle * 0.5f;
        float s = std::sin(halfAngle);

        Vector3 normalizedAxis = axis.Normalized();
        x = normalizedAxis.x * s;
        y = normalizedAxis.y * s;
        z = normalizedAxis.z * s;
        w = std::cos(halfAngle);
    }

    // Arithmetic operators
    Quaternion Quaternion::operator+(const Quaternion& other) const {
        return Quaternion(x + other.x, y + other.y, z + other.z, w + other.w);
    }

    Quaternion Quaternion::operator-(const Quaternion& other) const {
        return Quaternion(x - other.x, y - other.y, z - other.z, w - other.w);
    }

    Quaternion Quaternion::operator*(const Quaternion& other) const {
        return Quaternion(
            w * other.x + x * other.w + y * other.z - z * other.y,
            w * other.y - x * other.z + y * other.w + z * other.x,
            w * other.z + x * other.y - y * other.x + z * other.w,
            w * other.w - x * other.x - y * other.y - z * other.z
        );
    }

    Quaternion Quaternion::operator*(float scalar) const {
        return Quaternion(x * scalar, y * scalar, z * scalar, w * scalar);
    }

    Quaternion& Quaternion::operator+=(const Quaternion& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }

    Quaternion& Quaternion::operator*=(const Quaternion& other) {
        *this = *this * other;
        return *this;
    }

    Quaternion& Quaternion::operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        w *= scalar;
        return *this;
    }

    // Comparison
    bool Quaternion::operator==(const Quaternion& other) const {
        return std::abs(x - other.x) < EPSILON &&
               std::abs(y - other.y) < EPSILON &&
               std::abs(z - other.z) < EPSILON &&
               std::abs(w - other.w) < EPSILON;
    }

    bool Quaternion::operator!=(const Quaternion& other) const {
        return !(*this == other);
    }

    // Quaternion operations
    float Quaternion::Length() const {
        return std::sqrt(x * x + y * y + z * z + w * w);
    }

    float Quaternion::LengthSquared() const {
        return x * x + y * y + z * z + w * w;
    }

    void Quaternion::Normalize() {
        float len = Length();
        if (len > EPSILON) {
            x /= len;
            y /= len;
            z /= len;
            w /= len;
        }
    }

    Quaternion Quaternion::Normalized() const {
        Quaternion result = *this;
        result.Normalize();
        return result;
    }

    Quaternion Quaternion::Conjugate() const {
        return Quaternion(-x, -y, -z, w);
    }

    Quaternion Quaternion::Inverse() const {
        float lenSq = LengthSquared();
        if (lenSq > EPSILON) {
            return Conjugate() * (1.0f / lenSq);
        }
        return Identity;
    }

    float Quaternion::Dot(const Quaternion& other) const {
        return x * other.x + y * other.y + z * other.z + w * other.w;
    }

    // Rotate a vector
    Vector3 Quaternion::RotateVector(const Vector3& v) const {
        Quaternion q = Normalized(); // ensure unit quaternion

        Vector3 u(q.x, q.y, q.z);
        float s = q.w;

        return 2.0f * Vector3::Dot(u, v) * u
             + (s * s - Vector3::Dot(u, u)) * v
             + 2.0f * s * Vector3::Cross(u, v);
    }

    // Convert to rotation matrix (row-vector convention: v * M)
    Matrix4x4 Quaternion::ToMatrix() const {
        Matrix4x4 result = Matrix4x4::Identity;

        float xx = x * x;
        float yy = y * y;
        float zz = z * z;
        float xy = x * y;
        float xz = x * z;
        float yz = y * z;
        float wx = w * x;
        float wy = w * y;
        float wz = w * z;

        // Transposed for row-vector convention
        result.m[0][0] = 1.0f - 2.0f * (yy + zz);
        result.m[0][1] = 2.0f * (xy + wz);
        result.m[0][2] = 2.0f * (xz - wy);

        result.m[1][0] = 2.0f * (xy - wz);
        result.m[1][1] = 1.0f - 2.0f * (xx + zz);
        result.m[1][2] = 2.0f * (yz + wx);

        result.m[2][0] = 2.0f * (xz + wy);
        result.m[2][1] = 2.0f * (yz - wx);
        result.m[2][2] = 1.0f - 2.0f * (xx + yy);

        //std::cout << "ToMatrix this: " << x << "," << y << "," << z << "," << w << std::endl;

        return result;
    }

    // Convert to Euler angles (pitch, yaw, roll)
    // Unity-style YXZ intrinsic extraction
    // Pitch(X) is the middle rotation -> extracted via asin
    Vector3 Quaternion::ToEulerAngles() const {
        Vector3 euler;

        // Pitch (x-axis rotation) - middle rotation, extracted via asin
        float sinp = 2.0f * (w * x - y * z);
        if (std::abs(sinp) >= 1.0f) {
            euler.x = std::copysign(HALF_PI, sinp);
        } else {
            euler.x = std::asin(sinp);
        }

        // Yaw (y-axis rotation)
        euler.y = std::atan2(2.0f * (x * z + w * y), 1.0f - 2.0f * (x * x + y * y));

        // Roll (z-axis rotation)
        euler.z = std::atan2(2.0f * (x * y + w * z), 1.0f - 2.0f * (x * x + z * z));

        return euler;
    }

    // Static functions
    Quaternion Quaternion::FromAxisAngle(const Vector3& axis, float angle) {
        return Quaternion(axis, angle);
    }

    Quaternion Quaternion::FromEulerAngles(float pitch, float yaw, float roll) {
        // Creates quaternion from Euler angles in radians
        // pitch = X-axis, yaw = Y-axis, roll = Z-axis
        // Unity-style YXZ intrinsic: Qy(yaw) * Qx(pitch) * Qz(roll)

        float cy = std::cos(yaw * 0.5f);
        float sy = std::sin(yaw * 0.5f);
        float cp = std::cos(pitch * 0.5f);
        float sp = std::sin(pitch * 0.5f);
        float cr = std::cos(roll * 0.5f);
        float sr = std::sin(roll * 0.5f);

        Quaternion q;
        q.w = cy * cp * cr + sy * sp * sr;
        q.x = cy * sp * cr + sy * cp * sr;
        q.y = sy * cp * cr - cy * sp * sr;
        q.z = cy * cp * sr - sy * sp * cr;

        return q;
    }

    Quaternion Quaternion::FromEulerAngles(const Vector3& euler) {
        return FromEulerAngles(euler.x, euler.y, euler.z);
    }

    float Quaternion::Dot(const Quaternion& a, const Quaternion& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }

    // Spherical linear interpolation
    Quaternion Quaternion::Slerp(const Quaternion& a, const Quaternion& b, float t) {
        Quaternion q1 = a.Normalized();
        Quaternion q2 = b.Normalized();

        float dot = Dot(q1, q2);

        // If dot < 0, negate q2 to take shorter path
        if (dot < 0.0f) {
            q2 = q2 * -1.0f;
            dot = -dot;
        }

        // If quaternions are very close, use linear interpolation
        if (dot > 0.9995f) {
            return Lerp(q1, q2, t);
        }

        float theta = std::acos(dot);
        float sinTheta = std::sin(theta);
        float wa = std::sin((1.0f - t) * theta) / sinTheta;
        float wb = std::sin(t * theta) / sinTheta;

        return q1 * wa + q2 * wb;
    }

    // Linear interpolation
    Quaternion Quaternion::Lerp(const Quaternion& a, const Quaternion& b, float t) {
        Quaternion result = a * (1.0f - t) + b * t;
        result.Normalize();
        return result;
    }

    // Create rotation from forward direction
    Quaternion Quaternion::LookRotation(const Vector3& forward, const Vector3& up) {
        Vector3 f = forward.Normalized();
        Vector3 r = Vector3::Cross(up, f).Normalized();
        Vector3 u = Vector3::Cross(f, r);

        // Build matrix with basis vectors as COLUMNS (for FromMatrix)
        Matrix4x4 m = Matrix4x4::Identity;
        m.m[0][0] = r.x; m.m[1][0] = r.y; m.m[2][0] = r.z;
        m.m[0][1] = u.x; m.m[1][1] = u.y; m.m[2][1] = u.z;
        m.m[0][2] = f.x; m.m[1][2] = f.y; m.m[2][2] = f.z;

        return FromMatrix(m);
    }

    Quaternion Quaternion::FromMatrix(const Matrix4x4& m) {
        Quaternion q;

        float trace = m.m[0][0] + m.m[1][1] + m.m[2][2];

        if (trace > 0.0f) {
            float s = std::sqrt(trace + 1.0f) * 2.0f;
            q.w = 0.25f * s;
            q.x = (m.m[2][1] - m.m[1][2]) / s;
            q.y = (m.m[0][2] - m.m[2][0]) / s;
            q.z = (m.m[1][0] - m.m[0][1]) / s;
        } else if (m.m[0][0] > m.m[1][1] && m.m[0][0] > m.m[2][2]) {
            float s = std::sqrt(1.0f + m.m[0][0] - m.m[1][1] - m.m[2][2]) * 2.0f;
            q.w = (m.m[2][1] - m.m[1][2]) / s;
            q.x = 0.25f * s;
            q.y = (m.m[0][1] + m.m[1][0]) / s;
            q.z = (m.m[0][2] + m.m[2][0]) / s;
        } else if (m.m[1][1] > m.m[2][2]) {
            float s = std::sqrt(1.0f + m.m[1][1] - m.m[0][0] - m.m[2][2]) * 2.0f;
            q.w = (m.m[0][2] - m.m[2][0]) / s;
            q.x = (m.m[0][1] + m.m[1][0]) / s;
            q.y = 0.25f * s;
            q.z = (m.m[1][2] + m.m[2][1]) / s;
        } else {
            float s = std::sqrt(1.0f + m.m[2][2] - m.m[0][0] - m.m[1][1]) * 2.0f;
            q.w = (m.m[1][0] - m.m[0][1]) / s;
            q.x = (m.m[0][2] + m.m[2][0]) / s;
            q.y = (m.m[1][2] + m.m[2][1]) / s;
            q.z = 0.25f * s;
        }

        q.Normalize();
        return q;
    }

    // Non-member operator
    Quaternion operator*(float scalar, const Quaternion& q) {
        return q * scalar;
    }
}