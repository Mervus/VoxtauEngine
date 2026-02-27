//
// Created by Marvin on 27/02/2026.
//

#include "ServerInstance.h"
#include "ClientProxy.h"
#include "Core/Voxel/ChunkManager.h"
#include "Core/Voxel/IChunkGenerator.h"
#include "Core/Physics/Voxtau/VoxelPhysics.h"
#include "Core/Entity/EntityManager.h"
#include "Core/Entity/Living/Player/PlayerEntity.h"
#include "Core/Network/Transport/INetworkTransport.h"
#include "Core/Network/Protocol/PacketTypes.h"
#include "Resources/BlockRegistry.h"
#include "Core/Network/Replication/EntityReplicator.h"

#include <iostream>
#include <algorithm>
#include <cstring>

#include "Core/Network/Protocol/InputButton.h"
#include "Core/World/IWorldGenerator.h"

//  Lifecycle 

ServerInstance::ServerInstance() = default;

ServerInstance::~ServerInstance() {
    Shutdown();
}

void ServerInstance::Initialize(const ServerConfig& config) {
    _config = config;

    _chunkManager = std::make_unique<ChunkManager>();
    _entityManager = std::make_unique<EntityManager>();
    _physics = std::make_unique<VoxelPhysics>();
    _physics->Initialize(_chunkManager.get());

    _currentTick = 0;
    _accumulator = 0.0f;

    std::cout << "[Server] Initialized (tickRate=" << _config.tickRate
              << ", maxClients=" << _config.maxClients << ")" << std::endl;
}

void ServerInstance::SetBlockRegistry(BlockRegistry* registry) {
    _blockRegistry = registry;
    if (_chunkManager) {
        _chunkManager->SetBlockRegistry(registry);
    }
}

void ServerInstance::GenerateWorld() {
    if (!_chunkManager) {
        std::cerr << "[Server] Cannot generate world — not initialized" << std::endl;
        return;
    }

    // Phase 1: Set terrain generator for chunk creation
    if (_worldGenerator) {

        _chunkManager->SetChunkGenerator(_worldGenerator->GetTerrainGenerator());
    }

    // Phase 2: Create chunk volume
    for (int y = 0; y < _config.verticalChunks; y++)
        for (int x = -_config.renderDistance; x < _config.renderDistance; x++)
            for (int z = -_config.renderDistance; z < _config.renderDistance; z++)
                _chunkManager->CreateChunk(x, y, z);

    if (_worldGenerator) {
        _worldGenerator->Generate(_chunkManager.get());
    }

    std::cout << "[Server] Generating world (seed=" << _config.worldSeed
              << ", radius=" << _config.renderDistance
              << ", height=" << _config.verticalChunks << ")" << std::endl;

    // Create initial chunk volume
    for (int y = 0; y < _config.verticalChunks; y++) {
        for (int x = -_config.renderDistance; x < _config.renderDistance; x++) {
            for (int z = -_config.renderDistance; z < _config.renderDistance; z++) {
                _chunkManager->CreateChunk(x, y, z);
            }
        }
    }

    std::cout << "[Server] World generated ("
              << _chunkManager->GetChunks().size() << " chunks)" << std::endl;
}

void ServerInstance::Shutdown() {
    // Despawn all players
    for (auto& [connId, proxy] : _clients) {
        DespawnPlayer(connId);
    }
    _clients.clear();
    _transports.clear();

    if (_physics) {
        _physics->Shutdown();
        _physics.reset();
    }

    _entityManager.reset();
    _chunkManager.reset();

    std::cout << "[Server] Shutdown complete" << std::endl;
}

//  Networking 

void ServerInstance::AddTransport(INetworkTransport* transport) {
    if (!transport) return;
    _transports.push_back(transport);
}

//  Main Tick 

void ServerInstance::Tick(float deltaTime) {
    // 1. Read all incoming packets from all transports
    ProcessIncomingPackets();

    // 2. Fixed timestep simulation
    float fixedDt = 1.0f / _config.tickRate;
    _accumulator += deltaTime;

    // Prevent spiral of death — cap at 4 steps per frame
    int maxSteps = 4;
    int steps = 0;

    while (_accumulator >= fixedDt && steps < maxSteps) {
        // Apply buffered client inputs for this tick
        ApplyClientInputs();

        // Step the simulation
        StepSimulation();

        _accumulator -= fixedDt;
        _currentTick++;
        steps++;
    }

    // If we capped out, drain the excess to prevent permanent lag
    if (steps >= maxSteps && _accumulator > fixedDt) {
        _accumulator = 0.0f;
    }

    // 3. Send state to clients (at network send rate, not every sim tick)
    // For now: send every tick. Later: send at 20-30Hz with a send accumulator.
    ReplicateState();

    // 4. Stream chunk data to clients that need it
    StreamChunks();
}

//  Client Management 

