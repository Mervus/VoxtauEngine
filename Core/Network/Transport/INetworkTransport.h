//
// Created by Marvin on 27/02/2026.
//

#ifndef VOXTAU_INETWORKTRANSPORT_H
#define VOXTAU_INETWORKTRANSPORT_H
#pragma once

#include <cstdint>
#include <vector>
#include <span>
#include <string>

using ConnectionID = uint32_t;

enum class SendMode : uint8_t {
    Unreliable,          // Fire and forget (entity snapshots)
    UnreliableSequenced, // Drop old, keep newest (input)
    Reliable,            // Guaranteed delivery (chat, block changes)
    ReliableOrdered      // Guaranteed + in order (chunk data, RPCs)
};

struct NetworkEvent {
    enum class Type : uint8_t {
        Connected,
        Disconnected,
        DataReceived
    };

    Type type;
    ConnectionID connection;
    std::vector<uint8_t> data;  // empty for Connect/Disconnect
    uint8_t channel;
};

class INetworkTransport {
public:
    virtual ~INetworkTransport() = default;

    // Lifecycle
    virtual bool Initialize(uint16_t port, uint32_t maxConnections) = 0;
    virtual void Shutdown() = 0;

    // Client side
    virtual ConnectionID Connect(const std::string& address, uint16_t port) = 0;
    virtual void Disconnect(ConnectionID conn) = 0;

    // Send / Receive
    virtual void SendPacket(ConnectionID conn, std::span<const uint8_t> data,
                           uint8_t channel, SendMode mode) = 0;
    virtual void Broadcast(std::span<const uint8_t> data,
                          uint8_t channel, SendMode mode) = 0;
    virtual void PollEvents(std::vector<NetworkEvent>& outEvents) = 0;

    //  Info 
    virtual uint32_t GetRTT(ConnectionID conn) const = 0;
    virtual bool IsConnected(ConnectionID conn) const = 0;

    virtual void Flush() = 0;
};
#endif //VOXTAU_INETWORKTRANSPORT_H