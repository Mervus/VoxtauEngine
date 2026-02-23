//
// Created by Marvin on 11/02/2026.
//

#include "VoxelPhysics.h"
#include "Core/Voxel/ChunkManager.h"
#include <Core/Physics/Voxtau/AABB.h>

#include <cmath>
#include <algorithm>
#include <iostream>
#include <limits>

#include "Core/Profiler/Profiler.h"

// Construction / Initialization

VoxelPhysics::VoxelPhysics() = default;

VoxelPhysics::~VoxelPhysics() {
    Shutdown();
}

void VoxelPhysics::Initialize(ChunkManager* chunkManager) {
    if (!chunkManager) {
        std::cerr << "[VoxelPhysics] ERROR: null ChunkManager" << std::endl;
        return;
    }

    // Wrap ChunkManager::IsSolidAt into a lambda.
    // This is the only coupling to ChunkManager — the server can provide
    // its own query function via the other Initialize() overload.
    Initialize([chunkManager](int x, int y, int z) -> bool {
        return chunkManager->IsSolidAt(x, y, z);
    });
}

void VoxelPhysics::Initialize(VoxelSolidQuery solidQuery) {
    _solidQuery = std::move(solidQuery);
    _bodies.clear();
    _nextBodyId = 1;
    _accumulator = 0.0f;

    std::cout << "[VoxelPhysics] Initialized" << std::endl;
}

void VoxelPhysics::Shutdown() {
    _bodies.clear();
    _solidQuery = nullptr;
    std::cout << "[VoxelPhysics] Shutdown" << std::endl;
}

// Body Management
VoxelBodyID VoxelPhysics::CreateBody(const Math::Vector3& position, float width, float height) {
    VoxelBodyID id(_nextBodyId++);

    VoxelBody body;
    body.id = id;
    body.position = position;
    body.width = width;
    body.height = height;
    body.gravity = _config.defaultGravity;

    _bodies[id] = body;
    return id;
}

void VoxelPhysics::DestroyBody(VoxelBodyID id) {
    _bodies.erase(id);
}

VoxelBody* VoxelPhysics::GetBody(VoxelBodyID id) {
    auto it = _bodies.find(id);
    return it != _bodies.end() ? &it->second : nullptr;
}

const VoxelBody* VoxelPhysics::GetBody(VoxelBodyID id) const {
    auto it = _bodies.find(id);
    return it != _bodies.end() ? &it->second : nullptr;
}

// Simulation
void VoxelPhysics::Update(float deltaTime) {
    PROFILE_FUNCTION();
    if (!_solidQuery) return;

    // Clamp incoming dt to prevent huge accumulations (e.g. after breakpoint)
    deltaTime = std::min(deltaTime, _config.fixedTimestep * _config.maxStepsPerFrame);

    _accumulator += deltaTime;

    int steps = 0;
    while (_accumulator >= _config.fixedTimestep && steps < _config.maxStepsPerFrame) {
        for (auto& [id, body] : _bodies) {
            if (body.active) {
                StepBody(body, _config.fixedTimestep);
            }
        }
        _accumulator -= _config.fixedTimestep;
        steps++;
    }
}

void VoxelPhysics::StepBody(VoxelBody& body, float dt) {
    // 1. Apply gravity to persistent velocity
    if (body.affectedByGravity) {
        body.velocity.y += body.gravity * dt;

        // Clamp to terminal velocity
        if (body.velocity.y < body.maxFallSpeed) {
            body.velocity.y = body.maxFallSpeed;
        }
    }

    // 2. If grounded and not jumping, zero out downward velocity
    //    (prevents accumulating gravity while standing on ground)
    if (body.grounded && body.velocity.y < 0.0f) {
        body.velocity.y = 0.0f;
    }

    // 3. Combine velocities into this tick's movement delta
    Math::Vector3 totalVelocity = body.velocity + body.inputVelocity;
    Math::Vector3 delta = totalVelocity * dt;

    // 4. Clear input velocity (it's consumed each tick)
    body.inputVelocity = Math::Vector3(0, 0, 0);

    // 5. Reset collision flags
    body.hitWallX = false;
    body.hitWallZ = false;
    body.hitCeiling = false;

    // 6. Per-axis sweep: Y first (gravity), then X, then Z
    //    Y first so we know if we're grounded before horizontal movement,
    //    which matters for auto-step.
    Physics::AABB currentAABB = body.GetAABB();

    float actualY = SweepAxis(body, currentAABB, 1, delta.y);  // Y
    float actualX = SweepAxis(body, currentAABB, 0, delta.x);  // X
    float actualZ = SweepAxis(body, currentAABB, 2, delta.z);  // Z

    // 7. Update position from the resolved AABB
    float hw = body.HalfWidth();
    body.position.x = currentAABB.min.x + hw;
    body.position.y = currentAABB.min.y;
    body.position.z = currentAABB.min.z + hw;

    // 8. Handle collision responses on velocity
    if (body.hitWallX) body.velocity.x = 0.0f;
    if (body.hitWallZ) body.velocity.z = 0.0f;
    if (body.hitCeiling) {
        // Bonk head — zero upward velocity
        if (body.velocity.y > 0.0f) body.velocity.y = 0.0f;
    }

    // 9. Ground check
    body.grounded = CheckGrounded(body);

    // If we landed (Y was moving down and got blocked), zero Y velocity
    if (body.grounded && body.velocity.y < 0.0f) {
        body.velocity.y = 0.0f;
    }
}



