//
// Created by Marvin on 11/02/2026.
//

#ifndef VOXTAU_VOXELBODY_H
#define VOXTAU_VOXELBODY_H
#pragma once

#include <EngineApi.h>
#include <Core/Math/Vector3.h>
#include "AABB.h"
#include <cstdint>
#include <functional>

// VoxelBodyID: Strong typedef for body identification
struct VoxelBodyID {
    uint32_t value;

    VoxelBodyID() : value(0) {}
    explicit VoxelBodyID(uint32_t v) : value(v) {}

    bool IsValid() const { return value != 0; }
    bool operator==(const VoxelBodyID& other) const { return value == other.value; }
    bool operator!=(const VoxelBodyID& other) const { return value != other.value; }

    static const VoxelBodyID Invalid;
};

inline const VoxelBodyID VoxelBodyID::Invalid = VoxelBodyID(0);

// Hash for use in unordered_map
struct VoxelBodyIDHash {
    std::size_t operator()(const VoxelBodyID& id) const {
        return std::hash<uint32_t>()(id.value);
    }
};

// --- VoxelBody ---

struct VoxelBody {
    // Identity
    VoxelBodyID id;

    // Spatial — position is at feet (center-bottom of AABB)
    Math::Vector3 position;
    float width  = 0.6f;   // Full width (X and Z diameter)
    float height = 1.8f;   // Full height (Y)

    // Velocity
    Math::Vector3 velocity;         // Persistent velocity (gravity accumulates here)
    Math::Vector3 inputVelocity;    // Movement wish for this tick (cleared after each step)

    // Collision results (set by VoxelPhysics::Step)
    bool grounded   = false;
    bool hitCeiling = false;
    bool hitWallX   = false;
    bool hitWallZ   = false;

    // Configuration
    float gravity       = -20.0f;   // Gravity acceleration (negative = down)
    float maxFallSpeed  = -50.0f;   // Terminal velocity (negative = down)
    float stepHeight    = 0.55f;    // Auto-step up ledges ≤ this height (slightly over half block)

    bool affectedByGravity = true;
    bool active            = true;  // Inactive bodies are skipped

    // Helpers
    float HalfWidth() const { return width * 0.5f; }

    // Get the world-space AABB for this body at its current position
    Physics::AABB GetAABB() const {
        float hw = HalfWidth();
        return Physics::AABB(
            Math::Vector3(position.x - hw, position.y, position.z - hw),
            Math::Vector3(position.x + hw, position.y + height, position.z + hw)
        );
    }

    // Get the world-space AABB at an arbitrary position
    Physics::AABB GetAABBAt(const Math::Vector3& pos) const {
        float hw = HalfWidth();
        return Physics::AABB(
            Math::Vector3(pos.x - hw, pos.y, pos.z - hw),
            Math::Vector3(pos.x + hw, pos.y + height, pos.z + hw)
        );
    }
};

#endif //VOXTAU_VOXELBODY_H