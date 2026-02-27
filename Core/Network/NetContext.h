//
// Created by Marvin on 27/02/2026.
//

#ifndef VOXTAU_NETCONTEXT_H
#define VOXTAU_NETCONTEXT_H
#pragma once

#include <memory>
#include <cstdint>
#include <string>
#include "Core/Math/Vector3.h"

struct ServerConfig;
class ServerInstance;
class ClientSession;
class INetworkTransport;
class ChunkManager;
class VoxelPhysics;
class EntityManager;

enum class NetMode : uint8_t {
    Standalone,       // Singleplayer — local server + local client
    ListenServer,     // Co-op host — local server (port open) + local client
    DedicatedServer,  // Headless — server only, no client
    Client            // Remote — client only, connects to remote server
};

class NetContext
{
    public:
    NetContext();
    ~NetContext();

    // Setup (call once)

    // Singleplayer: creates local server + client, connects via LocalTransport
    void StartStandalone(const ServerConfig& config);

    // Host: creates local server + client, opens port for remote players
    void StartListenServer(const ServerConfig& config);

    // Dedicated: creates server only, opens port
    void StartDedicatedServer(const ServerConfig& config);

    // Remote client: creates client only, connects to server
    void ConnectToServer(const std::string& address, uint16_t port);

    void Shutdown();

    // Per-frame (call every tick)

    // Processes network I/O, advances simulation, sends state
    // Call this once per frame — it handles both server and client ticks
    void Tick(float deltaTime);

    //  Accessors 

    [[nodiscard]] NetMode GetMode() const { return _mode; }
    bool IsServer() const { return _mode == NetMode::Standalone
                                || _mode == NetMode::ListenServer
                                || _mode == NetMode::DedicatedServer; }
    bool IsClient() const { return _mode != NetMode::DedicatedServer; }
    bool HasLocalServer() const { return _mode == NetMode::Standalone
                                      || _mode == NetMode::ListenServer; }

    [[nodiscard]] ServerInstance* GetServer() { return _server.get(); }
    [[nodiscard]] ClientSession* GetClient() { return _client.get(); }

    // Server-side accessors (nullptr if not a server)
    ChunkManager* GetChunkManager();
    VoxelPhysics* GetPhysics();
    EntityManager* GetEntityManager();

    // Client-side: the predicted local player position (for rendering)
    Math::Vector3 GetLocalPlayerPosition() const;

    // Network stats
    uint32_t GetCurrentTick() const;
    float GetRTT() const;

private:
    NetMode _mode = NetMode::Standalone;

    std::unique_ptr<ServerInstance> _server;
    std::unique_ptr<ClientSession> _client;

    // Transport layers
    std::unique_ptr<INetworkTransport> _localTransportServer;  // Server end (local)
    std::unique_ptr<INetworkTransport> _localTransportClient;  // Client end (local)
    std::unique_ptr<INetworkTransport> _networkTransport;      // Real network (ENet/GNS)
};


#endif //VOXTAU_NETCONTEXT_H