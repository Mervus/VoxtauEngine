//
// Created by Marvin on 27/02/2026.
//

#include "EntityReplicator.h"
#include "Core/Entity/Entity.h"
#include "Core/Entity/EntityManager.h"
#include "Core/Entity/Living/LivingEntity.h"
#include "Core/Entity/Living/Player/PlayerEntity.h"

#include <cmath>

// Thresholds for dirty detection
// Avoid marking fields dirty for trivial floating-point noise.
static constexpr float POSITION_EPSILON = 0.001f;   // ~1mm
static constexpr float VELOCITY_EPSILON = 0.01f;
static constexpr float YAW_EPSILON      = 0.01f;    // ~0.5 degrees
static constexpr float HEALTH_EPSILON   = 0.1f;

EntityReplicator::EntityReplicator() = default;
EntityReplicator::~EntityReplicator() = default;

// Snapshot Building

void EntityReplicator::BuildSnapshot(ConnectionID client,
                                      const EntityManager* entityManager,
                                      EntityID excludeEntity,
                                      const Math::Vector3& viewCenter,
                                      float viewRadius,
                                      std::vector<ReplicatedEntityState>& outEntities) {
    outEntities.clear();

    // Get or create baseline data for this client
    auto& baselines = _clientData[client];

    float viewRadiusSq = viewRadius * viewRadius;

    entityManager->ForEach([&](Entity* entity) {
        EntityID id = entity->GetID();

        // Skip the client's own player — their state goes in the snapshot header
        if (id == excludeEntity) return;

        // AOI check: is this entity within view distance?
        Math::Vector3 diff = entity->GetPosition() - viewCenter;
        float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        if (distSq > viewRadiusSq) return;

        // Capture current state
        ReplicatedEntityState current = CaptureEntityState(entity);

        // Compute dirty flags against this client's confirmed baseline
        auto baselineIt = baselines.confirmed.find(id);
        if (baselineIt != baselines.confirmed.end()) {
            current.dirtyFlags = ComputeDirtyFlags(baselineIt->second, current);

            // Nothing changed — skip this entity entirely
            if (current.dirtyFlags == 0) return;
        } else {
            // No baseline = client has never seen this entity
            // Mark all dirty (full state send)
            current.dirtyFlags = DirtyFlag::All;
        }

        outEntities.push_back(current);

        // Store as pending baseline (promoted to confirmed when client acks)
        baselines.pending[id] = current;
    });

    baselines.pendingTick = 0; // Set by caller (ServerInstance sets the tick)
}

// Baseline Management

void EntityReplicator::AcknowledgeTick(ConnectionID client, uint32_t tick) {
    auto it = _clientData.find(client);
    if (it == _clientData.end()) return;

    auto& baselines = it->second;

    // Only promote if this ack is for our pending data
    if (tick >= baselines.pendingTick) {
        // Move all pending baselines to confirmed
        for (auto& [entityId, state] : baselines.pending) {
            baselines.confirmed[entityId] = state;
        }
        baselines.pending.clear();
    }
}

void EntityReplicator::RemoveClient(ConnectionID client) {
    _clientData.erase(client);
}

void EntityReplicator::OnEntityDestroyed(EntityID entity) {
    // Remove from all client baselines
    for (auto& [clientId, baselines] : _clientData) {
        baselines.confirmed.erase(entity);
        baselines.pending.erase(entity);
    }
}

// State Capture

ReplicatedEntityState EntityReplicator::CaptureEntityState(const Entity* entity) const {
    ReplicatedEntityState state;
    state.id = entity->GetID();
    state.type = entity->GetType();
    state.position = entity->GetPosition();

    // Living entity fields
    if (entity->GetType() == EntityType::Player ||
        entity->GetType() == EntityType::NPC ||
        entity->GetType() == EntityType::Monster) {

        const auto* living = static_cast<const LivingEntity*>(entity);
        state.health = living->GetCurrentHealth();
        state.maxHealth = living->GetMaxHealth();
        state.isDead = living->IsDead();

        // Player-specific
        if (entity->GetType() == EntityType::Player) {
            const auto* player = static_cast<const PlayerEntity*>(entity);
            state.movementState = static_cast<uint8_t>(player->GetMovementState());
        }
    }

    // Velocity from physics body would be set by ServerInstance
    // before calling BuildSnapshot (it has access to VoxelPhysics)
    // For now, velocity stays at default (0,0,0) unless set externally

    return state;
}

// Delta Comparison
uint32_t EntityReplicator::ComputeDirtyFlags(const ReplicatedEntityState& baseline,
                                              const ReplicatedEntityState& current) const {
    uint32_t flags = 0;

    // Position
    float dx = current.position.x - baseline.position.x;
    float dy = current.position.y - baseline.position.y;
    float dz = current.position.z - baseline.position.z;
    if (dx * dx + dy * dy + dz * dz > POSITION_EPSILON * POSITION_EPSILON) {
        flags |= DirtyFlag::Position;
    }

    // Velocity
    float dvx = current.velocity.x - baseline.velocity.x;
    float dvy = current.velocity.y - baseline.velocity.y;
    float dvz = current.velocity.z - baseline.velocity.z;
    if (dvx * dvx + dvy * dvy + dvz * dvz > VELOCITY_EPSILON * VELOCITY_EPSILON) {
        flags |= DirtyFlag::Velocity;
    }

    // Yaw
    if (std::abs(current.yaw - baseline.yaw) > YAW_EPSILON) {
        flags |= DirtyFlag::Yaw;
    }

    // Health
    if (std::abs(current.health - baseline.health) > HEALTH_EPSILON) {
        flags |= DirtyFlag::Health;
    }

    // Max health
    if (std::abs(current.maxHealth - baseline.maxHealth) > HEALTH_EPSILON) {
        flags |= DirtyFlag::MaxHealth;
    }

    // Dead state
    if (current.isDead != baseline.isDead) {
        flags |= DirtyFlag::IsDead;
    }

    // Movement state
    if (current.movementState != baseline.movementState) {
        flags |= DirtyFlag::MovementState;
    }

    // Animation
    if (current.animationId != baseline.animationId) {
        flags |= DirtyFlag::AnimationId;
    }

    return flags;
}