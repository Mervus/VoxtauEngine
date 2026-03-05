//
// Created by Marvin on 27/02/2026.
//

#ifndef VOXTAU_LOCALTRANSPORT_H
#define VOXTAU_LOCALTRANSPORT_H
#pragma once

#include "INetworkTransport.h"
#include <queue>
#include <mutex>

// Two of these are created, one for the server side, one for the client side.
// They share a pair of thread-safe queues. Packets "sent" by one appear as
// "received" events on the other. Zero serialization overhead in singleplayer.

class LocalTransport : public INetworkTransport {
public:
    // Create a linked pair: server transport + client transport
    static std::pair<std::unique_ptr<LocalTransport>,
                     std::unique_ptr<LocalTransport>> CreatePair();

    bool Initialize(uint16_t port, uint32_t maxConnections) override;
    void Shutdown() override;

    ConnectionID Connect(const std::string& address, uint16_t port) override;
    void Disconnect(ConnectionID conn) override;

    void SendPacket(ConnectionID conn, std::span<const uint8_t> data,
                   uint8_t channel, SendMode mode) override;
    void Broadcast(std::span<const uint8_t> data,
                  uint8_t channel, SendMode mode) override;
    void PollEvents(std::vector<NetworkEvent>& outEvents) override;

    [[nodiscard]] uint32_t GetRTT(ConnectionID conn) const override { return 0; }
    [[nodiscard]] bool IsConnected(ConnectionID conn) const override { return _connected; }

    void Flush() override;
private:
    LocalTransport() = default;

    // The other end of this "connection"
    LocalTransport* _peer = nullptr;
    bool _connected = false;

    // Incoming queue (peer writes here, we read)
    std::queue<NetworkEvent> _inbox;
    std::mutex _inboxMutex;

    // Peer pushes into our inbox
    void Enqueue(NetworkEvent event);
};


#endif //VOXTAU_LOCALTRANSPORT_H