//
// Created by Marvin on 27/02/2026.
//

#ifndef VOXTAU_SERVERSNAPSHOT_H
#define VOXTAU_SERVERSNAPSHOT_H
#pragma once

#include <vector>
#include <cstdint>
#include "Core/Entity/EntityID.h"
#include "Core/Entity/EntityType.h"
#include "Core/Math/Vector3.h"

// Per-entity state included in a snapshot
// Only contains fields that change frequently and matter for rendering/gameplay
struct ReplicatedEntityState {
    EntityID id;
    EntityType type = EntityType::None;

    // Transform
    Math::Vector3 position;
    Math::Vector3 velocity;
    float yaw = 0.0f;

    // LivingEntity state
    float health = 0.0f;
    float maxHealth = 0.0f;
    bool isDead = false;

    // Player-specific
    uint8_t movementState = 0;  // MovementState enum cast

    // Animation (client uses this to pick the right animation)
    uint8_t animationId = 0;

    // Dirty flags — which fields changed since the client's last acked baseline
    // Used by EntityReplicator for delta compression, not sent on the wire
    uint32_t dirtyFlags = 0xFFFFFFFF;  // all dirty by default (full state)
};

// What the server sends to ONE specific client each network tick
struct ServerSnapshot {
    // Header
    uint32_t serverTick = 0;              // which simulation tick this represents
    uint32_t lastProcessedInput = 0;      // "I've consumed your inputs up to this tick"
    uint32_t worldStateVersion = 0;       // block-change counter (for reconciliation sync)

    // Local player authoritative state
    // This is what the CLIENT compares against its prediction
    Math::Vector3 playerPosition;
    Math::Vector3 playerVelocity;
    bool playerOnGround = false;

    // Visible entities
    // Only entities within this client's area of interest
    // Excludes the client's own player (that's above)
    std::vector<ReplicatedEntityState> entities;

    // Entity lifecycle events (reliable, sent separately)
    // These would normally be on a reliable channel, not in the snapshot.
    // Listed here for completeness — in practice they're separate packets.
    // std::vector<EntityID> spawned;
    // std::vector<EntityID> despawned;
};
#endif //VOXTAU_SERVERSNAPSHOT_H