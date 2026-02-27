//
// Created by Marvin on 27/02/2026.
//

#ifndef VOXTAU_ENTITYREPLICATOR_H
#define VOXTAU_ENTITYREPLICATOR_H
#pragma once

#include <unordered_map>
#include <vector>
#include <cstdint>
#include "Core/Entity/EntityID.h"
#include "Core/Entity/EntityType.h"
#include "Core/Network/Protocol/ServerSnapshot.h"

class EntityManager;
class Entity;
class LivingEntity;

using ConnectionID = uint32_t;

// Dirty flag bits — one per replicated field in ReplicatedEntityState.
// Used for delta compression: only send fields that changed since the
// client's last acknowledged baseline.
namespace DirtyFlag {
    constexpr uint32_t Position      = 1 << 0;
    constexpr uint32_t Velocity      = 1 << 1;
    constexpr uint32_t Yaw           = 1 << 2;
    constexpr uint32_t Health        = 1 << 3;
    constexpr uint32_t MaxHealth     = 1 << 4;
    constexpr uint32_t IsDead        = 1 << 5;
    constexpr uint32_t MovementState = 1 << 6;
    constexpr uint32_t AnimationId   = 1 << 7;
    constexpr uint32_t All           = 0xFFFFFFFF;
}

class EntityReplicator {
public:
    EntityReplicator();
    ~EntityReplicator();

    // Per-tick: build snapshots

    // Build the entity portion of a ServerSnapshot for a specific client.
    // Compares current entity state against that client's baseline,
    // only includes entities within AOI, marks dirty flags per entity.
    void BuildSnapshot(ConnectionID client,
                       const EntityManager* entityManager,
                       EntityID excludeEntity,          // client's own player
                       const Math::Vector3& viewCenter, // client's position (for AOI)
                       float viewRadius,                // how far they can see
                       std::vector<ReplicatedEntityState>& outEntities);

    // Baseline management

    // Client acknowledged receiving snapshot at this tick.
    // Promote pending baselines to confirmed for this client.
    void AcknowledgeTick(ConnectionID client, uint32_t tick);

    // Client disconnected — clean up all their baselines.
    void RemoveClient(ConnectionID client);

    // Entity destroyed — remove from all client baselines.
    void OnEntityDestroyed(EntityID entity);

private:
    // Per-client, per-entity: the last state the client acknowledged receiving.
    // We diff against this to determine what's dirty.
    struct ClientBaselines {
        std::unordered_map<EntityID, ReplicatedEntityState> confirmed;
        // Pending: sent but not yet acked. Promoted on ack.
        std::unordered_map<EntityID, ReplicatedEntityState> pending;
        uint32_t pendingTick = 0;
    };

    std::unordered_map<ConnectionID, ClientBaselines> _clientData;

    // Internal

    // Snapshot the current state of an entity into a ReplicatedEntityState.
    ReplicatedEntityState CaptureEntityState(const Entity* entity) const;

    // Compare two states and return a dirty bitmask.
    uint32_t ComputeDirtyFlags(const ReplicatedEntityState& baseline,
                                const ReplicatedEntityState& current) const;
};

#endif //VOXTAU_ENTITYREPLICATOR_H