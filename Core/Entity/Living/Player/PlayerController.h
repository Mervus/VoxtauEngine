//
// Created by Marvin on 10/02/2026.
//

#ifndef VOXTAU_PLAYERCONTROLLER_H
#define VOXTAU_PLAYERCONTROLLER_H
#pragma once
#include <EngineApi.h>

#include "Core/Entity/EntityID.h"
#include "Core/Math/Vector3.h"

class InputManager;
class EntityManager;
class PlayerEntity;
class Camera;

class PlayerController {
protected:
    EntityManager* _entityManager;
    InputManager* _inputManager;
    Camera* _camera;
    EntityID _playerID;

    // Camera rotation
    float _yaw;
    float _pitch;
    float _mouseSensitivity;

    // Camera mode
    Math::Vector3 _cameraOffset;
    bool _thirdPerson;

    // Debug camera
    bool _debugCamera = false;
    float _debugCamSpeed = 10.0f;

    // Input processing — virtual so game layer can extend/override
    virtual void HandleMovement(float deltaTime);
    virtual void HandleCamera(float deltaTime);
    virtual void HandleDebugMovement(float deltaTime);

public:
    PlayerController(EntityManager* entityManager, InputManager* inputManager, Camera* camera);
    virtual ~PlayerController();

    void SetPlayerID(EntityID id) { _playerID = id; }
    EntityID GetPlayerID() const { return _playerID; }

    void SetMouseSensitivity(float sensitivity) { _mouseSensitivity = sensitivity; }
    void SetThirdPerson(bool enabled, const Math::Vector3& offset = Math::Vector3(0, 5, -10));

    float GetYaw() const { return _yaw; }
    float GetPitch() const { return _pitch; }
    bool IsDebugCamera() const { return _debugCamera; }

    virtual void Update(float deltaTime);
    virtual void LateUpdate(float deltaTime);

    PlayerEntity* GetPlayer() const;
};


#endif //VOXTAU_PLAYERCONTROLLER_H