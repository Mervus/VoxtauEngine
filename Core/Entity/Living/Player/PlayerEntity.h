//
// Created by Marvin on 10/02/2026.
//

#ifndef VOXTAU_PLAYER_H
#define VOXTAU_PLAYER_H
#pragma once
#include <EngineApi.h>
#include <Core/Entity/Living/LivingEntity.h>
#include <Core/Physics/Voxtau/VoxelBody.h>

class VoxelPhysics;

enum class MovementState : uint8_t {
    Grounded,
    Jumping,
    Falling,
    Swimming
};

class PlayerEntity : public LivingEntity {
protected:
    // Movement
    MovementState _movementState = MovementState::Falling;
    float _jumpForce = 8.0f;

    // Physics binding
    VoxelPhysics* _physics = nullptr;
    VoxelBodyID _bodyId;

    // Respawn
    Math::Vector3 _respawnPosition;

    void OnDeath(EntityID killedBy) override;

public:
    PlayerEntity(const std::string& name = "Player");
    virtual ~PlayerEntity();

    void Update(float deltaTime) override;

    // Physics Binding
    // Called by GameplayScene after creating both the entity and physics body
    void BindPhysics(VoxelPhysics* physics, VoxelBodyID bodyId);
    VoxelBodyID GetPhysicsBodyID() const { return _bodyId; }

    // Movement Interface
    // Called by PlayerController — doesn't read input directly

    // Sets the horizontal input velocity on the physics body
    virtual void MoveInput(const Math::Vector3& direction, float deltaTime);
    virtual void Jump();

    MovementState GetMovementState() const { return _movementState; }

    // Jump
    float GetJumpForce() const { return _jumpForce; }
    void SetJumpForce(float force) { _jumpForce = force; }

    // Respawn
    void SetRespawnPosition(const Math::Vector3& pos) { _respawnPosition = pos; }
    Math::Vector3 GetRespawnPosition() const { return _respawnPosition; }
    virtual void Respawn();

    virtual void OnRespawn() {}

    // Replication
    // virtual void SerializeNetState(ByteBuffer& buffer) override;
    // virtual void DeserializeNetState(ByteBuffer& buffer) override;
};

#endif //VOXTAU_PLAYER_H