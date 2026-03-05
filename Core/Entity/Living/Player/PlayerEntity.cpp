//
// Created by Marvin on 10/02/2026.
//

#include "PlayerEntity.h"

#include "Core/Network/Client/ClientSession.h"
#include "Core/Physics/Voxtau/VoxelPhysics.h"
#include "Resources/Animation/Animator.h"

PlayerEntity::PlayerEntity(const std::string& name)
    : LivingEntity(EntityType::Player, name)
{
    _moveSpeed = 4.0f;
    _maxHealth = 100.0f;
    SetCurrentHealth(100.0f);
}

PlayerEntity::~PlayerEntity() = default;

void PlayerEntity::BindPhysics(VoxelPhysics* physics, VoxelBodyID bodyId) {
    _physics = physics;
    _bodyId = bodyId;
}

void PlayerEntity::Update(float deltaTime) {
    LivingEntity::Update(deltaTime);

    if (_animator) {
        _animator->Update(deltaTime);

        float horizontalSpeedSq = _velocity.x * _velocity.x + _velocity.z * _velocity.z;
        bool moving = horizontalSpeedSq > 0.01f;
        const std::string& current = _animator->GetCurrentClipName();

        if (moving && current != "walking")
            _animator->Play("walking");
        else if (!moving && current != "idle")
            _animator->Play("idle");
    }

    // Only server has Physics.
    if (!_physics || !_bodyId.IsValid()) return;

    // Read authoritative position from physics body
    const VoxelBody* body = _physics->GetBody(_bodyId);
    if (!body) return;

    SetPosition(body->position);

    // Update movement state from physics
    if (body->grounded) {
        if (_movementState == MovementState::Jumping || _movementState == MovementState::Falling) {
            _movementState = MovementState::Grounded;
        }
    } else {
        if (_movementState != MovementState::Jumping) {
            _movementState = MovementState::Falling;
        }
        // Transition from Jumping to Falling when velocity turns downward
        if (_movementState == MovementState::Jumping && body->velocity.y <= 0.0f) {
            _movementState = MovementState::Falling;
        }
    }
}

void PlayerEntity::MoveInput(const Math::Vector3& direction, float deltaTime) {
    if (!_physics || !_bodyId.IsValid()) return;

    VoxelBody* body = _physics->GetBody(_bodyId);
    if (!body) return;

    // Set input velocity — physics system will use this during the next step.
    // Direction is already normalized by PlayerController.
    body->inputVelocity.x = direction.x * GetMoveSpeed();
    body->inputVelocity.z = direction.z * GetMoveSpeed();
}

void PlayerEntity::Jump() {
    if (!_physics || !_bodyId.IsValid()) return;

    VoxelBody* body = _physics->GetBody(_bodyId);
    if (!body) return;

    // Only jump if grounded
    if (body->grounded) {
        body->velocity.y = _jumpForce;
        _movementState = MovementState::Jumping;
    }
}

void PlayerEntity::OnDeath(EntityID killedBy) {
    // TODO: death animation, respawn timer
    Respawn();
}

void PlayerEntity::Respawn() {
    if (!_physics || !_bodyId.IsValid()) return;

    VoxelBody* body = _physics->GetBody(_bodyId);
    if (!body) return;

    body->position = _respawnPosition;
    body->velocity = Math::Vector3(0, 0, 0);
    _movementState = MovementState::Falling;

    SetCurrentHealth(GetMaxHealth());
    SetPosition(_respawnPosition);

    OnRespawn();
}

RenderData PlayerEntity::GetRenderData() const
{
    RenderData rd = _renderData;
    rd.worldMatrix = _transform.GetWorldMatrix();
    rd.animator = _animator;
    return rd;
}
