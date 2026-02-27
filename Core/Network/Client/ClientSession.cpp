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

#include <cmath>
#include <cstring>
#include <iostream>

#include "Core/Network/Protocol/InputButton.h"

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
    // 1. Process snapshots from server
    ProcessServerSnapshots();

    // 2. Send our current input to the server
    SendInput();

    // 3. Run local prediction (apply input to local physics)
    Predict();

    // 4. Smooth visual position toward simulation position
    SmoothVisualPosition(deltaTime);

    // 5. Advance interpolation for remote entities
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
                // TODO: deserialize from event.data into ServerSnapshot
                // For now with LocalTransport, we'll need a shared struct approach
                // or proper serialization. Placeholder:
                //
                // ServerSnapshot snapshot;
                // DeserializeSnapshot(event.data, snapshot);
                // Reconcile(snapshot);
                //
                // Also feed remote entity states to interpolator:
                // for (auto& entityState : snapshot.entities) {
                //     _interpolator->PushState(entityState.id, entityState);
                // }
                break;
            }

            case PacketType::EntitySpawn: {
                // TODO: full entity state for new entity entering AOI
                break;
            }

            case PacketType::EntityDespawn: {
                // TODO: entity left AOI, remove from interpolator
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

    uint32_t index = _currentTick % INPUT_BUFFER_SIZE;
    const PlayerInputState& input = _inputHistory[index];

    // Packet layout: [PacketType(1)] [inputCount(1)] [PlayerInputState * N]
    // Send current + previous 2 inputs for redundancy (handles packet loss)
    constexpr uint8_t REDUNDANT_INPUTS = 3;
    uint8_t inputCount = static_cast<uint8_t>(
        std::min(static_cast<uint32_t>(REDUNDANT_INPUTS), _currentTick + 1));

    std::vector<uint8_t> packet;
    packet.reserve(2 + sizeof(PlayerInputState) * inputCount);
    packet.push_back(static_cast<uint8_t>(PacketType::ClientInput));
    packet.push_back(inputCount);

    // Oldest first so server can process in order
    for (int i = inputCount - 1; i >= 0; i--) {
        uint32_t tick = _currentTick - i;
        uint32_t idx = tick % INPUT_BUFFER_SIZE;
        const auto* src = reinterpret_cast<const uint8_t*>(&_inputHistory[idx]);
        packet.insert(packet.end(), src, src + sizeof(PlayerInputState));
    }

    _transport->SendPacket(1, packet, 0, SendMode::UnreliableSequenced);
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

    float error = std::sqrt(errorSq);
    if (error > 0.1f) {
        std::cout << "[Client] Reconciled: error=" << error
                  << " replayed " << (_currentTick - serverTick - 1) << " ticks" << std::endl;
    }
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