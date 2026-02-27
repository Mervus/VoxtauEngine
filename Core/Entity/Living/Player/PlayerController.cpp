//
// Created by Marvin on 10/02/2026.
//

#include "PlayerController.h"
#include "PlayerEntity.h"
#include "Core/Input/InputManager.h"
#include "Core/Scene/Camera.h"
#include <cmath>

#include "Core/Entity/EntityManager.h"
#include "Core/Network/Client/ClientSession.h"
#include "Core/Network/Protocol/InputButton.h"
#include "Resources/Animation/Animator.h"

PlayerController::PlayerController(EntityManager* entityManager, InputManager* inputManager, Camera* camera, ClientSession* clientSession)
    : _entityManager(entityManager)
    , _inputManager(inputManager)
    , _camera(camera)
    , _yaw(0.0f)
    , _pitch(0.0f)
    , _mouseSensitivity(0.15f)
    , _cameraOffset(0.0f, 5.0f, -10.0f)
    , _thirdPerson(false)
    , _clientSession(clientSession)
{
}

PlayerController::~PlayerController() {
}

PlayerEntity* PlayerController::GetPlayer() const {
    if (!_entityManager || !_playerID.IsValid()) return nullptr;
    return _entityManager->GetEntityAs<PlayerEntity>(_playerID);
}

void PlayerController::SetThirdPerson(bool enabled, const Math::Vector3& offset) {
    _thirdPerson = enabled;
    _cameraOffset = offset;
}

void PlayerController::Update(float deltaTime) {
    if (!_inputManager || !_entityManager) return;

    PlayerEntity* player = GetPlayer();
    if (!player || player->IsDead()) return;

    if (player->GetAnimator())
    {
        player->GetAnimator()->Update(deltaTime);
    }

    // Toggle mouse capture
    if (_inputManager->IsKeyPressed(KeyCode::Escape)) {
        _inputManager->SetMouseCaptured(!_inputManager->IsMouseCaptured());
    }

    // Toggle debug camera
    if (_inputManager->IsKeyPressed(KeyCode::F3)) {
        _debugCamera = !_debugCamera;
    }

    if (_inputManager->IsMouseCaptured()) {
        HandleCamera(deltaTime);
        if (_debugCamera) {
            HandleDebugMovement(deltaTime);
        } else {
            HandleMovement(deltaTime);
        }
    }

    // Build input state AFTER HandleCamera so yaw/pitch are current
    PlayerInputState input{};
    input.tick = _clientSession->GetCurrentTick();
    input.yaw = _yaw * DEG_TO_RAD;
    input.pitch = _pitch * DEG_TO_RAD;
    input.buttons = 0;

    // Gather raw movement axes (no yaw rotation — server applies that)
    if (_inputManager->IsMouseCaptured() && !_debugCamera) {
        if (_inputManager->IsKeyDown(KeyCode::W)) input.moveZ += 1.0f;
        if (_inputManager->IsKeyDown(KeyCode::S)) input.moveZ -= 1.0f;
        if (_inputManager->IsKeyDown(KeyCode::D)) input.moveX += 1.0f;
        if (_inputManager->IsKeyDown(KeyCode::A)) input.moveX -= 1.0f;
        if (_inputManager->IsKeyDown(KeyCode::Space)) input.buttons |= InputButton::Jump;
    }

    _clientSession->SetLocalInput(input);
}

void PlayerController::LateUpdate(float deltaTime) {
    if (!_camera) return;

    // Debug camera is already positioned by HandleDebugMovement
    if (_debugCamera) return;

    PlayerEntity* player = GetPlayer();
    if (!player) return;

    Math::Vector3 playerPos = player->GetPosition();

    if (_thirdPerson) {
        float yawRad = _yaw * DEG_TO_RAD;

        Math::Vector3 offset;
        offset.x = _cameraOffset.z * std::sin(yawRad);
        offset.y = _cameraOffset.y;
        offset.z = _cameraOffset.z * std::cos(yawRad);

        _camera->SetPosition(playerPos + offset);
        _camera->SetLookAt(playerPos + Math::Vector3(0, 1.5f, 0));
    } else {
        Math::Vector3 cameraPos = playerPos + Math::Vector3(0, 1.7f, 0);
        _camera->SetPosition(cameraPos);
        _camera->SetRotation(_yaw * DEG_TO_RAD, _pitch * DEG_TO_RAD);
    }
}

void PlayerController::HandleCamera(float deltaTime) {
    double deltaX, deltaY;
    _inputManager->GetMouseDelta(deltaX, deltaY);

    _yaw += static_cast<float>(deltaX) * _mouseSensitivity;
    _pitch += static_cast<float>(deltaY) * _mouseSensitivity;

    if (_pitch > 89.0f) _pitch = 89.0f;
    if (_pitch < -89.0f) _pitch = -89.0f;
}

void PlayerController::HandleMovement(float deltaTime) {
    PlayerEntity* player = GetPlayer();
    if (!player) return;

    bool isMoving = _inputManager->IsKeyDown(KeyCode::W) ||
                    _inputManager->IsKeyDown(KeyCode::S) ||
                    _inputManager->IsKeyDown(KeyCode::D) ||
                    _inputManager->IsKeyDown(KeyCode::A);

    Animator* animator = player->GetAnimator();
    if (animator) {
        MovementState state = player->GetMovementState();
        const std::string& current = animator->GetCurrentClipName();

        if (state == MovementState::Jumping || state == MovementState::Falling) {
            // Airborne — keep current anim
        } else if (isMoving && current != "Running") {
            animator->Play("Running", true);
        } else if (!isMoving && current != "Idle") {
            animator->Play("Idle", true);
        }
    }
}

void PlayerController::HandleDebugMovement(float deltaTime) {
    if (!_camera) return;

    _camera->SetRotation(_yaw * DEG_TO_RAD, _pitch * DEG_TO_RAD);

    float speed = _debugCamSpeed * deltaTime;

    if (_inputManager->IsKeyDown(KeyCode::W)) _camera->MoveForward(speed);
    if (_inputManager->IsKeyDown(KeyCode::S)) _camera->MoveForward(-speed);
    if (_inputManager->IsKeyDown(KeyCode::D)) _camera->MoveRight(speed);
    if (_inputManager->IsKeyDown(KeyCode::A)) _camera->MoveRight(-speed);
    if (_inputManager->IsKeyDown(KeyCode::Space)) _camera->MoveUp(speed);
    if (_inputManager->IsKeyDown(KeyCode::LeftShift)) _camera->MoveUp(-speed);
}