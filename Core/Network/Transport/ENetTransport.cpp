//
// Created by Marvin on 27/02/2026.
//

#include "ENetTransport.h"

#include <iostream>

// ENet must be initialized once globally
static bool s_enetInitialized = false;
static int s_enetRefCount = 0;

static void EnsureENetInit() {
    if (!s_enetInitialized) {
        if (enet_initialize() != 0) {
            std::cerr << "[ENet] Failed to initialize!" << std::endl;
            return;
        }
        s_enetInitialized = true;
    }
    s_enetRefCount++;
}

static void ENetRelease() {
    s_enetRefCount--;
    if (s_enetRefCount <= 0) {
        enet_deinitialize();
        s_enetInitialized = false;
        s_enetRefCount = 0;
    }
}

//  Lifecycle 

ENetTransport::ENetTransport() = default;

ENetTransport::~ENetTransport() {
    Shutdown();
}

bool ENetTransport::Initialize(uint16_t port, uint32_t maxConnections) {
    EnsureENetInit();

    if (port > 0) {
        // Server mode: listen on port
        _isServer = true;

        ENetAddress address;
        address.host = ENET_HOST_ANY;
        address.port = port;

        _host = enet_host_create(&address, maxConnections, CHANNEL_COUNT, 0, 0);
        if (!_host) {
            std::cerr << "[ENet] Failed to create server on port " << port << std::endl;
            return false;
        }

        std::cout << "[ENet] Server listening on port " << port
                  << " (max " << maxConnections << " clients)" << std::endl;
    } else {
        // Client mode: no listening, just outgoing connections
        _isServer = false;

        _host = enet_host_create(nullptr, maxConnections, CHANNEL_COUNT, 0, 0);
        if (!_host) {
            std::cerr << "[ENet] Failed to create client host" << std::endl;
            return false;
        }

        std::cout << "[ENet] Client host created" << std::endl;
    }

    return true;
}

void ENetTransport::Shutdown() {
    if (!_host) return;

    // Disconnect all peers gracefully
    for (auto& [connId, peer] : _peers) {
        enet_peer_disconnect(peer, 0);
    }

    // Flush disconnects (give 500ms for graceful close)
    ENetEvent event;
    while (enet_host_service(_host, &event, 500) > 0) {
        if (event.type == ENET_EVENT_TYPE_RECEIVE) {
            enet_packet_destroy(event.packet);
        }
    }

    _peers.clear();
    _peerToConn.clear();

    enet_host_destroy(_host);
    _host = nullptr;

    ENetRelease();

    std::cout << "[ENet] Shutdown complete" << std::endl;
}

//  Client Side 

ConnectionID ENetTransport::Connect(const std::string& address, uint16_t port) {
    if (!_host) return 0;

    ENetAddress enetAddr;
    enet_address_set_host(&enetAddr, address.c_str());
    enetAddr.port = port;

    ENetPeer* peer = enet_host_connect(_host, &enetAddr, CHANNEL_COUNT, 0);
    if (!peer) {
        std::cerr << "[ENet] Failed to initiate connection to "
                  << address << ":" << port << std::endl;
        return 0;
    }

    // Assign a ConnectionID now — the Connected event confirms it later
    ConnectionID connId = _nextConnId++;
    _peers[connId] = peer;
    _peerToConn[peer] = connId;

    std::cout << "[ENet] Connecting to " << address << ":" << port
              << " (conn=" << connId << ")" << std::endl;

    return connId;
}

void ENetTransport::Disconnect(ConnectionID conn) {
    auto it = _peers.find(conn);
    if (it == _peers.end()) return;

    enet_peer_disconnect(it->second, 0);

    // Don't erase yet — wait for the disconnect event in PollEvents
}

//  Send / Receive 

void ENetTransport::SendPacket(ConnectionID conn, std::span<const uint8_t> data,
                                uint8_t channel, SendMode mode) {
    auto it = _peers.find(conn);
    if (it == _peers.end()) return;

    uint32_t flags = ToENetFlags(mode);
    ENetPacket* packet = enet_packet_create(data.data(), data.size(), flags);

    uint8_t ch = (channel < CHANNEL_COUNT) ? channel : 0;
    enet_peer_send(it->second, ch, packet);
}