ConnectionID ServerInstance::OnClientConnected(INetworkTransport* transport) {
    if (_clients.size() >= _config.maxClients) {
        std::cerr << "[Server] Rejecting connection — server full ("
                  << _config.maxClients << " max)" << std::endl;
        return INVALID_CONNECTION;
    }

    ConnectionID connId = _nextConnectionId++;

    auto proxy = std::make_unique<ClientProxy>();
    proxy->connectionId = connId;
    proxy->transport = transport;
    proxy->state = ClientProxy::State::Connected;

    _clients[connId] = std::move(proxy);

    std::cout << "[Server] Client " << connId << " connected" << std::endl;

    // Spawn a player entity for this client
    EntityID playerId = SpawnPlayer(connId);

    // Notify game code (set spawn position, inventory, etc.)
    if (OnPlayerSpawned) {
        OnPlayerSpawned(playerId, connId);
    }

    return connId;
}

void ServerInstance::OnClientDisconnected(ConnectionID id) {
    auto it = _clients.find(id);
    if (it == _clients.end()) return;

    std::cout << "[Server] Client " << id << " disconnected" << std::endl;

    DespawnPlayer(id);
    _clients.erase(it);
}

EntityID ServerInstance::GetPlayerEntity(ConnectionID client) const {
    auto it = _clients.find(client);
    if (it == _clients.end()) return EntityID();
    return it->second->playerEntity;
}

//  Internal: Packet Processing 

void ServerInstance::ProcessIncomingPackets() {
    std::vector<NetworkEvent> events;

    for (auto* transport : _transports) {
        events.clear();
        transport->PollEvents(events);

        for (const auto& event : events) {
            switch (event.type) {

            case NetworkEvent::Type::Connected:
                OnClientConnected(transport);
                break;

            case NetworkEvent::Type::Disconnected:
                OnClientDisconnected(event.connection);
                break;

            case NetworkEvent::Type::DataReceived:
                // Route packet to the correct client proxy
                {
                    // Find which of our clients this connection belongs to
                    // For LocalTransport, connection ID maps directly
                    // For real network, we need a transport+conn → ConnectionID lookup
                    // For now: iterate (fine for ≤32 clients)
                    for (auto& [connId, proxy] : _clients) {
                        if (proxy->transport == transport) {
                            proxy->EnqueuePacket(event.data);
                            break;
                        }
                    }
                }
                break;
            }
        }
    }
}

//  Internal: Parse Raw Packets into Typed Queues

void ServerInstance::ProcessClientPackets() {
    for (auto& [connId, proxy] : _clients) {
        while (!proxy->rawPacketQueue.empty()) {
            const auto& data = proxy->rawPacketQueue.front();
            if (data.size() < 2) { proxy->rawPacketQueue.pop(); continue; }

            auto type = static_cast<PacketType>(data[0]);
            switch (type) {

            case PacketType::ClientInput: {
                uint8_t inputCount = data[1];
                size_t offset = 2;
                for (uint8_t i = 0; i < inputCount; i++) {
                    if (offset + sizeof(PlayerInputState) > data.size()) break;
                    PlayerInputState input;
                    std::memcpy(&input, data.data() + offset, sizeof(PlayerInputState));
                    offset += sizeof(PlayerInputState);

                    // Skip inputs we've already processed (from redundant sends)
                    if (input.tick > proxy->lastProcessedInputTick) {
                        proxy->EnqueueInput(input);
                    }
                }
                break;
            }

            default:
                break;
            }

            proxy->rawPacketQueue.pop();
        }
    }
}

//  Internal: Input Application

void ServerInstance::ApplyClientInputs() {
    // First parse any raw packets into the typed input queue
    ProcessClientPackets();

    for (auto& [connId, proxy] : _clients) {
        if (proxy->state != ClientProxy::State::InGame) continue;
        if (!proxy->playerEntity.IsValid()) continue;

        // Dequeue one input per tick from this client's buffer
        PlayerInputState input;
        if (!proxy->DequeueInput(input)) {
            // No input available — reuse last input (handles packet loss)
            input = proxy->lastAppliedInput;
        }

        // Apply input to the player's physics body
        auto* player = _entityManager->GetEntityAs<PlayerEntity>(proxy->playerEntity);
        if (!player) continue;

        VoxelBody* body = _physics->GetBody(player->GetPhysicsBodyID());
        if (!body) continue;

        // Reconstruct world-space movement from input axes + yaw
        // Server does this itself — never trusts a client-provided direction vector
        float sinYaw = std::sin(input.yaw);
        float cosYaw = std::cos(input.yaw);

        float worldMoveX = input.moveX * cosYaw - input.moveZ * sinYaw;
        float worldMoveZ = input.moveX * sinYaw + input.moveZ * cosYaw;

        body->inputVelocity.x = worldMoveX * player->GetMoveSpeed();
        body->inputVelocity.z = worldMoveZ * player->GetMoveSpeed();

        // Handle fly mode if allowed
        // if (proxy->canFly) {
        //     body->inputVelocity.y = input.moveY * player->GetMoveSpeed();
        // }

        // Handle jump
        if ((input.buttons & InputButton::Jump) && body->grounded) {
            body->velocity.y = player->GetJumpForce();
        }

        proxy->lastAppliedInput = input;
        proxy->lastProcessedInputTick = input.tick;
    }
}

