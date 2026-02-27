//
// Created by Marvin on 27/02/2026.
//

#include "NetContext.h"
#include "Server/ServerInstance.h"
#include "Client/ClientSession.h"
#include "Transport/LocalTransport.h"

NetContext::NetContext()
{
}

NetContext::~NetContext()
= default;

void NetContext::StartStandalone(const ServerConfig& config)
{
    _mode = NetMode::Standalone;

    // Create the local server (owns the world simulation)
    _server = std::make_unique<ServerInstance>();

    ServerConfig serverConfig;
    serverConfig.tickRate = config.tickRate;
    serverConfig.maxClients = 1;
    serverConfig.worldSeed = config.worldSeed;
    _server->Initialize(serverConfig);

    // Create a linked transport pair (in-process, zero latency)
    auto [serverSide, clientSide] = LocalTransport::CreatePair();

    // Give the server its end of the transport
    _server->AddTransport(serverSide.get());
    _localTransportServer = std::move(serverSide);
    _localTransportClient = std::move(clientSide);

    // Create the client session
    _client = std::make_unique<ClientSession>();
    _client->Initialize(_localTransportClient.get());

    // Client "connects" — server sees a new connection, spawns a player
    _localTransportClient->Connect("local", 0);
    // Process the connection event so the player entity exists immediately
    _server->Tick(0.0f);
}

void NetContext::Shutdown()
{
    if (_client) {
        _client->Shutdown();
        _client.reset();
    }
    if (_server) {
        _server->Shutdown();
        _server.reset();
    }
    _localTransportServer.reset();
    _localTransportClient.reset();
    _networkTransport.reset();
    _mode = NetMode::Standalone;
}

void NetContext::Tick(float deltaTime) {
    // Server processes incoming packets and simulates
    if (_server) {
        _server->Tick(deltaTime);
    }

    // Client processes snapshots, sends input, predicts
    if (_client) {
        _client->Tick(deltaTime);
    }
}