//
// Created by Marvin on 27/02/2026.
//

#ifndef VOXTAU_CLIENTSESSION_H
#define VOXTAU_CLIENTSESSION_H
#pragma once

#include <memory>
#include <vector>
#include <cstdint>
#include "Core/Entity/EntityID.h"
#include "Core/Math/Vector3.h"
#include "Core/Physics/Voxtau/VoxelBody.h"
#include "Core/Physics/Voxtau/VoxelPhysics.h"

#include "Core/Network/Protocol/PlayerInputState.h"
#include "Core/Network/Server/ClientProxy.h"

struct ServerSnapshot;
class INetworkTransport;
class VoxelPhysics;
class ChunkManager;
class EntityManager;
class EntityInterpolator;

class ClientSession
{
public:
    ClientSession();
    ~ClientSession();

    // Setup
    void Initialize(INetworkTransport* transport);
    // Set up prediction physics. Call after Initialize, once we know
    // the spawn position and have access to world data for collision.
    // solidQuery: how the prediction physics checks for solid blocks
    //   - Singleplayer: share the server's ChunkManager::IsSolidAt
    //   - Remote: use client's local chunk cache
    void InitializePrediction(VoxelSolidQuery solidQuery,
                              const Math::Vector3& startPos,
                              float bodyWidth = 0.6f,
                              float bodyHeight = 1.8f);

    void Shutdown();

    // Connect to server (for remote mode — LocalTransport connects immediately)
    void Connect(const std::string& address, uint16_t port);

    // Per-tick

    // Full client tick: receive snapshots → send input → predict → reconcile
    void Tick(float deltaTime);

    // Input

    // Game code sets this each frame (from PlayerController)
    void SetLocalInput(const PlayerInputState& input);

    void UpdateVisualSmoothing(float deltaTime);

    // Local player — predicted position (smooth, no waiting for server)
    [[nodiscard]] Math::Vector3 GetLocalPlayerPosition() const;
    [[nodiscard]] Math::Vector3 GetLocalPlayerVisualPosition() const;  // smoothed for rendering
    bool IsLocalPlayerOnGround() const;

    // Remote entities — interpolated positions
    // The renderer reads these to draw other players/monsters
    [[nodiscard]] Math::Vector3 GetEntityPosition(EntityID id) const;
    [[nodiscard]] float GetEntityYaw(EntityID id) const;
    bool IsEntityVisible(EntityID id) const;

    // Info

    [[nodiscard]] EntityID GetLocalPlayerEntity() const { return _localPlayerEntity; }
    void SetLocalPlayerEntity(EntityID id) { _localPlayerEntity = id; }
    [[nodiscard]] uint32_t GetCurrentTick() const { return _currentTick; }
    float GetRTT() const;
    bool IsConnected() const { return _connected; }

    // Query for the scene's loading state
    bool HasReceivedSpawn() const { return _hasReceivedSpawn; }
    uint32_t GetChunksReceived() const { return _chunksReceived; }
    Math::Vector3 GetSpawnPosition() const { return _spawnPosition; }
    ChunkManager* GetLocalChunkManager(){return _localChunkManager.get(); }
    EntityManager* GetLocalEntityManager() { return _localEntityManager.get(); }

private:
    INetworkTransport* _transport = nullptr;  // not owned
    ConnectionID _serverConnectionId = 0;

    // Prediction state
    // Local copy of physics for prediction only
    // The server has the authoritative copy
    std::unique_ptr<VoxelPhysics> _predictionPhysics;
    VoxelBodyID _predictionBody;

    // Input history ring buffer (for replay during reconciliation)
    static constexpr size_t INPUT_BUFFER_SIZE = 128;
    PlayerInputState _inputHistory[INPUT_BUFFER_SIZE];
    uint32_t _inputHead = 0;

    // State history (for comparison during reconciliation)
    struct PredictedState {
        uint32_t tick;
        Math::Vector3 position;
        Math::Vector3 velocity;
        bool onGround;
    };
    PredictedState _stateHistory[INPUT_BUFFER_SIZE];

    // Visual smoothing
    Math::Vector3 _visualPosition;
    Math::Vector3 _simulationPosition;

    // Remote entity interpolation
    std::unique_ptr<EntityInterpolator> _interpolator;

    // Connection state
    EntityID _localPlayerEntity;
    uint32_t _currentTick = 0;
    uint32_t _lastAckedServerTick = 0;
    bool _connected = false;

    std::unique_ptr<ChunkManager> _localChunkManager;

    Math::Vector3 _spawnPosition;
    uint32_t _chunksReceived = 0;
    bool _hasReceivedSpawn = false;

    std::unique_ptr<EntityManager> _localEntityManager;

    //  Internal 
    void ProcessServerSnapshots();
    void SendInput();
    void Predict();
    void Reconcile(const ServerSnapshot& snapshot);
    void SmoothVisualPosition(float dt);
};


#endif //VOXTAU_CLIENTSESSION_H