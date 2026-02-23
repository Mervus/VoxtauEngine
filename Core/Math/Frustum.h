//
// Created by Claude on 02/02/2026.
//

#ifndef VOXTAU_FRUSTUM_H
#define VOXTAU_FRUSTUM_H
#pragma once

#include <EngineApi.h>
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4x4.h"

namespace Math {

// Axis-Aligned Bounding Box
struct ENGINE_API AABB {
    Vector3 min;
    Vector3 max;

    AABB() : min(0, 0, 0), max(0, 0, 0) {}
    AABB(const Vector3& min, const Vector3& max) : min(min), max(max) {}

    Vector3 GetCenter() const { return (min + max) * 0.5f; }
    Vector3 GetExtents() const { return (max - min) * 0.5f; }
};

class ENGINE_API Frustum {
public:
    enum Plane {
        Left = 0,
        Right,
        Bottom,
        Top,
        Near,
        Far,
        PlaneCount
    };

    Frustum() = default;

    // Extract frustum planes from view-projection matrix
    void ExtractFromMatrix(const Matrix4x4& viewProjection);

    // Test if a point is inside the frustum
    bool ContainsPoint(const Vector3& point) const;

    // Test if an AABB intersects or is inside the frustum
    bool ContainsAABB(const AABB& box) const;

    // Test if a sphere intersects or is inside the frustum
    bool ContainsSphere(const Vector3& center, float radius) const;

    // Copy frustum planes out for GPU upload
    void GetPlanes(Vector4 outPlanes[6]) const {
        for (int i = 0; i < PlaneCount; ++i) outPlanes[i] = planes[i];
    }

private:
    // Plane equation: ax + by + cz + d = 0
    // Stored as Vector4(a, b, c, d) - normalized so (a,b,c) is unit length
    Vector4 planes[PlaneCount];

    void NormalizePlane(Vector4& plane);
};

} // namespace Math

#endif //VOXTAU_FRUSTUM_H