//  Internal: Simulation Step 

void ServerInstance::StepSimulation() {
    float fixedDt = 1.0f / _config.tickRate;

    // Physics steps all bodies
    _physics->Update(fixedDt);

    // Entity update (AI, status effects, timers, etc.)
    _entityManager->Update(fixedDt);
    _entityManager->LateUpdate(fixedDt);
    _entityManager->DestroyPending();
}

//  Internal: State Replication 

void ServerInstance::ReplicateState() {
    // Build and send a snapshot to each connected client
    for (auto& [connId, proxy] : _clients) {
        if (proxy->state != ClientProxy::State::InGame) continue;

        ServerSnapshot snapshot;
        snapshot.serverTick = _currentTick;
        snapshot.lastProcessedInput = proxy->lastProcessedInputTick;
        snapshot.worldStateVersion = 0; // TODO: track block changes

        // Local player authoritative state
        auto* player = _entityManager->GetEntityAs<PlayerEntity>(proxy->playerEntity);
        if (player) {
            VoxelBody* body = _physics->GetBody(player->GetPhysicsBodyID());
            if (body) {
                snapshot.playerPosition = body->position;
                snapshot.playerVelocity = body->velocity;
                snapshot.playerOnGround = body->grounded;
            }
        }

        // Visible entities (everything within this client's area of interest)
        // For now: send all entities except the client's own player
        // TODO: proper AOI based on distance / chunk subscription
        _entityManager->ForEach([&](Entity* entity) {
            if (entity->GetID() == proxy->playerEntity) return; // skip self

            ReplicatedEntityState state;
            state.id = entity->GetID();
            state.type = entity->GetType();
            state.position = entity->GetPosition();

            // If it's a living entity, include combat-relevant state
            if (entity->GetType() == EntityType::Player ||
                entity->GetType() == EntityType::NPC ||
                entity->GetType() == EntityType::Monster) {
                auto* living = static_cast<LivingEntity*>(entity);
                state.health = living->GetCurrentHealth();
                state.maxHealth = living->GetMaxHealth();
                state.isDead = living->IsDead();
                state.velocity = Math::Vector3(); // TODO: get from physics body
            }

            snapshot.entities.push_back(state);
        });

        // Serialize and send
        // TODO: actual serialization + delta compression
        // For now with LocalTransport, we could pass the struct directly
        // but we should serialize to practice the real path

        // proxy->transport->SendPacket(connId, serializedData, Channel::Snapshot, SendMode::Unreliable);
    }
}

//  Internal: Chunk Streaming 

void ServerInstance::StreamChunks() {
    // TODO: For each client, check if they need new chunks based on their
    // player position. Send chunk data for chunks they don't have yet.
    // Priority: closest chunks first.
    //
    // For singleplayer with LocalTransport, the client can read directly
    // from the server's ChunkManager (optimization — skip serialization).
}

//  Internal: Player Management 

EntityID ServerInstance::SpawnPlayer(ConnectionID client) {
    auto it = _clients.find(client);
    if (it == _clients.end()) return EntityID();

    // Create the player entity
    EntityID playerId = _entityManager->CreateEntity<PlayerEntity>("Player_" + std::to_string(client));

    auto* player = _entityManager->GetEntityAs<PlayerEntity>(playerId);
    if (!player) return EntityID();

    // Create a physics body for this player
    // Default spawn position — game code overrides via OnPlayerSpawned callback
    Math::Vector3 spawnPos(0.0f, 80.0f, 0.0f);
    VoxelBodyID bodyId = _physics->CreateBody(spawnPos, 0.6f, 1.8f);
    player->BindPhysics(_physics.get(), bodyId);
    player->SetRespawnPosition(spawnPos);

    // Link player to client proxy
    it->second->playerEntity = playerId;
    it->second->state = ClientProxy::State::InGame;

    std::cout << "[Server] Spawned player (entity=" << playerId.Get()
              << ") for client " << client << std::endl;

    return playerId;
}

void ServerInstance::DespawnPlayer(ConnectionID client) {
    auto it = _clients.find(client);
    if (it == _clients.end()) return;

    EntityID playerId = it->second->playerEntity;
    if (!playerId.IsValid()) return;

    // Destroy physics body
    auto* player = _entityManager->GetEntityAs<PlayerEntity>(playerId);
    if (player) {
        VoxelBodyID bodyId = player->GetPhysicsBodyID();
        if (bodyId.IsValid()) {
            _physics->DestroyBody(bodyId);
        }
    }

    // Destroy entity
    _entityManager->DestroyEntity(playerId);
    _entityManager->DestroyPending();

    it->second->playerEntity = EntityID();
    it->second->state = ClientProxy::State::Connected;

    std::cout << "[Server] Despawned player for client " << client << std::endl;
}