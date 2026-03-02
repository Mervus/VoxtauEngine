//
// Created by Marvin on 27/02/2026.
//

#ifndef VOXTAU_CLIENTPROXY_H
#define VOXTAU_CLIENTPROXY_H
#pragma once

#include <cstdint>
#include <queue>
#include <unordered_set>
#include <vector>
#include "Core/Entity/EntityID.h"
#include "Core/Network/Protocol/PlayerInputState.h"
#include "Core/Network/Protocol/ServerSnapshot.h"
#include "Core/Voxel/ChunkManager.h"

class INetworkTransport;

using ConnectionID = uint32_t;

// Server-side representation of a connected client.
// Holds their input buffer, connection state, and replication bookkeeping.
struct ClientProxy {
    enum class State : uint8_t {
        Connected,     // Just connected, waiting for handshake / loading
        InGame         // Fully spawned, receiving snapshots
    };

    // Server's internal ID — unique across ALL transports.
    // Used as the key in ServerInstance::_clients map.
    // Assigned by ServerInstance::_nextConnectionId counter.
    // Never sent to a transport — transports don't know about this.
    ConnectionID connectionId = 0;

    // The ID assigned by the transport layer (LocalTransport or ENetTransport).
    // Each transport has its own counter, so these can collide across transports
    // (e.g. LocalTransport client=1 and ENetTransport client=1 are different).
    // Used exclusively for transport->SendPacket() calls.
    ConnectionID transportConnId = 0;
    INetworkTransport* transport = nullptr;
    State state = State::Connected;

    // The player entity this client controls
    EntityID playerEntity;

    // Input buffering
    // Server buffers a few ticks of input to absorb jitter.
    // Consumes one per simulation tick.
    static constexpr size_t MAX_INPUT_BUFFER = 16;
    std::queue<PlayerInputState> inputBuffer;
    PlayerInputState lastAppliedInput;    // reused if no input available (packet loss)
    uint32_t lastProcessedInputTick = 0;  // sent back in snapshot for reconciliation

    void EnqueueInput(const PlayerInputState& input) {
        // Drop if buffer is full (client is too far ahead)
        if (inputBuffer.size() >= MAX_INPUT_BUFFER) {
            inputBuffer.pop(); // drop oldest
        }
        inputBuffer.push(input);
    }

    bool DequeueInput(PlayerInputState& out) {
        if (inputBuffer.empty()) return false;
        out = inputBuffer.front();
        inputBuffer.pop();
        return true;
    }

    // Raw packet buffering (before deserialization)
    // ProcessIncomingPackets dumps raw bytes here,
    // then we parse them into inputs/block requests/etc.
    std::queue<std::vector<uint8_t>> rawPacketQueue;

    void EnqueuePacket(const std::vector<uint8_t>& data) {
        rawPacketQueue.push(data);
    }

    // Replication bookkeeping
    // Per-entity baselines for delta compression (added later)
    // std::unordered_map<EntityID, ReplicatedEntityState> baselines;

    // Which chunks this client has received (for streaming)
    // std::unordered_set<ChunkCoord> receivedChunks;

    // Tracks which chunks this client has received
    std::unordered_set<Math::Vector3, Vector3Hash, Vector3Equal> receivedChunks;

    // Chunks queued to send (sorted by priority — closest first)
    std::vector<Math::Vector3> chunkSendQueue;

    // Max chunks to send per tick (bandwidth throttle)
    static constexpr int MAX_CHUNKS_PER_TICK = 4;
};


#endif //VOXTAU_CLIENTPROXY_H