// Per-Axis Sweep
float VoxelPhysics::SweepAxis(VoxelBody& body, Physics::AABB& currentAABB, int axis, float delta) {
    if (std::abs(delta) < 1e-6f) return 0.0f;

    // Expand the AABB in the movement direction to find candidate blocks
    Math::Vector3 deltaVec(0, 0, 0);
    if (axis == 0) deltaVec.x = delta;
    else if (axis == 1) deltaVec.y = delta;
    else deltaVec.z = delta;

    Physics::AABB sweptRegion = currentAABB.Expanded(deltaVec);

    // Collect all solid blocks overlapping the swept region
    std::vector<Physics::AABB> solidBlocks;
    CollectSolidBlocks(sweptRegion, solidBlocks);

    // For each solid block, clamp the delta so we don't penetrate
    float remaining = delta;

    for (const Physics::AABB& block : solidBlocks) {
        if (std::abs(remaining) < 1e-6f) break;

        // Check if block overlaps on the OTHER two axes
        // (we only resolve on the sweep axis)
        bool overlapsOther;
        if (axis == 0) {
            overlapsOther = currentAABB.min.y < block.max.y && currentAABB.max.y > block.min.y &&
                            currentAABB.min.z < block.max.z && currentAABB.max.z > block.min.z;
        } else if (axis == 1) {
            overlapsOther = currentAABB.min.x < block.max.x && currentAABB.max.x > block.min.x &&
                            currentAABB.min.z < block.max.z && currentAABB.max.z > block.min.z;
        } else {
            overlapsOther = currentAABB.min.x < block.max.x && currentAABB.max.x > block.min.x &&
                            currentAABB.min.y < block.max.y && currentAABB.max.y > block.min.y;
        }

        if (!overlapsOther) continue;

        if (remaining > 0.0f) {
            // Moving in positive direction: clamp to block's min face
            float maxDist;
            if (axis == 0) maxDist = block.min.x - currentAABB.max.x;
            else if (axis == 1) maxDist = block.min.y - currentAABB.max.y;
            else maxDist = block.min.z - currentAABB.max.z;

            if (maxDist < remaining) {
                remaining = std::max(maxDist, 0.0f);
            }
        } else {
            // Moving in negative direction: clamp to block's max face
            float maxDist;
            if (axis == 0) maxDist = block.max.x - currentAABB.min.x;
            else if (axis == 1) maxDist = block.max.y - currentAABB.min.y;
            else maxDist = block.max.z - currentAABB.min.z;

            if (maxDist > remaining) {
                remaining = std::min(maxDist, 0.0f);
            }
        }
    }

    // If we were blocked, try auto-step for horizontal axes
    bool blocked = std::abs(remaining) < std::abs(delta) - 1e-5f;
    if (blocked && axis != 1) {
        // Attempt auto-step before committing the clamped movement
        if (TryAutoStep(body, currentAABB, axis, delta)) {
            // Auto-step succeeded — it already updated currentAABB
            return delta;
        }
    }

    // Apply the clamped movement to the AABB
    if (axis == 0) {
        currentAABB.min.x += remaining;
        currentAABB.max.x += remaining;
        if (blocked) body.hitWallX = true;
    } else if (axis == 1) {
        currentAABB.min.y += remaining;
        currentAABB.max.y += remaining;
        if (blocked) {
            if (delta > 0.0f) body.hitCeiling = true;
            // grounded is set separately via CheckGrounded
        }
    } else {
        currentAABB.min.z += remaining;
        currentAABB.max.z += remaining;
        if (blocked) body.hitWallZ = true;
    }

    return remaining;
}

