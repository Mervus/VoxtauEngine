//
// Created by Claude on 02/02/2026.
//

#include "Frustum.h"
#include <cmath>

namespace Math {

void Frustum::NormalizePlane(Vector4& plane)
{
    float length = sqrtf(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
    if (length > 0.0001f) {
        plane.x /= length;
        plane.y /= length;
        plane.z /= length;
        plane.w /= length;
    }
}

void Frustum::ExtractFromMatrix(const Matrix4x4& vp)
{
    // Gribb/Hartmann method for extracting frustum planes
    // Matrix is row-major: m[row][column]

    // Left plane: row3 + row0
    planes[Left].x = vp.m[0][3] + vp.m[0][0];
    planes[Left].y = vp.m[1][3] + vp.m[1][0];
    planes[Left].z = vp.m[2][3] + vp.m[2][0];
    planes[Left].w = vp.m[3][3] + vp.m[3][0];
    NormalizePlane(planes[Left]);

    // Right plane: row3 - row0
    planes[Right].x = vp.m[0][3] - vp.m[0][0];
    planes[Right].y = vp.m[1][3] - vp.m[1][0];
    planes[Right].z = vp.m[2][3] - vp.m[2][0];
    planes[Right].w = vp.m[3][3] - vp.m[3][0];
    NormalizePlane(planes[Right]);

    // Bottom plane: row3 + row1
    planes[Bottom].x = vp.m[0][3] + vp.m[0][1];
    planes[Bottom].y = vp.m[1][3] + vp.m[1][1];
    planes[Bottom].z = vp.m[2][3] + vp.m[2][1];
    planes[Bottom].w = vp.m[3][3] + vp.m[3][1];
    NormalizePlane(planes[Bottom]);

    // Top plane: row3 - row1
    planes[Top].x = vp.m[0][3] - vp.m[0][1];
    planes[Top].y = vp.m[1][3] - vp.m[1][1];
    planes[Top].z = vp.m[2][3] - vp.m[2][1];
    planes[Top].w = vp.m[3][3] - vp.m[3][1];
    NormalizePlane(planes[Top]);

    // Near plane: row3 + row2
    planes[Near].x = vp.m[0][3] + vp.m[0][2];
    planes[Near].y = vp.m[1][3] + vp.m[1][2];
    planes[Near].z = vp.m[2][3] + vp.m[2][2];
    planes[Near].w = vp.m[3][3] + vp.m[3][2];
    NormalizePlane(planes[Near]);

    // Far plane: row3 - row2
    planes[Far].x = vp.m[0][3] - vp.m[0][2];
    planes[Far].y = vp.m[1][3] - vp.m[1][2];
    planes[Far].z = vp.m[2][3] - vp.m[2][2];
    planes[Far].w = vp.m[3][3] - vp.m[3][2];
    NormalizePlane(planes[Far]);
}

bool Frustum::ContainsPoint(const Vector3& point) const
{
    for (int i = 0; i < PlaneCount; ++i) {
        float distance = planes[i].x * point.x +
                         planes[i].y * point.y +
                         planes[i].z * point.z +
                         planes[i].w;
        if (distance < 0) {
            return false;
        }
    }
    return true;
}

bool Frustum::ContainsAABB(const AABB& box) const
{
    for (int i = 0; i < PlaneCount; ++i) {
        const Vector4& plane = planes[i];

        // Find the positive vertex (furthest along plane normal)
        Vector3 pVertex;
        pVertex.x = (plane.x >= 0) ? box.max.x : box.min.x;
        pVertex.y = (plane.y >= 0) ? box.max.y : box.min.y;
        pVertex.z = (plane.z >= 0) ? box.max.z : box.min.z;

        // If positive vertex is outside, the entire box is outside
        float distance = plane.x * pVertex.x +
                         plane.y * pVertex.y +
                         plane.z * pVertex.z +
                         plane.w;

        if (distance < 0) {
            return false;
        }
    }
    return true;
}

bool Frustum::ContainsSphere(const Vector3& center, float radius) const
{
    for (int i = 0; i < PlaneCount; ++i) {
        float distance = planes[i].x * center.x +
                         planes[i].y * center.y +
                         planes[i].z * center.z +
                         planes[i].w;
        if (distance < -radius) {
            return false;
        }
    }
    return true;
}

} // namespace Math
