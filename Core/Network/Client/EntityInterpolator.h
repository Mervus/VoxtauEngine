//
// Created by Marvin on 27/02/2026.
//

#ifndef VOXTAU_ENTITYINTERPOLATOR_H
#define VOXTAU_ENTITYINTERPOLATOR_H
#pragma once

#include <unordered_map>
#include <cstdint>
#include "Core/Entity/EntityID.h"
#include "Core/Math/Vector3.h"
#include "Core/Network/Protocol/ServerSnapshot.h"

// Buffers server snapshots for remote entities and interpolates between them.
// Entities are always rendered slightly in the past (interpolation delay)
// to ensure we always have two data points to interpolate between.
class EntityInterpolator {
public:
    EntityInterpolator();
    ~EntityInterpolator();

    // Configuration

    // How far behind real-time we render remote entities.
    // Must be >= one snapshot interval. Default 66ms (two 30Hz snapshots).
    void SetInterpolationDelay(float seconds) { _interpDelay = seconds; }

    //  Per-tick 

    // Advance interpolation time. Call once per frame.
    void Update(float deltaTime);

    //  Data input (from ServerSnapshot) 

    // Push a new state for an entity from a server snapshot.
    // Timestamp is the server time (or tick converted to time).
    void PushState(EntityID id, float timestamp, const ReplicatedEntityState& state);

    // Entity left our area of interest — stop tracking.
    void RemoveEntity(EntityID id);

    //  Interpolated state (for rendering) 

    [[nodiscard]] Math::Vector3 GetPosition(EntityID id) const;
    [[nodiscard]] Math::Vector3 GetVelocity(EntityID id) const;
    [[nodiscard]] float GetYaw(EntityID id) const;
    [[nodiscard]] float GetHealth(EntityID id) const;
    [[nodiscard]] bool HasEntity(EntityID id) const;

private:
    // Per-entity: we keep a small buffer of recent snapshots
    // and interpolate between the two surrounding our render time.
    struct SnapshotEntry {
        float timestamp = 0.0f;
        ReplicatedEntityState state;
    };

    struct EntityBuffer {
        // Ring buffer of recent snapshots (oldest first)
        static constexpr size_t MAX_ENTRIES = 8;
        SnapshotEntry entries[MAX_ENTRIES];
        size_t count = 0;
        size_t head = 0;  // next write position

        void Push(float timestamp, const ReplicatedEntityState& state);

        // Find the two entries surrounding renderTime.
        // Returns false if we don't have enough data.
        bool FindBracket(float renderTime,
                         const SnapshotEntry*& before,
                         const SnapshotEntry*& after) const;
    };

    std::unordered_map<EntityID, EntityBuffer> _entities;

    float _currentTime = 0.0f;
    float _interpDelay = 0.066f; // 66ms default

    float GetRenderTime() const { return _currentTime - _interpDelay; }

    // Lerp helper
    static Math::Vector3 Lerp(const Math::Vector3& a, const Math::Vector3& b, float t);
    static float LerpAngle(float a, float b, float t);
};

#endif //VOXTAU_ENTITYINTERPOLATOR_H