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
#include "Core/Network/Replication/RepField.h"

class EntityManager;
class Entity;
class BitWriter;

using ConnectionID = uint32_t;

class EntityReplicator {
public:
    EntityReplicator();
    ~EntityReplicator();

    
    // Per-tick: serialize entity state for one client

    /**
    Writes all visible entity state into the BitWriter.
    Handles AOI filtering, scope selection, and delta compression.

    For each entity in AOI (excluding ownerEntity):
      - Serializes Scope::All fields
      - Compares against confirmed baseline, skips if unchanged
    For ownerEntity:
      - Also serializes Scope::Owner fields

    Wire format per entity:
      [EntityID]    4 bytes
      [EntityType]  1 byte
      [Scope::All serialized fields]
      [hasOwner]    1 byte (bool)
      [Scope::Owner serialized fields]  (only if hasOwner is true)

    Packet ends with EntityID(0) sentinel.

     * @param client
     * @param entityManager
     * @param ownerEntity
     * @param viewCenter
     * @param viewRadius
     * @param serverTick
     * @param writer
     */
    void SerializeEntitiesForClient(ConnectionID client,
                                    const EntityManager* entityManager,
                                    EntityID ownerEntity,
                                    const Math::Vector3& viewCenter,
                                    float viewRadius,
                                    uint32_t serverTick,
                                    BitWriter& writer);

    // Client acknowledged receiving snapshot at this tick.
    // Promote pending baselines to confirmed.
    void AcknowledgeTick(ConnectionID client, uint32_t tick);

    // Register which entity a client owns (for Scope::Owner fields).
    void SetClientOwner(ConnectionID client, EntityID ownerEntity);

    // Client disconnected — clean up all their data.
    void RemoveClient(ConnectionID client);

    // Entity destroyed — remove from all client baselines.
    void OnEntityDestroyed(EntityID entity);


private:
    /** Per-entity baseline: raw bytes for each scope */
    struct EntityBaseline {
        /** Scope::All fields, serialized */
        std::vector<uint8_t> publicState;
        /** Scope::Owner fields, serialized */
        std::vector<uint8_t> ownerState;
    };

    struct ClientData {
        // Confirmed: client has acked receiving this state.
        // Delta is computed against confirmed baselines.
        std::unordered_map<EntityID, EntityBaseline> confirmed;

        // Pending: sent but not yet acked.
        // Promoted to confirmed when client acks the tick.
        std::unordered_map<EntityID, EntityBaseline> pending;

        // Which tick the pending data was sent on.
        uint32_t pendingTick = 0;

        // Which entity this client owns (receives Scope::Owner fields).
        EntityID ownerEntity;
    };

    std::unordered_map<ConnectionID, ClientData> _clientData;

    //  Internal helpers 

    // Serialize an entity's fields for a given scope into a byte buffer.
    // Returns the serialized bytes.
    static std::vector<uint8_t> CaptureScope(const Entity* entity, Scope scope);

    // Compare two byte buffers. Returns true if identical.
    static bool BaselinesMatch(const std::vector<uint8_t>& a,
                                const std::vector<uint8_t>& b);
};

#endif //VOXTAU_ENTITYREPLICATOR_H