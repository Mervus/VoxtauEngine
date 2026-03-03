//
// Created by Marvin on 27/02/2026.
//

#include "LocalTransport.h"

std::pair<std::unique_ptr<LocalTransport>,
          std::unique_ptr<LocalTransport>> LocalTransport::CreatePair()
{
    // Can't use make_unique with private constructor
    std::unique_ptr<LocalTransport> a(new LocalTransport());
    std::unique_ptr<LocalTransport> b(new LocalTransport());
    a->_peer = b.get();
    b->_peer = a.get();
    return {std::move(a), std::move(b)};
}

bool LocalTransport::Initialize(uint16_t /*port*/, uint32_t /*maxConnections*/)
{
    return true;
}

void LocalTransport::Shutdown()
{
    _connected = false;
    if (_peer) {
        _peer->_connected = false;
        _peer->_peer = nullptr;
    }
    _peer = nullptr;
}

ConnectionID LocalTransport::Connect(const std::string& /*address*/, uint16_t /*port*/)
{
    if (!_peer) return 0;

    _connected = true;
    _peer->_connected = true;

    // Notify the peer (server side) that a connection arrived
    NetworkEvent serverEvt;
    serverEvt.type = NetworkEvent::Type::Connected;
    serverEvt.connection = 1;
    _peer->Enqueue(std::move(serverEvt));

    // Notify ourselves (client side) that we're connected
    NetworkEvent clientEvt;
    clientEvt.type = NetworkEvent::Type::Connected;
    clientEvt.connection = 1;
    Enqueue(std::move(clientEvt));

    return 1;
}

void LocalTransport::Disconnect(ConnectionID /*conn*/)
{
    if (!_peer) return;

    NetworkEvent evt;
    evt.type = NetworkEvent::Type::Disconnected;
    evt.connection = 1;
    _peer->Enqueue(std::move(evt));

    _connected = false;
    _peer->_connected = false;
}

void LocalTransport::SendPacket(ConnectionID /*conn*/, std::span<const uint8_t> data,
                                uint8_t channel, SendMode /*mode*/)
{
    if (!_peer) return;

    NetworkEvent evt;
    evt.type = NetworkEvent::Type::DataReceived;
    evt.connection = 1;
    evt.channel = channel;
    evt.data.assign(data.begin(), data.end());
    _peer->Enqueue(std::move(evt));
}

void LocalTransport::Broadcast(std::span<const uint8_t> data,
                               uint8_t channel, SendMode mode)
{
    // Local transport only has one peer
    SendPacket(1, data, channel, mode);
}

void LocalTransport::PollEvents(std::vector<NetworkEvent>& outEvents)
{
    std::lock_guard lock(_inboxMutex);
    while (!_inbox.empty()) {
        outEvents.push_back(std::move(_inbox.front()));
        _inbox.pop();
    }
}

void LocalTransport::Flush()
{
}

void LocalTransport::Enqueue(NetworkEvent event)
{
    std::lock_guard lock(_inboxMutex);
    _inbox.push(std::move(event));
}
