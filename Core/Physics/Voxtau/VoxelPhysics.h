//
// Created by Marvin on 11/02/2026.
//
// Owns all VoxelBodies. Steps at a fixed 60Hz regardless of framerate.
// Resolves AABB-vs-voxel collisions using per-axis sweep.
// Designed to run identically on client and server for multiplayer.
//
// Usage:
//   VoxelPhysics physics;
//   physics.Initialize(&chunkManager);
//   VoxelBodyID id = physics.CreateBody(startPos, 0.6f, 1.8f);
//   // Each frame:
//   physics.GetBody(id)->inputVelocity = moveDir * speed;
//   physics.Update(deltaTime);
//   entity.SetPosition(physics.GetBody(id)->position);
//

#ifndef VOXTAU_VOXELPHYSICS_H
#define VOXTAU_VOXELPHYSICS_H
#pragma once

#include <EngineApi.h>
#include <Core/Math/Vector3.h>
#include "VoxelBody.h"
#include "AABB.h"

#include <unordered_map>
#include <vector>
#include <functional>

class ChunkManager;

// World query function: (worldX, worldY, worldZ) -> is solid?
// This decouples VoxelPhysics from ChunkManager so the server
// can provide its own world data source.
using VoxelSolidQuery = std::function<bool(int, int, int)>;

struct ENGINE_API VoxelPhysicsConfig {
    float fixedTimestep = 1.0f / 60.0f;    // 60 Hz
    int   maxStepsPerFrame = 4;             // Prevent spiral of death
    float defaultGravity = -20.0f;
};

class ENGINE_API VoxelPhysics {
public:
    VoxelPhysics();
    ~VoxelPhysics();

    // Initialize with a ChunkManager (convenience — wraps IsSolidAt into a query)
    void Initialize(ChunkManager* chunkManager);

    // Initialize with a custom solid query (for server or testing)
    void Initialize(VoxelSolidQuery solidQuery);

    void Shutdown();

    // Body Management
    VoxelBodyID CreateBody(const Math::Vector3& position, float width, float height);
    void DestroyBody(VoxelBodyID id);

    VoxelBody* GetBody(VoxelBodyID id);
    const VoxelBody* GetBody(VoxelBodyID id) const;

    // Simulation

    // Call once per frame with real deltaTime. Internally steps at fixed rate.
    void Update(float deltaTime);

    // Configuration

    VoxelPhysicsConfig& GetConfig() { return _config; }
    const VoxelPhysicsConfig& GetConfig() const { return _config; }

    // Queries

    // Raycast against the voxel world. Returns true if hit, sets outHitPos and outHitNormal.
    bool Raycast(const Math::Vector3& origin, const Math::Vector3& direction, float maxDistance,
                 Math::Vector3& outHitPos, Math::Vector3& outHitNormal) const;

    // Check if a point is inside solid terrain
    bool IsSolid(int x, int y, int z) const;

    // Debug
    int GetBodyCount() const { return static_cast<int>(_bodies.size()); }

private:
    // Step a single body by fixedDt
    void StepBody(VoxelBody& body, float dt);

    // Per-axis sweep: move the body along one axis, clamping at solid blocks
    // Returns the actual distance moved
    float SweepAxis(VoxelBody& body, Physics::AABB& currentAABB, int axis, float delta);

    // Try to step up a ledge when blocked horizontally
    bool TryAutoStep(VoxelBody& body, Physics::AABB& currentAABB, int axis, float delta);

    // Ground check: probe slightly below the body
    bool CheckGrounded(const VoxelBody& body) const;

    // Collect all solid block AABBs that overlap a region
    void CollectSolidBlocks(const Physics::AABB& region, std::vector<Physics::AABB>& outBlocks) const;

    // State
    VoxelPhysicsConfig _config;
    VoxelSolidQuery _solidQuery;
    std::unordered_map<VoxelBodyID, VoxelBody, VoxelBodyIDHash> _bodies;
    uint32_t _nextBodyId = 1;
    float _accumulator = 0.0f;
};

#endif //VOXTAU_VOXELPHYSICS_H