// Auto-Step
bool VoxelPhysics::TryAutoStep(VoxelBody& body, Physics::AABB& currentAABB, int axis, float delta) {
    if (body.stepHeight <= 0.0f) return false;
    if (!body.grounded) return false; // Only step up when on the ground

    // Step 1: Lift the AABB up by stepHeight
    Physics::AABB lifted = currentAABB;
    lifted.min.y += body.stepHeight;
    lifted.max.y += body.stepHeight;

    // Check that the lifted position itself is clear
    std::vector<Physics::AABB> liftBlocks;
    CollectSolidBlocks(lifted, liftBlocks);
    for (const Physics::AABB& block : liftBlocks) {
        if (lifted.Overlaps(block)) return false; // Can't lift — something above
    }

    // Step 2: Try the horizontal move at the lifted height
    Math::Vector3 liftDeltaVec(0, 0, 0);
    if (axis == 0) liftDeltaVec.x = delta;
    else liftDeltaVec.z = delta;

    Physics::AABB liftSwept = lifted.Expanded(liftDeltaVec);
    std::vector<Physics::AABB> moveBlocks;
    CollectSolidBlocks(liftSwept, moveBlocks);

    float moveDelta = delta;
    for (const Physics::AABB& block : moveBlocks) {
        if (std::abs(moveDelta) < 1e-6f) break;

        // Check overlap on non-sweep axes
        bool overlaps;
        if (axis == 0) {
            overlaps = lifted.min.y < block.max.y && lifted.max.y > block.min.y &&
                       lifted.min.z < block.max.z && lifted.max.z > block.min.z;
        } else {
            overlaps = lifted.min.x < block.max.x && lifted.max.x > block.min.x &&
                       lifted.min.y < block.max.y && lifted.max.y > block.min.y;
        }
        if (!overlaps) continue;

        if (moveDelta > 0.0f) {
            float maxDist = (axis == 0) ? block.min.x - lifted.max.x : block.min.z - lifted.max.z;
            if (maxDist < moveDelta) moveDelta = std::max(maxDist, 0.0f);
        } else {
            float maxDist = (axis == 0) ? block.max.x - lifted.min.x : block.max.z - lifted.min.z;
            if (maxDist > moveDelta) moveDelta = std::min(maxDist, 0.0f);
        }
    }

    // If still fully blocked at the lifted height, auto-step fails
    if (std::abs(moveDelta) < 1e-5f) return false;

    // Step 3: Apply the horizontal move at lifted height
    if (axis == 0) { lifted.min.x += moveDelta; lifted.max.x += moveDelta; }
    else { lifted.min.z += moveDelta; lifted.max.z += moveDelta; }

    // Step 4: Snap back down to ground (sweep Y downward by stepHeight)
    float dropDelta = -body.stepHeight;
    Physics::AABB dropSwept = lifted.Expanded(Math::Vector3(0, dropDelta, 0));
    std::vector<Physics::AABB> dropBlocks;
    CollectSolidBlocks(dropSwept, dropBlocks);

    float dropRemaining = dropDelta;
    for (const Physics::AABB& block : dropBlocks) {
        if (std::abs(dropRemaining) < 1e-6f) break;

        bool overlaps = lifted.min.x < block.max.x && lifted.max.x > block.min.x &&
                        lifted.min.z < block.max.z && lifted.max.z > block.min.z;
        if (!overlaps) continue;

        float maxDist = block.max.y - lifted.min.y;
        if (maxDist > dropRemaining) {
            dropRemaining = std::min(maxDist, 0.0f);
        }
    }

    lifted.min.y += dropRemaining;
    lifted.max.y += dropRemaining;

    // Commit: the body is now at the stepped-up position
    currentAABB = lifted;
    return true;
}

// Ground Check
bool VoxelPhysics::CheckGrounded(const VoxelBody& body) const {
    // Probe slightly below the body's feet
    constexpr float GROUND_PROBE = 0.04f;

    Physics::AABB feetProbe = body.GetAABB();
    feetProbe.max.y = feetProbe.min.y + GROUND_PROBE;
    feetProbe.min.y -= GROUND_PROBE;

    // Check all blocks the probe overlaps
    int minBX = static_cast<int>(std::floor(feetProbe.min.x));
    int maxBX = static_cast<int>(std::floor(feetProbe.max.x - 1e-5f));
    int minBZ = static_cast<int>(std::floor(feetProbe.min.z));
    int maxBZ = static_cast<int>(std::floor(feetProbe.max.z - 1e-5f));
    int probeY = static_cast<int>(std::floor(feetProbe.min.y));

    for (int bx = minBX; bx <= maxBX; bx++) {
        for (int bz = minBZ; bz <= maxBZ; bz++) {
            if (_solidQuery(bx, probeY, bz)) {
                // Verify the block's top face is near our feet
                float blockTop = static_cast<float>(probeY + 1);
                if (std::abs(body.position.y - blockTop) < GROUND_PROBE * 2.0f) {
                    return true;
                }
            }
        }
    }

    return false;
}

// Block Collection

