//
// Created by Marvin on 27/02/2026.
//

#include "ClientSession.h"
#include "EntityInterpolator.h"
#include "Core/Network/Transport/INetworkTransport.h"
#include "Core/Network/Protocol/PacketTypes.h"
#include "Core/Network/Protocol/PlayerInputState.h"
#include "Core/Network/Protocol/ServerSnapshot.h"
#include "Core/Physics/Voxtau/VoxelPhysics.h"
#include "Core/Physics/Voxtau/VoxelBody.h"
#include "Core/Entity/EntityManager.h"

#include <cmath>
#include <cstring>
#include <iostream>

#include "Core/Entity/Living/Player/PlayerEntity.h"
#include "Core/Network/Protocol/InputButton.h"
#include "Core/Network/Protocol/PacketSerializer.h"
#include "Core/Voxel/ChunkManager.h"

// How aggressively the visual position chases the simulation position.
// 0.1 = smooth (10% per frame), 1.0 = instant snap.
static constexpr float VISUAL_SMOOTH_RATE = 0.2f;

// If prediction error exceeds this, snap instead of smoothing.
static constexpr float SNAP_THRESHOLD = 3.0f;

// Threshold below which prediction is considered correct (no correction needed).
static constexpr float RECONCILE_EPSILON = 0.01f;

//  Lifecycle 

ClientSession::ClientSession() = default;

ClientSession::~ClientSession() {
    Shutdown();
}

void ClientSession::Initialize(INetworkTransport* transport) {
    _transport = transport;

    // Create a local physics instance for prediction.
    // This runs the same VoxelPhysics code as the server, but only
    // simulates the local player's body. It needs a solid query —
    // for singleplayer, the server's ChunkManager is in-process so
    // we can share the query. For remote, we'd use the client's
    // local chunk data.
    _predictionPhysics = std::make_unique<VoxelPhysics>();

    _interpolator = std::make_unique<EntityInterpolator>();

    _localChunkManager =  std::make_unique<ChunkManager>();

    // Zero out input history
    std::memset(_inputHistory, 0, sizeof(_inputHistory));
    std::memset(_stateHistory, 0, sizeof(_stateHistory));
    _inputHead = 0;

    _currentTick = 0;
    _lastAckedServerTick = 0;
    _connected = false;

    std::cout << "[Client] Initialized" << std::endl;
}

void ClientSession::InitializePrediction(VoxelSolidQuery solidQuery,
                                          const Math::Vector3& startPos,
                                          float bodyWidth, float bodyHeight) {
    _predictionPhysics->Initialize(std::move(solidQuery));

    // Create a prediction body matching the server's player body
    _predictionBody = _predictionPhysics->CreateBody(startPos, bodyWidth, bodyHeight);

    // Disable the internal accumulator behavior — we step manually
    // by calling Update(fixedTimestep) which produces exactly one step
    _predictionPhysics->GetConfig().maxStepsPerFrame = 1;

    _simulationPosition = startPos;
    _visualPosition = startPos;

    std::cout << "[Client] Prediction initialized at ("
              << startPos.x << ", " << startPos.y << ", " << startPos.z << ")" << std::endl;
}

void ClientSession::Shutdown() {
    if (_predictionBody.IsValid() && _predictionPhysics) {
        _predictionPhysics->DestroyBody(_predictionBody);
        _predictionBody = VoxelBodyID();
    }
    _predictionPhysics.reset();
    _interpolator.reset();
    _connected = false;

    std::cout << "[Client] Shutdown" << std::endl;
}

void ClientSession::Connect(const std::string& address, uint16_t port) {
    if (!_transport) return;
    _transport->Connect(address, port);
    // Connection is confirmed when we receive a Connected event
}

//  Per-Tick 

void ClientSession::Tick(float deltaTime) {
    ProcessServerSnapshots();
    SendInput();
    Predict();

    // 5. Push smoothed position back to the player entity (for rendering and camera)
    // TODO: Extremely laggy because ping how can we do this better.
    // if (_localEntityManager && _localPlayerEntity.IsValid()) {
    //     Entity* entity = _localEntityManager->GetEntity(_localPlayerEntity);
    //     if (entity) {
    //         entity->SetPosition(_visualPosition);
    //     }
    // }

    // Advance interpolation for remote entities
    if (_interpolator) {
        _interpolator->Update(deltaTime);
    }
}