void ENetTransport::Broadcast(std::span<const uint8_t> data,
                               uint8_t channel, SendMode mode) {
    if (!_host) return;

    uint32_t flags = ToENetFlags(mode);
    ENetPacket* packet = enet_packet_create(data.data(), data.size(), flags);

    uint8_t ch = (channel < CHANNEL_COUNT) ? channel : 0;
    enet_host_broadcast(_host, ch, packet);
}

void ENetTransport::PollEvents(std::vector<NetworkEvent>& outEvents) {
    if (!_host) return;

    ENetEvent event;

    // Process all pending events (timeout = 0 for non-blocking)
    while (enet_host_service(_host, &event, 0) > 0) {
        switch (event.type) {

        case ENET_EVENT_TYPE_CONNECT: {
            ConnectionID connId = GetOrAssignConnection(event.peer);

            NetworkEvent netEvent;
            netEvent.type = NetworkEvent::Type::Connected;
            netEvent.connection = connId;
            outEvents.push_back(std::move(netEvent));

            char hostname[256];
            enet_address_get_host_ip(&event.peer->address, hostname, sizeof(hostname));
            std::cout << "[ENet] Peer connected: " << hostname
                      << ":" << event.peer->address.port
                      << " (conn=" << connId << ")" << std::endl;
            break;
        }

        case ENET_EVENT_TYPE_RECEIVE: {
            ConnectionID connId = GetOrAssignConnection(event.peer);

            NetworkEvent netEvent;
            netEvent.type = NetworkEvent::Type::DataReceived;
            netEvent.connection = connId;
            netEvent.channel = event.channelID;
            netEvent.data.assign(
                event.packet->data,
                event.packet->data + event.packet->dataLength
            );
            outEvents.push_back(std::move(netEvent));

            enet_packet_destroy(event.packet);
            break;
        }

        case ENET_EVENT_TYPE_DISCONNECT: {
            auto peerIt = _peerToConn.find(event.peer);
            ConnectionID connId = (peerIt != _peerToConn.end()) ? peerIt->second : 0;

            NetworkEvent netEvent;
            netEvent.type = NetworkEvent::Type::Disconnected;
            netEvent.connection = connId;
            outEvents.push_back(std::move(netEvent));

            // Clean up mappings
            if (peerIt != _peerToConn.end()) {
                _peers.erase(peerIt->second);
                _peerToConn.erase(peerIt);
            }

            std::cout << "[ENet] Peer disconnected (conn=" << connId << ")" << std::endl;
            break;
        }

        case ENET_EVENT_TYPE_NONE:
            break;
        }
    }
}

// Info

uint32_t ENetTransport::GetRTT(ConnectionID conn) const {
    auto it = _peers.find(conn);
    if (it == _peers.end()) return 0;
    return it->second->roundTripTime;
}

bool ENetTransport::IsConnected(ConnectionID conn) const {
    auto it = _peers.find(conn);
    if (it == _peers.end()) return false;
    return it->second->state == ENET_PEER_STATE_CONNECTED;
}

//  Internal 

uint32_t ENetTransport::ToENetFlags(SendMode mode) {
    switch (mode) {
        case SendMode::Unreliable:          return 0;
        case SendMode::UnreliableSequenced: return ENET_PACKET_FLAG_UNSEQUENCED;
        case SendMode::Reliable:            return ENET_PACKET_FLAG_RELIABLE;
        case SendMode::ReliableOrdered:     return ENET_PACKET_FLAG_RELIABLE;
        default:                            return 0;
    }
}

ConnectionID ENetTransport::GetOrAssignConnection(ENetPeer* peer) {
    auto it = _peerToConn.find(peer);
    if (it != _peerToConn.end()) {
        return it->second;
    }

    // New peer (server side — incoming connection)
    ConnectionID connId = _nextConnId++;
    _peers[connId] = peer;
    _peerToConn[peer] = connId;
    return connId;
}