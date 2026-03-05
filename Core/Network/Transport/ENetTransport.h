//
// Created by Marvin on 27/02/2026.
//

#ifndef VOXTAU_ENETTRANSPORT_H
#define VOXTAU_ENETTRANSPORT_H
#pragma once

#include "INetworkTransport.h"
#include <unordered_map>
#include <enet/enet.h>

// Real UDP transport using ENet.
// Used for listen server (host accepts remote players) and dedicated server,
// and for remote clients connecting to a server.
//
// Channels:
//   0 = Unreliable (snapshots, input)
//   1 = Reliable ordered (chunk data, entity spawn/despawn, block changes)
//   2 = Reliable unordered (chat, misc RPCs)

class ENetTransport : public INetworkTransport {
public:
    static constexpr uint8_t CHANNEL_COUNT = 3;

    ENetTransport();
    ~ENetTransport() override;

    // Lifecycle

    // Server mode: port > 0, creates a host that accepts connections.
    // Client mode: port = 0, creates a host for outgoing connections.
    bool Initialize(uint16_t port, uint32_t maxConnections) override;
    void Shutdown() override;

    // Client side
    ConnectionID Connect(const std::string& address, uint16_t port) override;
    void Disconnect(ConnectionID conn) override;

    // Send / Receive
    void SendPacket(ConnectionID conn, std::span<const uint8_t> data,
                    uint8_t channel, SendMode mode) override;
    void Broadcast(std::span<const uint8_t> data,
                   uint8_t channel, SendMode mode) override;
    void PollEvents(std::vector<NetworkEvent>& outEvents) override;

    // Info
    [[nodiscard]] uint32_t GetRTT(ConnectionID conn) const override;
    [[nodiscard]] bool IsConnected(ConnectionID conn) const override;

    void Flush() override;
private:
    ENetHost* _host = nullptr;
    bool _isServer = false;

    // Map our ConnectionID ↔ ENet peer
    std::unordered_map<ConnectionID, ENetPeer*> _peers;
    std::unordered_map<ENetPeer*, ConnectionID> _peerToConn;
    ConnectionID _nextConnId = 1;

    // Convert SendMode to ENet flags
    static uint32_t ToENetFlags(SendMode mode);

    // Get or assign a ConnectionID for a peer
    ConnectionID GetOrAssignConnection(ENetPeer* peer);
};

#endif //VOXTAU_ENETTRANSPORT_H