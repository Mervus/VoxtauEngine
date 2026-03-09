//
// Created by Marvin on 27/02/2026.
//

#ifndef VOXTAU_SERVERINSTANCE_H
#define VOXTAU_SERVERINSTANCE_H
#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <functional>
#include <set>

#include "Core/Entity/EntityID.h"
#include "Core/Math/Vector3.h"

class IWorldGenerator;
class ChunkManager;
class VoxelPhysics;
class EntityManager;
class BlockRegistry;
class IChunkGenerator;
class INetworkTransport;
class EntityReplicator;
class ClientProxy;

using ConnectionID = uint32_t;
constexpr ConnectionID INVALID_CONNECTION = 0;

struct ServerConfig {
    float tickRate = 60.0f;
    uint32_t maxClients = 32;
    uint32_t worldSeed = 12345;
    std::string serverAddress = "127.0.0.1";
    uint16_t port = 7777;
    int renderDistance = 8;        // in chunks, per client
    int verticalChunks = 6;
};

class ServerInstance
{
    public:
    ServerInstance();
    ~ServerInstance();

    //  Lifecycle 
    // Initialize all simulation systems
    void Initialize(const ServerConfig& config);

    // Set the terrain generator (game provides this  not engine's job)
    void SetWorldGenerator(IWorldGenerator* generator) { _worldGenerator = generator; }

    // Set the block registry (game provides this)
    void SetBlockRegistry(BlockRegistry* registry);

    // Generate initial world (call after setting generator)
    void GenerateWorld();

    void Shutdown();

    //  Networking 

    // Add a transport that this server listens on
    // A server can have multiple: LocalTransport for host + ENetTransport for remotes
    void AddTransport(INetworkTransport* transport);

    //  Per-tick 

    // Full server tick: receive input -> simulate -> replicate
    void Tick(float deltaTime);

    //  Client management 

    // Called when a new client connects (transport fires this)
    ConnectionID OnClientConnected(INetworkTransport* transport, ConnectionID transportConnId);

    // Called when a client disconnects
    void OnClientDisconnected(ConnectionID id);

    // Get the EntityID of a client's player
    [[nodiscard]] EntityID GetPlayerEntity(ConnectionID client) const;

    //  World access (for game code) 

    [[nodiscard]] ChunkManager* GetChunkManager() { return _chunkManager.get(); }
    [[nodiscard]] VoxelPhysics* GetPhysics() { return _physics.get(); }
    [[nodiscard]] EntityManager* GetEntityManager() { return _entityManager.get(); }

    [[nodiscard]] uint32_t GetCurrentTick() const { return _currentTick; }

    //  Events (for game code to hook into) 

    // Called after a player entity is spawned game can set spawn position, inventory, etc.
    std::function<void(EntityID, ConnectionID)> OnPlayerSpawned;

    // Called when a client requests a block change, game validates
    std::function<bool(ConnectionID, int, int, int, uint8_t)> OnBlockChangeRequested;

private:
    //  Simulation systems (owned) 
    std::unique_ptr<ChunkManager> _chunkManager;
    std::unique_ptr<VoxelPhysics> _physics;
    std::unique_ptr<EntityManager> _entityManager;

    //  Networking
    std::vector<INetworkTransport*> _transports;  // not owned, just observed
    std::unique_ptr<EntityReplicator> _replicator;
    std::unordered_map<ConnectionID, std::unique_ptr<ClientProxy>> _clients;
    ConnectionID _nextConnectionId = 1;

    //  Config 
    ServerConfig _config;
    BlockRegistry* _blockRegistry = nullptr;       // not owned
    IWorldGenerator* _worldGenerator = nullptr;      // not owned

    //  Timing 
    uint32_t _currentTick = 0;
    float _accumulator = 0.0f;

    //  Internal tick steps
    void ProcessIncomingPackets();
    void ProcessClientPackets();
    void ApplyClientInputs();
    void StepSimulation();
    void ReplicateState();
    void StreamChunks();

    //  Player management 
    EntityID SpawnPlayer(ConnectionID client);
    void DespawnPlayer(ConnectionID client);
};


#endif //VOXTAU_SERVERINSTANCE_H