//  Input 

void ClientSession::SetLocalInput(const PlayerInputState& input) {
    // Store in ring buffer for replay during reconciliation
    uint32_t index = _currentTick % INPUT_BUFFER_SIZE;
    _inputHistory[index] = input;
    _inputHistory[index].tick = _currentTick;
}

void ClientSession::UpdateVisualSmoothing(float deltaTime)
{
    float dx = _simulationPosition.x - _visualPosition.x;
    float dy = _simulationPosition.y - _visualPosition.y;
    float dz = _simulationPosition.z - _visualPosition.z;
    float distSq = dx * dx + dy * dy + dz * dz;

    if (distSq > SNAP_THRESHOLD * SNAP_THRESHOLD) {
        _visualPosition = _simulationPosition;
        return;
    }

    // Frame-rate independent smoothing
    float t = 1.0f - std::pow(1.0f - VISUAL_SMOOTH_RATE, deltaTime * 60.0f);
    _visualPosition.x += dx * t;
    _visualPosition.y += dy * t;
    _visualPosition.z += dz * t;
}

//  Rendering State 

Math::Vector3 ClientSession::GetLocalPlayerPosition() const {
    return _simulationPosition;
}

Math::Vector3 ClientSession::GetLocalPlayerVisualPosition() const {
    return _visualPosition;
}

bool ClientSession::IsLocalPlayerOnGround() const {
    if (!_predictionPhysics || !_predictionBody.IsValid()) return false;
    const VoxelBody* body = _predictionPhysics->GetBody(_predictionBody);
    return body ? body->grounded : false;
}

Math::Vector3 ClientSession::GetEntityPosition(EntityID id) const {
    if (_interpolator) return _interpolator->GetPosition(id);
    return Math::Vector3();
}

float ClientSession::GetEntityYaw(EntityID id) const {
    if (_interpolator) return _interpolator->GetYaw(id);
    return 0.0f;
}

bool ClientSession::IsEntityVisible(EntityID id) const {
    if (_interpolator) return _interpolator->HasEntity(id);
    return false;
}

float ClientSession::GetRTT() const {
    // TODO: implement ping/pong measurement
    return 0.0f;
}

//  Internal: Process Server Snapshots 

