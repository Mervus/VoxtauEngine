//
// Created by Marvin on 27/02/2026.
//

#ifndef VOXTAU_CLIENTPROXY_H
#define VOXTAU_CLIENTPROXY_H
#pragma once

#include <cstdint>
#include <queue>
#include <vector>
#include "Core/Entity/EntityID.h"
#include "Core/Network/Protocol/PlayerInputState.h"
#include "Core/Network/Protocol/ServerSnapshot.h"

class INetworkTransport;

using ConnectionID = uint32_t;

// Server-side representation of a connected client.
// Holds their input buffer, connection state, and replication bookkeeping.
struct ClientProxy {
    enum class State : uint8_t {
        Connected,     // Just connected, waiting for handshake / loading
        InGame         // Fully spawned, receiving snapshots
    };

    ConnectionID connectionId = 0;
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
};


#endif //VOXTAU_CLIENTPROXY_H