//
// Created by Marvin on 11/02/2026.
//

#ifndef VOXTAU_AABB_H
#define VOXTAU_AABB_H
#pragma once

#include <EngineApi.h>
#include <Core/Math/Vector3.h>
#include <algorithm>
#include <cmath>
namespace Physics
{
    struct ENGINE_API AABB {
    Math::Vector3 min;
    Math::Vector3 max;

    AABB() : min(0, 0, 0), max(0, 0, 0) {}
    AABB(const Math::Vector3& min, const Math::Vector3& max) : min(min), max(max) {}

    // Create from center-bottom position and half extents
    // Position is at feet (center-bottom), halfExtents defines the half-width and half-depth,
    // and full height
    static AABB FromFeetAndSize(const Math::Vector3& feetPos, const Math::Vector3& halfExtents, float height) {
        return AABB(
            Math::Vector3(feetPos.x - halfExtents.x, feetPos.y, feetPos.z - halfExtents.z),
            Math::Vector3(feetPos.x + halfExtents.x, feetPos.y + height, feetPos.z + halfExtents.z)
        );
    }

    // Create AABB for a single block at integer coordinates
    static AABB FromBlock(int bx, int by, int bz) {
        return AABB(
            Math::Vector3(static_cast<float>(bx), static_cast<float>(by), static_cast<float>(bz)),
            Math::Vector3(static_cast<float>(bx + 1), static_cast<float>(by + 1), static_cast<float>(bz + 1))
        );
    }

    bool Overlaps(const AABB& other) const {
        return min.x < other.max.x && max.x > other.min.x &&
               min.y < other.max.y && max.y > other.min.y &&
               min.z < other.max.z && max.z > other.min.z;
    }

    // Expand this AABB by a delta vector (for sweep tests)
    // Returns an AABB that covers the full swept region
    AABB Expanded(const Math::Vector3& delta) const {
        AABB result = *this;
        if (delta.x > 0) result.max.x += delta.x; else result.min.x += delta.x;
        if (delta.y > 0) result.max.y += delta.y; else result.min.y += delta.y;
        if (delta.z > 0) result.max.z += delta.z; else result.min.z += delta.z;
        return result;
    }

    // Grow AABB by a uniform margin on all sides
    AABB Grown(float margin) const {
        return AABB(
            Math::Vector3(min.x - margin, min.y - margin, min.z - margin),
            Math::Vector3(max.x + margin, max.y + margin, max.z + margin)
        );
    }

    Math::Vector3 Center() const {
        return Math::Vector3(
            (min.x + max.x) * 0.5f,
            (min.y + max.y) * 0.5f,
            (min.z + max.z) * 0.5f
        );
    }

    Math::Vector3 Size() const {
        return Math::Vector3(max.x - min.x, max.y - min.y, max.z - min.z);
    }
};

}

#endif //VOXTAU_AABB_H