void ClientSession::ProcessServerSnapshots() {
    if (!_transport) return;

    std::vector<NetworkEvent> events;
    _transport->PollEvents(events);

    for (const auto& event : events) {
        switch (event.type) {

        case NetworkEvent::Type::Connected:
            _connected = true;
            _serverConnectionId = event.connection;
            std::cout << "[Client] Connected to server" << std::endl;
            break;

        case NetworkEvent::Type::Disconnected:
            _connected = false;
            std::cout << "[Client] Disconnected from server" << std::endl;
            break;

        case NetworkEvent::Type::DataReceived:
            if (event.data.empty()) break;

            PacketType type = static_cast<PacketType>(event.data[0]);
            switch (type) {

            case PacketType::Snapshot: {
                    ServerSnapshot snapshot;
                    if (PacketSerializer::DeserializeSnapshot(event.data, snapshot)) {
                        Reconcile(snapshot);
                        float timestamp = static_cast<float>(snapshot.serverTick) / 60.0f;
                        for (const auto& e : snapshot.entities) {
                            _interpolator->PushState(e.id, timestamp, e);
                        }
                    }
                    break;
            }

            case PacketType::ChunkData: {
                    PacketSerializer::ChunkPacketData chunkData;
                    if (PacketSerializer::DeserializeChunk(event.data, chunkData)) {
                        // TODO: cant we serialize blocks and copy it to memory space of new chunk?
                        if (!chunkData.isEmpty && _localChunkManager) {
                            Chunk* chunk = new Chunk(Math::Vector3(chunkData.chunkX, chunkData.chunkY, chunkData.chunkZ));
                            for (uint32_t y = 0; y < Chunk::CHUNK_SIZE; y++)
                                for (uint32_t z = 0; z < Chunk::CHUNK_SIZE; z++)
                                    for (uint32_t x = 0; x < Chunk::CHUNK_SIZE; x++)
                                        chunk->SetBlock(x, y, z, chunkData.blocks[x + z * Chunk::CHUNK_SIZE + y * Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE]);

                            _localChunkManager->AddChunk(chunk->GetWorldPosition(), chunk);
                            //_localChunkManager->GenerateChunkMesh(chunk);
                        }
                        _chunksReceived++;
                    }
                    break;
            }

            case PacketType::PlayerSpawn: {
                    std::cout << "[Client] Received PlayerSpawn packet" << std::endl;
                    // Server telling us which entity is our player
                    PacketSerializer::PlayerSpawnData spawnData;
                    if (PacketSerializer::DeserializePlayerSpawn(event.data, spawnData))
                    {
                        assert(_localEntityManager.get() == nullptr);
                        _localEntityManager = std::make_unique<EntityManager>(spawnData.playerId);
                        EntityID playerId = _localEntityManager->CreateEntityWithID<PlayerEntity>(spawnData.playerId, "LocalPlayer");


                        _spawnPosition = spawnData.spawnPos;
                        _hasReceivedSpawn = true;

                        SetLocalPlayerEntity(playerId);

                        std::cout << "[Client] Assigned player entity "
                                    << playerId.Get() << " at ("
                                    << _spawnPosition.x << ", " << _spawnPosition.y << ", "
                                    << _spawnPosition.z << ")" << std::endl;
                    }
                    break;
            }

            case PacketType::EntitySpawn: {
                // TODO: full entity state for new entity entering AOI
                break;
            }

            case PacketType::EntityDespawn: {
                // Entity left our AOI
                if (_interpolator && event.data.size() >= 1 + sizeof(EntityID)) {
                    EntityID id;
                    std::memcpy(&id, event.data.data() + 1, sizeof(EntityID));
                    _interpolator->RemoveEntity(id);
                }
                break;
            }

            default:
                break;
            }
            break;
        }
    }
}

//  Internal: Send Input 

void ClientSession::SendInput() {
    if (!_transport || !_connected) return;

    PlayerInputState inputs[3];
    for (int i = 0; i < 3; i++) {
        uint32_t idx = (_currentTick - i) % INPUT_BUFFER_SIZE;
        inputs[2 - i] = _inputHistory[idx]; // oldest first
    }

    auto packetData = PacketSerializer::SerializeInputs(inputs, 3);
    _transport->SendPacket(_serverConnectionId, packetData, 0, SendMode::UnreliableSequenced);
}

//  Internal: Prediction 

void ClientSession::Predict() {
    if (!_predictionPhysics || !_predictionBody.IsValid()) return;

    uint32_t index = _currentTick % INPUT_BUFFER_SIZE;
    const PlayerInputState& input = _inputHistory[index];

    VoxelBody* body = _predictionPhysics->GetBody(_predictionBody);
    if (!body) return;

    // Apply input to prediction body (same math as ServerInstance::ApplyClientInputs)
    float sinYaw = std::sin(input.yaw);
    float cosYaw = std::cos(input.yaw);

    float worldMoveX = input.moveX * cosYaw - input.moveZ * sinYaw;
    float worldMoveZ = input.moveX * sinYaw + input.moveZ * cosYaw;

    body->inputVelocity.x = worldMoveX * 4.0f; // TODO: get move speed from player config
    body->inputVelocity.z = worldMoveZ * 4.0f;

    if ((input.buttons & InputButton::Jump) && body->grounded) {
        body->velocity.y = 8.0f; // TODO: get jump force from player config
    }

    // Step physics exactly one tick
    float fixedDt = _predictionPhysics->GetConfig().fixedTimestep;
    _predictionPhysics->Update(fixedDt);

    // Save predicted state for later reconciliation
    _stateHistory[index].tick = _currentTick;
    _stateHistory[index].position = body->position;
    _stateHistory[index].velocity = body->velocity;
    _stateHistory[index].onGround = body->grounded;

    _simulationPosition = body->position;
    _currentTick++;
}

//  Internal: Reconciliation 