void VoxelPhysics::CollectSolidBlocks(const Physics::AABB& region, std::vector<Physics::AABB>& outBlocks) const {
    outBlocks.clear();

    // Iterate all block positions that overlap the region
    // floor() for min, ceil()-1 for max gives us the block coordinate range
    int minBX = static_cast<int>(std::floor(region.min.x));
    int minBY = static_cast<int>(std::floor(region.min.y));
    int minBZ = static_cast<int>(std::floor(region.min.z));

    // For max: a value of exactly 5.0 should NOT include block 5
    // (block 5 spans [5, 6)), so we use floor(max - epsilon)
    int maxBX = static_cast<int>(std::floor(region.max.x - 1e-5f));
    int maxBY = static_cast<int>(std::floor(region.max.y - 1e-5f));
    int maxBZ = static_cast<int>(std::floor(region.max.z - 1e-5f));

    for (int by = minBY; by <= maxBY; by++) {
        for (int bz = minBZ; bz <= maxBZ; bz++) {
            for (int bx = minBX; bx <= maxBX; bx++) {
                if (_solidQuery(bx, by, bz)) {
                    outBlocks.push_back(Physics::AABB::FromBlock(bx, by, bz));
                }
            }
        }
    }
}

// World Queries

bool VoxelPhysics::IsSolid(int x, int y, int z) const {
    return _solidQuery && _solidQuery(x, y, z);
}

bool VoxelPhysics::Raycast(const Math::Vector3& origin, const Math::Vector3& direction, float maxDistance,
                            Math::Vector3& outHitPos, Math::Vector3& outHitNormal) const {
    if (!_solidQuery) return false;

    // DDA (Digital Differential Analyzer) raycast through the voxel grid
    // This is the standard algorithm for raycasting in voxel worlds

    Math::Vector3 dir = direction.Normalized();

    // Current voxel position
    int x = static_cast<int>(std::floor(origin.x));
    int y = static_cast<int>(std::floor(origin.y));
    int z = static_cast<int>(std::floor(origin.z));

    // Step direction (+1 or -1)
    int stepX = (dir.x >= 0) ? 1 : -1;
    int stepY = (dir.y >= 0) ? 1 : -1;
    int stepZ = (dir.z >= 0) ? 1 : -1;

    // Distance along ray to cross one voxel boundary on each axis
    float tDeltaX = (std::abs(dir.x) > 1e-8f) ? std::abs(1.0f / dir.x) : 1e30f;
    float tDeltaY = (std::abs(dir.y) > 1e-8f) ? std::abs(1.0f / dir.y) : 1e30f;
    float tDeltaZ = (std::abs(dir.z) > 1e-8f) ? std::abs(1.0f / dir.z) : 1e30f;

    // Distance to next boundary
    float tMaxX, tMaxY, tMaxZ;
    if (dir.x >= 0) tMaxX = (static_cast<float>(x + 1) - origin.x) * tDeltaX / (1.0f); // simplified
    else tMaxX = (origin.x - static_cast<float>(x)) * tDeltaX;
    // Recalculate properly:
    tMaxX = (dir.x >= 0)
        ? ((static_cast<float>(x + 1) - origin.x) / std::abs(dir.x))
        : ((origin.x - static_cast<float>(x)) / std::abs(dir.x));
    tMaxY = (dir.y >= 0)
        ? ((static_cast<float>(y + 1) - origin.y) / std::abs(dir.y))
        : ((origin.y - static_cast<float>(y)) / std::abs(dir.y));
    tMaxZ = (dir.z >= 0)
        ? ((static_cast<float>(z + 1) - origin.z) / std::abs(dir.z))
        : ((origin.z - static_cast<float>(z)) / std::abs(dir.z));

    // Handle zero-direction axes
    if (std::abs(dir.x) < 1e-8f) tMaxX = 1e30f;
    if (std::abs(dir.y) < 1e-8f) tMaxY = 1e30f;
    if (std::abs(dir.z) < 1e-8f) tMaxZ = 1e30f;

    float t = 0.0f;
    Math::Vector3 lastNormal(0, 0, 0);

    while (t < maxDistance) {
        if (_solidQuery(x, y, z)) {
            outHitPos = origin + dir * t;
            outHitNormal = lastNormal;
            return true;
        }

        // Advance along the axis with the smallest tMax
        if (tMaxX < tMaxY && tMaxX < tMaxZ) {
            t = tMaxX;
            x += stepX;
            tMaxX += tDeltaX;
            lastNormal = Math::Vector3(static_cast<float>(-stepX), 0, 0);
        } else if (tMaxY < tMaxZ) {
            t = tMaxY;
            y += stepY;
            tMaxY += tDeltaY;
            lastNormal = Math::Vector3(0, static_cast<float>(-stepY), 0);
        } else {
            t = tMaxZ;
            z += stepZ;
            tMaxZ += tDeltaZ;
            lastNormal = Math::Vector3(0, 0, static_cast<float>(-stepZ));
        }
    }

    return false;
}