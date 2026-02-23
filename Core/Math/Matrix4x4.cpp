//
// Created by Marvin on 28/01/2026.
//

#include "Matrix4x4.h"
#include <Core/Math/Vector3.h>
#include "MathTypes.h"
#include <cstring>
#include <cmath>

namespace Math {
    // Static members
    const Matrix4x4 Matrix4x4::Identity = Matrix4x4(1.0f);
    const Matrix4x4 Matrix4x4::Zero = Matrix4x4(0.0f);

    // Constructors
    Matrix4x4::Matrix4x4() {
        std::memset(m, 0, sizeof(m));
    }

    Matrix4x4::Matrix4x4(float diagonal) {
        std::memset(m, 0, sizeof(m));
        m[0][0] = diagonal;
        m[1][1] = diagonal;
        m[2][2] = diagonal;
        m[3][3] = diagonal;
    }

    // Matrix multiplication
    Matrix4x4 Matrix4x4::operator*(const Matrix4x4& other) const {
        Matrix4x4 result;

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result.m[i][j] =
                    m[i][0] * other.m[0][j] +
                    m[i][1] * other.m[1][j] +
                    m[i][2] * other.m[2][j] +
                    m[i][3] * other.m[3][j];
            }
        }

        return result;
    }

    // Matrix-vector multiplication
    Vector4 Matrix4x4::operator*(const Vector4& v) const {
        return Vector4(
            m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * v.w,
            m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * v.w,
            m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * v.w,
            m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] * v.w
        );
    }

    Matrix4x4& Matrix4x4::operator*=(const Matrix4x4& other) {
        *this = *this * other;
        return *this;
    }

    // Translation matrix
    Matrix4x4 Matrix4x4::Translation(const Vector3& position) {
        Matrix4x4 m;

        m.m[0][0] = 1.0f; m.m[0][1] = 0.0f; m.m[0][2] = 0.0f; m.m[0][3] = 0.0f;
        m.m[1][0] = 0.0f; m.m[1][1] = 1.0f; m.m[1][2] = 0.0f; m.m[1][3] = 0.0f;
        m.m[2][0] = 0.0f; m.m[2][1] = 0.0f; m.m[2][2] = 1.0f; m.m[2][3] = 0.0f;
        m.m[3][0] = position.x;
        m.m[3][1] = position.y;
        m.m[3][2] = position.z;
        m.m[3][3] = 1.0f;

        return m;
    }

    // Rotation around arbitrary axis
    Matrix4x4 Matrix4x4::Rotation(const Vector3& axis, float angle) {
        Matrix4x4 result = Identity;

        Vector3 a = axis.Normalized();
        float c = std::cos(angle);
        float s = std::sin(angle);
        float t = 1.0f - c;

        result.m[0][0] = t * a.x * a.x + c;
        result.m[0][1] = t * a.x * a.y - s * a.z;
        result.m[0][2] = t * a.x * a.z + s * a.y;

        result.m[1][0] = t * a.x * a.y + s * a.z;
        result.m[1][1] = t * a.y * a.y + c;
        result.m[1][2] = t * a.y * a.z - s * a.x;

        result.m[2][0] = t * a.x * a.z - s * a.y;
        result.m[2][1] = t * a.y * a.z + s * a.x;
        result.m[2][2] = t * a.z * a.z + c;

        return result;
    }

    // Rotation around X axis
    Matrix4x4 Matrix4x4::RotationX(float angle) {
        Matrix4x4 result = Identity;

        float c = std::cos(angle);
        float s = std::sin(angle);

        result.m[1][1] = c;
        result.m[1][2] = -s;
        result.m[2][1] = s;
        result.m[2][2] = c;

        return result;
    }

    // Rotation around Y axis
    Matrix4x4 Matrix4x4::RotationY(float angle) {
        Matrix4x4 result = Identity;

        float c = std::cos(angle);
        float s = std::sin(angle);

        result.m[0][0] = c;
        result.m[0][2] = s;
        result.m[2][0] = -s;
        result.m[2][2] = c;

        return result;
    }

    // Rotation around Z axis
    Matrix4x4 Matrix4x4::RotationZ(float angle) {
        Matrix4x4 result = Identity;

        float c = std::cos(angle);
        float s = std::sin(angle);

        result.m[0][0] = c;
        result.m[0][1] = -s;
        result.m[1][0] = s;
        result.m[1][1] = c;

        return result;
    }

    // Scale matrix
    Matrix4x4 Matrix4x4::Scale(const Vector3& scale) {
        Matrix4x4 result = Identity;
        result.m[0][0] = scale.x;
        result.m[1][1] = scale.y;
        result.m[2][2] = scale.z;

        return result;
    }

    Matrix4x4 Matrix4x4::Scale(float uniformScale) {
        return Scale(Vector3(uniformScale, uniformScale, uniformScale));
    }

    // Look-at matrix (view matrix) for row-vector convention (v * M)
    Matrix4x4 Matrix4x4::LookAt(const Vector3& eye, const Vector3& target, const Vector3& up) {
        Vector3 forward = (target - eye).Normalized();
        Vector3 right = Vector3::Cross(up, forward).Normalized();
        Vector3 newUp = Vector3::Cross(forward, right);

        Matrix4x4 result;
        std::memset(&result, 0, sizeof(Matrix4x4));

        // Basis vectors as COLUMNS (for row-vector multiplication)
        // Column 0 = right
        result.m[0][0] = right.x;
        result.m[1][0] = right.y;
        result.m[2][0] = right.z;

        // Column 1 = up
        result.m[0][1] = newUp.x;
        result.m[1][1] = newUp.y;
        result.m[2][1] = newUp.z;

        // Column 2 = forward
        result.m[0][2] = forward.x;
        result.m[1][2] = forward.y;
        result.m[2][2] = forward.z;

        // Translation in row 3
        result.m[3][0] = -Vector3::Dot(right, eye);
        result.m[3][1] = -Vector3::Dot(newUp, eye);
        result.m[3][2] = -Vector3::Dot(forward, eye);

        result.m[3][3] = 1.0f;

        return result;
    }

    // Perspective projection matrix
    Matrix4x4 Matrix4x4::Perspective(float fov, float aspect, float near, float far) {
        Matrix4x4 result;
        std::memset(&result, 0, sizeof(Matrix4x4));

        float tanHalfFov = std::tan(fov * 0.5f);
        float range = far / (far - near);

        result.m[0][0] = 1.0f / (aspect * tanHalfFov);
        result.m[1][1] = 1.0f / tanHalfFov;
        result.m[2][2] = range;
        result.m[2][3] = 1.0f;
        result.m[3][2] = -range * near;
        result.m[3][3] = 0.0f;

        return result;
    }

    // Orthographic projection matrix (DirectX style, depth [0,1], left-handed)
    Matrix4x4 Matrix4x4::Orthographic(float left, float right, float bottom, float top, float near, float far) {
        Matrix4x4 result = Identity;

        result.m[0][0] = 2.0f / (right - left);
        result.m[1][1] = 2.0f / (top - bottom);
        result.m[2][2] = 1.0f / (far - near);  // DirectX [0,1] depth range

        // Translation in last row (row-vector convention like LookAt)
        result.m[3][0] = -(right + left) / (right - left);
        result.m[3][1] = -(top + bottom) / (top - bottom);
        result.m[3][2] = -near / (far - near);  // DirectX depth offset

        return result;
    }

    // Transpose
    Matrix4x4 Matrix4x4::Transposed() const {
        Matrix4x4 result;

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result.m[i][j] = m[j][i];
            }
        }

        return result;
    }

    // Determinant
    float Matrix4x4::Determinant() const {
        // Using cofactor expansion along first row
        float det = 0.0f;

        // 3x3 sub-determinants
        float sub00 = m[1][1] * (m[2][2] * m[3][3] - m[2][3] * m[3][2]) -
                      m[1][2] * (m[2][1] * m[3][3] - m[2][3] * m[3][1]) +
                      m[1][3] * (m[2][1] * m[3][2] - m[2][2] * m[3][1]);

        float sub01 = m[1][0] * (m[2][2] * m[3][3] - m[2][3] * m[3][2]) -
                      m[1][2] * (m[2][0] * m[3][3] - m[2][3] * m[3][0]) +
                      m[1][3] * (m[2][0] * m[3][2] - m[2][2] * m[3][0]);

        float sub02 = m[1][0] * (m[2][1] * m[3][3] - m[2][3] * m[3][1]) -
                      m[1][1] * (m[2][0] * m[3][3] - m[2][3] * m[3][0]) +
                      m[1][3] * (m[2][0] * m[3][1] - m[2][1] * m[3][0]);

        float sub03 = m[1][0] * (m[2][1] * m[3][2] - m[2][2] * m[3][1]) -
                      m[1][1] * (m[2][0] * m[3][2] - m[2][2] * m[3][0]) +
                      m[1][2] * (m[2][0] * m[3][1] - m[2][1] * m[3][0]);

        det = m[0][0] * sub00 - m[0][1] * sub01 + m[0][2] * sub02 - m[0][3] * sub03;

        return det;
    }

    // Inverse (using Gauss-Jordan elimination)
    Matrix4x4 Matrix4x4::Inverted() const {
      float det = Determinant();

    if (std::abs(det) < EPSILON) {
        return Identity;
    }

    float invDet = 1.0f / det;
    Matrix4x4 result;

    // Row 0
    result.m[0][0] = (m[1][1] * (m[2][2] * m[3][3] - m[2][3] * m[3][2]) -
                      m[1][2] * (m[2][1] * m[3][3] - m[2][3] * m[3][1]) +
                      m[1][3] * (m[2][1] * m[3][2] - m[2][2] * m[3][1])) * invDet;

    result.m[0][1] = -(m[0][1] * (m[2][2] * m[3][3] - m[2][3] * m[3][2]) -
                       m[0][2] * (m[2][1] * m[3][3] - m[2][3] * m[3][1]) +
                       m[0][3] * (m[2][1] * m[3][2] - m[2][2] * m[3][1])) * invDet;

    result.m[0][2] = (m[0][1] * (m[1][2] * m[3][3] - m[1][3] * m[3][2]) -
                      m[0][2] * (m[1][1] * m[3][3] - m[1][3] * m[3][1]) +
                      m[0][3] * (m[1][1] * m[3][2] - m[1][2] * m[3][1])) * invDet;

    result.m[0][3] = -(m[0][1] * (m[1][2] * m[2][3] - m[1][3] * m[2][2]) -
                       m[0][2] * (m[1][1] * m[2][3] - m[1][3] * m[2][1]) +
                       m[0][3] * (m[1][1] * m[2][2] - m[1][2] * m[2][1])) * invDet;

    // Row 1
    result.m[1][0] = -(m[1][0] * (m[2][2] * m[3][3] - m[2][3] * m[3][2]) -
                       m[1][2] * (m[2][0] * m[3][3] - m[2][3] * m[3][0]) +
                       m[1][3] * (m[2][0] * m[3][2] - m[2][2] * m[3][0])) * invDet;

    result.m[1][1] = (m[0][0] * (m[2][2] * m[3][3] - m[2][3] * m[3][2]) -
                      m[0][2] * (m[2][0] * m[3][3] - m[2][3] * m[3][0]) +
                      m[0][3] * (m[2][0] * m[3][2] - m[2][2] * m[3][0])) * invDet;

    result.m[1][2] = -(m[0][0] * (m[1][2] * m[3][3] - m[1][3] * m[3][2]) -
                       m[0][2] * (m[1][0] * m[3][3] - m[1][3] * m[3][0]) +
                       m[0][3] * (m[1][0] * m[3][2] - m[1][2] * m[3][0])) * invDet;

    result.m[1][3] = (m[0][0] * (m[1][2] * m[2][3] - m[1][3] * m[2][2]) -
                      m[0][2] * (m[1][0] * m[2][3] - m[1][3] * m[2][0]) +
                      m[0][3] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])) * invDet;

    // Row 2
    result.m[2][0] = (m[1][0] * (m[2][1] * m[3][3] - m[2][3] * m[3][1]) -
                      m[1][1] * (m[2][0] * m[3][3] - m[2][3] * m[3][0]) +
                      m[1][3] * (m[2][0] * m[3][1] - m[2][1] * m[3][0])) * invDet;

    result.m[2][1] = -(m[0][0] * (m[2][1] * m[3][3] - m[2][3] * m[3][1]) -
                       m[0][1] * (m[2][0] * m[3][3] - m[2][3] * m[3][0]) +
                       m[0][3] * (m[2][0] * m[3][1] - m[2][1] * m[3][0])) * invDet;

    result.m[2][2] = (m[0][0] * (m[1][1] * m[3][3] - m[1][3] * m[3][1]) -
                      m[0][1] * (m[1][0] * m[3][3] - m[1][3] * m[3][0]) +
                      m[0][3] * (m[1][0] * m[3][1] - m[1][1] * m[3][0])) * invDet;

    result.m[2][3] = -(m[0][0] * (m[1][1] * m[2][3] - m[1][3] * m[2][1]) -
                       m[0][1] * (m[1][0] * m[2][3] - m[1][3] * m[2][0]) +
                       m[0][3] * (m[1][0] * m[2][1] - m[1][1] * m[2][0])) * invDet;

    // Row 3
    result.m[3][0] = -(m[1][0] * (m[2][1] * m[3][2] - m[2][2] * m[3][1]) -
                       m[1][1] * (m[2][0] * m[3][2] - m[2][2] * m[3][0]) +
                       m[1][2] * (m[2][0] * m[3][1] - m[2][1] * m[3][0])) * invDet;

    result.m[3][1] = (m[0][0] * (m[2][1] * m[3][2] - m[2][2] * m[3][1]) -
                      m[0][1] * (m[2][0] * m[3][2] - m[2][2] * m[3][0]) +
                      m[0][2] * (m[2][0] * m[3][1] - m[2][1] * m[3][0])) * invDet;

    result.m[3][2] = -(m[0][0] * (m[1][1] * m[3][2] - m[1][2] * m[3][1]) -
                       m[0][1] * (m[1][0] * m[3][2] - m[1][2] * m[3][0]) +
                       m[0][2] * (m[1][0] * m[3][1] - m[1][1] * m[3][0])) * invDet;

    result.m[3][3] = (m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
                      m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
                      m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0])) * invDet;

    return result;
    }
}