void ClientSession::Reconcile(const ServerSnapshot& snapshot) {
    if (!_predictionPhysics || !_predictionBody.IsValid()) return;

    uint32_t serverTick = snapshot.lastProcessedInput;

    // Find our predicted state at the tick the server is confirming
    uint32_t index = serverTick % INPUT_BUFFER_SIZE;
    const PredictedState& predicted = _stateHistory[index];

    // If we don't have a prediction for this tick, skip
    // (can happen during initial connection)
    if (predicted.tick != serverTick) return;

    // Compare server's authoritative position vs our prediction
    float dx = snapshot.playerPosition.x - predicted.position.x;
    float dy = snapshot.playerPosition.y - predicted.position.y;
    float dz = snapshot.playerPosition.z - predicted.position.z;
    float errorSq = dx * dx + dy * dy + dz * dz;

    if (errorSq < RECONCILE_EPSILON * RECONCILE_EPSILON) {
        // Prediction was correct — nothing to do
        _lastAckedServerTick = serverTick;
        return;
    }

    // Prediction was wrong — snap to server state and replay inputs

    VoxelBody* body = _predictionPhysics->GetBody(_predictionBody);
    if (!body) return;

    // Rewind to server's authoritative state
    body->position = snapshot.playerPosition;
    body->velocity = snapshot.playerVelocity;
    body->grounded = snapshot.playerOnGround;

    float fixedDt = _predictionPhysics->GetConfig().fixedTimestep;

    // Replay all inputs from (serverTick + 1) through (_currentTick - 1)
    // These are inputs the server hasn't processed yet
    if (serverTick + 1 >= _currentTick) {
        _simulationPosition = body->position;
        _lastAckedServerTick = serverTick;
        return;
    }
    for (uint32_t tick = serverTick + 1; tick < _currentTick; tick++) {
        uint32_t replayIndex = tick % INPUT_BUFFER_SIZE;
        const PlayerInputState& input = _inputHistory[replayIndex];

        // Apply input (same as Predict)
        float sinYaw = std::sin(input.yaw);
        float cosYaw = std::cos(input.yaw);

        float worldMoveX = input.moveX * cosYaw - input.moveZ * sinYaw;
        float worldMoveZ = input.moveX * sinYaw + input.moveZ * cosYaw;

        body->inputVelocity.x = worldMoveX * 4.0f; // TODO: move speed
        body->inputVelocity.z = worldMoveZ * 4.0f;

        if ((input.buttons & InputButton::Jump) && body->grounded) {
            body->velocity.y = 8.0f; // TODO: jump force
        }

        // Step one tick
        _predictionPhysics->Update(fixedDt);

        // Update state history with corrected prediction
        _stateHistory[replayIndex].tick = tick;
        _stateHistory[replayIndex].position = body->position;
        _stateHistory[replayIndex].velocity = body->velocity;
        _stateHistory[replayIndex].onGround = body->grounded;
    }

    // Update simulation position to the replayed result
    _simulationPosition = body->position;
    _lastAckedServerTick = serverTick;

    //std::cout << "[Client] Position " << body->position << std::endl;

    // float error = std::sqrt(errorSq);
    // if (error > 0.1f) {
    //     std::cout << "[Client] Reconciled: error=" << error
    //               << " replayed " << (_currentTick - serverTick - 1) << " ticks" << std::endl;
    // }
}

//  Internal: Visual Smoothing 

void ClientSession::SmoothVisualPosition(float dt) {
    // If error is huge (teleport, respawn), snap immediately
    float dx = _simulationPosition.x - _visualPosition.x;
    float dy = _simulationPosition.y - _visualPosition.y;
    float dz = _simulationPosition.z - _visualPosition.z;
    float distSq = dx * dx + dy * dy + dz * dz;

    if (distSq > SNAP_THRESHOLD * SNAP_THRESHOLD) {
        _visualPosition = _simulationPosition;
        return;
    }

    // Lerp toward simulation position
    _visualPosition.x += dx * VISUAL_SMOOTH_RATE;
    _visualPosition.y += dy * VISUAL_SMOOTH_RATE;
    _visualPosition.z += dz * VISUAL_SMOOTH_RATE;
}