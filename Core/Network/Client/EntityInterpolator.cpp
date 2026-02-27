//
// Created by Marvin on 27/02/2026.
//

#include "EntityInterpolator.h"
#include <cmath>
#include <algorithm>

EntityInterpolator::EntityInterpolator() = default;
EntityInterpolator::~EntityInterpolator() = default;

// Per-tick

void EntityInterpolator::Update(float deltaTime) {
    _currentTime += deltaTime;
}

// Data Input

void EntityInterpolator::PushState(EntityID id, float timestamp,
                                    const ReplicatedEntityState& state) {
    _entities[id].Push(timestamp, state);
}

void EntityInterpolator::RemoveEntity(EntityID id) {
    _entities.erase(id);
}

// Interpolated State

Math::Vector3 EntityInterpolator::GetPosition(EntityID id) const {
    auto it = _entities.find(id);
    if (it == _entities.end()) return Math::Vector3();

    const SnapshotEntry* before = nullptr;
    const SnapshotEntry* after = nullptr;
    float renderTime = GetRenderTime();

    if (!it->second.FindBracket(renderTime, before, after)) {
        // Not enough data — return latest known position
        if (it->second.count > 0) {
            size_t latest = (it->second.head + it->second.MAX_ENTRIES - 1) % it->second.MAX_ENTRIES;
            return it->second.entries[latest].state.position;
        }
        return Math::Vector3();
    }

    // Interpolation factor: how far between the two snapshots
    float range = after->timestamp - before->timestamp;
    float t = (range > 0.0001f)
        ? (renderTime - before->timestamp) / range
        : 0.0f;
    t = std::clamp(t, 0.0f, 1.0f);

    return Lerp(before->state.position, after->state.position, t);
}

Math::Vector3 EntityInterpolator::GetVelocity(EntityID id) const {
    auto it = _entities.find(id);
    if (it == _entities.end()) return Math::Vector3();

    const SnapshotEntry* before = nullptr;
    const SnapshotEntry* after = nullptr;
    float renderTime = GetRenderTime();

    if (!it->second.FindBracket(renderTime, before, after)) {
        return Math::Vector3();
    }

    float range = after->timestamp - before->timestamp;
    float t = (range > 0.0001f)
        ? (renderTime - before->timestamp) / range
        : 0.0f;
    t = std::clamp(t, 0.0f, 1.0f);

    return Lerp(before->state.velocity, after->state.velocity, t);
}

float EntityInterpolator::GetYaw(EntityID id) const {
    auto it = _entities.find(id);
    if (it == _entities.end()) return 0.0f;

    const SnapshotEntry* before = nullptr;
    const SnapshotEntry* after = nullptr;
    float renderTime = GetRenderTime();

    if (!it->second.FindBracket(renderTime, before, after)) {
        if (it->second.count > 0) {
            size_t latest = (it->second.head + it->second.MAX_ENTRIES - 1) % it->second.MAX_ENTRIES;
            return it->second.entries[latest].state.yaw;
        }
        return 0.0f;
    }

    float range = after->timestamp - before->timestamp;
    float t = (range > 0.0001f)
        ? (renderTime - before->timestamp) / range
        : 0.0f;
    t = std::clamp(t, 0.0f, 1.0f);

    return LerpAngle(before->state.yaw, after->state.yaw, t);
}

float EntityInterpolator::GetHealth(EntityID id) const {
    // Health doesn't interpolate smoothly — snap to latest
    auto it = _entities.find(id);
    if (it == _entities.end()) return 0.0f;
    if (it->second.count == 0) return 0.0f;

    size_t latest = (it->second.head + it->second.MAX_ENTRIES - 1) % it->second.MAX_ENTRIES;
    return it->second.entries[latest].state.health;
}

bool EntityInterpolator::HasEntity(EntityID id) const {
    return _entities.find(id) != _entities.end();
}

//  EntityBuffer 

void EntityInterpolator::EntityBuffer::Push(float timestamp,
                                             const ReplicatedEntityState& state) {
    entries[head].timestamp = timestamp;
    entries[head].state = state;
    head = (head + 1) % MAX_ENTRIES;
    if (count < MAX_ENTRIES) count++;
}

bool EntityInterpolator::EntityBuffer::FindBracket(
        float renderTime,
        const SnapshotEntry*& before,
        const SnapshotEntry*& after) const {

    if (count < 2) return false;

    // Walk through entries chronologically to find the pair
    // that brackets renderTime: before.timestamp <= renderTime <= after.timestamp
    //
    // Entries are in ring buffer order. The oldest entry is at
    // (head - count) % MAX_ENTRIES, newest at (head - 1) % MAX_ENTRIES.

    size_t oldest = (head + MAX_ENTRIES - count) % MAX_ENTRIES;

    const SnapshotEntry* prev = nullptr;

    for (size_t i = 0; i < count; i++) {
        size_t idx = (oldest + i) % MAX_ENTRIES;
        const SnapshotEntry* current = &entries[idx];

        if (prev && prev->timestamp <= renderTime && current->timestamp >= renderTime) {
            before = prev;
            after = current;
            return true;
        }

        prev = current;
    }

    // renderTime is beyond our newest snapshot — extrapolation case.
    // Return the last two entries and let the caller clamp t to 1.0.
    if (prev && count >= 2) {
        size_t secondLast = (head + MAX_ENTRIES - 2) % MAX_ENTRIES;
        size_t last = (head + MAX_ENTRIES - 1) % MAX_ENTRIES;
        before = &entries[secondLast];
        after = &entries[last];
        return true;
    }

    return false;
}

//  Helpers 

Math::Vector3 EntityInterpolator::Lerp(const Math::Vector3& a,
                                        const Math::Vector3& b, float t) {
    return Math::Vector3(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    );
}

float EntityInterpolator::LerpAngle(float a, float b, float t) {
    // Handle wrapping around 2*PI
    constexpr float TWO_PI = 6.2831853f;
    float diff = b - a;

    // Normalize to [-PI, PI]
    while (diff > 3.1415927f) diff -= TWO_PI;
    while (diff < -3.1415927f) diff += TWO_PI;

    return a + diff * t;
}