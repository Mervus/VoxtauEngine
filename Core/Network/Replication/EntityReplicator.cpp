//
// Created by Marvin on 27/02/2026.
//

#include "EntityReplicator.h"
#include "Core/Entity/Entity.h"
#include "Core/Entity/EntityManager.h"
#include "Core/Network/Protocol/BitStream.h"

EntityReplicator::EntityReplicator() = default;
EntityReplicator::~EntityReplicator() = default;

void EntityReplicator::SerializeEntitiesForClient(
        ConnectionID client,
        const EntityManager* entityManager,
        EntityID ownerEntity,
        const Math::Vector3& viewCenter,
        float viewRadius,
        uint32_t serverTick,
        BitWriter& writer)
{
    auto& data = _clientData[client];
    data.ownerEntity = ownerEntity;
    data.pendingTick = serverTick;

    float viewRadiusSq = viewRadius * viewRadius;

    entityManager->ForEach([&](Entity* entity) {
        EntityID id = entity->GetID();

        //  AOI check 
        Math::Vector3 diff = entity->GetPosition() - viewCenter;
        float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        if (distSq > viewRadiusSq) return;

        //  Capture current state (serialize once into byte buffers) 
        std::vector<uint8_t> currentPublic = CaptureScope(entity, Scope::All);

        bool isOwner = (id == ownerEntity);
        std::vector<uint8_t> currentOwner;
        if (isOwner) {
            currentOwner = CaptureScope(entity, Scope::Owner);
        }

        //  Delta check against confirmed baseline 
        auto baselineIt = data.confirmed.find(id);
        bool isNew = (baselineIt == data.confirmed.end());

        if (!isNew) {
            bool publicChanged = !BaselinesMatch(baselineIt->second.publicState, currentPublic);
            bool ownerChanged = isOwner &&
                !BaselinesMatch(baselineIt->second.ownerState, currentOwner);

            if (!publicChanged && !ownerChanged) {
                return; // Nothing changed — skip this entity
            }
        }

        //  Write this entity 
        // Header: EntityID + EntityType
        writer.Write(id.Get());
        writer.Write(static_cast<uint8_t>(entity->GetType()));

        // Scope::All fields — write the already-captured bytes directly
        writer.WriteBytes(currentPublic.data(), currentPublic.size());

        // Scope::Owner flag + fields
        writer.Write(isOwner);
        if (isOwner) {
            writer.WriteBytes(currentOwner.data(), currentOwner.size());
        }

        //  Store as pending baseline 
        EntityBaseline& pending = data.pending[id];
        pending.publicState = std::move(currentPublic);
        if (isOwner) {
            pending.ownerState = std::move(currentOwner);
        } else {
            pending.ownerState.clear();
        }
    });

    //  Sentinel: EntityID(0) marks end of entity list 
    writer.Write(static_cast<uint32_t>(0));
}

void EntityReplicator::AcknowledgeTick(ConnectionID client, uint32_t tick) {
    auto it = _clientData.find(client);
    if (it == _clientData.end()) return;

    auto& data = it->second;

    // Only promote if this ack is for our pending data
    if (tick >= data.pendingTick) {
        for (auto& [entityId, baseline] : data.pending) {
            data.confirmed[entityId] = std::move(baseline);
        }
        data.pending.clear();
    }
}

void EntityReplicator::SetClientOwner(ConnectionID client, EntityID ownerEntity) {
    _clientData[client].ownerEntity = ownerEntity;
}

void EntityReplicator::RemoveClient(ConnectionID client) {
    _clientData.erase(client);
}

void EntityReplicator::OnEntityDestroyed(EntityID entity) {
    for (auto& [clientId, data] : _clientData) {
        data.confirmed.erase(entity);
        data.pending.erase(entity);
    }
}

std::vector<uint8_t> EntityReplicator::CaptureScope(const Entity* entity, Scope scope) {
    BitWriter writer(64);
    entity->SerializeScope(writer, scope);
    return writer.Release();
}

bool EntityReplicator::BaselinesMatch(const std::vector<uint8_t>& a,
                                       const std::vector<uint8_t>& b) {
    if (a.size() != b.size()) return false;
    if (a.empty()) return true;
    return std::memcmp(a.data(), b.data(), a.size()) == 0;
}