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

void EntityInterpolator::PushState(EntityID id, float timestamp, Math::Vector3 position, Math::Vector3 velocity, Math::Quaternion rotation) {
    auto& buffer = _entities[id];
    auto& entry = buffer.entries[buffer.head];
    entry.timestamp = timestamp;
    entry.position = position;
    entry.velocity = velocity;
    entry.rotation = rotation;
    buffer.head = (buffer.head + 1) % EntityBuffer::MAX_ENTRIES;
    if (buffer.count < EntityBuffer::MAX_ENTRIES) buffer.count++;
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
            return it->second.entries[latest].position;
        }
        return Math::Vector3();
    }

    // Interpolation factor: how far between the two snapshots
    float range = after->timestamp - before->timestamp;
    float t = (range > 0.0001f)
        ? (renderTime - before->timestamp) / range
        : 0.0f;
    t = std::clamp(t, 0.0f, 1.0f);

    return Lerp(before->position, after->position, t);
}

Math::Vector3 EntityInterpolator::GetVelocity(EntityID id) const {
    auto it = _entities.find(id);
    if (it == _entities.end()) return Math::Vector3();

    const SnapshotEntry* before = nullptr;
    const SnapshotEntry* after = nullptr;
    float renderTime = GetRenderTime();

    if (!it->second.FindBracket(renderTime, before, after)) {
        // Return latest known velocity instead of zero
        if (it->second.count > 0) {
            size_t latest = (it->second.head + it->second.MAX_ENTRIES - 1) % it->second.MAX_ENTRIES;
            return it->second.entries[latest].velocity;
        }
        return Math::Vector3();
    }

    float range = after->timestamp - before->timestamp;
    float t = (range > 0.0001f)
        ? (renderTime - before->timestamp) / range
        : 0.0f;
    t = std::clamp(t, 0.0f, 1.0f);

    return Lerp(before->velocity, after->velocity, t);
}

Math::Quaternion EntityInterpolator::GetRotation(EntityID id) const {
    auto it = _entities.find(id);
    if (it == _entities.end()) return Math::Quaternion::Identity;

    const SnapshotEntry* before = nullptr;
    const SnapshotEntry* after = nullptr;
    float renderTime = GetRenderTime();

    if (!it->second.FindBracket(renderTime, before, after)) {
        if (it->second.count > 0) {
            size_t latest = (it->second.head + it->second.MAX_ENTRIES - 1)
                            % it->second.MAX_ENTRIES;
            return it->second.entries[latest].rotation;
        }
        return Math::Quaternion::Identity;
    }

    float range = after->timestamp - before->timestamp;
    float t = (range > 0.0001f)
        ? (renderTime - before->timestamp) / range
        : 0.0f;
    t = std::clamp(t, 0.0f, 1.0f);

    return Math::Quaternion::Slerp(before->rotation, after->rotation, t);
}

bool EntityInterpolator::EntityBuffer::FindBracket(float renderTime,
                                                    const SnapshotEntry*& before,
                                                    const SnapshotEntry*& after) const {
    if (count < 2) return false;

    // Walk the ring buffer in chronological order and find the two entries
    // that straddle renderTime (before.timestamp <= renderTime <= after.timestamp).
    size_t oldest = (head + MAX_ENTRIES - count) % MAX_ENTRIES;

    for (size_t i = 0; i + 1 < count; ++i) {
        size_t idxA = (oldest + i)     % MAX_ENTRIES;
        size_t idxB = (oldest + i + 1) % MAX_ENTRIES;

        if (entries[idxA].timestamp <= renderTime &&
            renderTime <= entries[idxB].timestamp) {
            before = &entries[idxA];
            after  = &entries[idxB];
            return true;
        }
    }

    return false;
}

bool EntityInterpolator::HasEntity(EntityID id) const {
    return _entities.find(id) != _entities.end();
}

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