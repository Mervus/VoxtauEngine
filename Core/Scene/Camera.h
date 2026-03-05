//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_CAMERA_H
#define DIRECTX11_CAMERA_H

#pragma once
#include "Core/Math/Matrix4x4.h"
#include "Core/Math/Vector3.h"
#include <EngineApi.h>

class Camera {
private:
    Math::Vector3 position;
    Math::Vector3 forward;
    Math::Vector3 up;
    Math::Vector3 right;

    float yaw;         // Rotation around Y axis (radians)
    float pitch;       // Rotation around X axis (radians)

    float fov;         // Field of view (radians)
    float aspectRatio;
    float nearPlane;
    float farPlane;

    Math::Matrix4x4 viewMatrix;
    Math::Matrix4x4 projectionMatrix;
    Math::Matrix4x4 viewProjectionMatrix;

    bool needsUpdate;

    void UpdateMatrices();
    void UpdateVectors();

public:
    Camera();

    void SetPosition(const Math::Vector3& pos);
    void SetLookAt(const Math::Vector3& target);
    void SetPerspective(float fov, float aspect, float near, float far);
    void SetAspectRatio(float aspect);

    // Movement
    void Move(const Math::Vector3& offset);
    void MoveForward(float distance);
    void MoveRight(float distance);
    void MoveUp(float distance);

    // Rotation
    void Rotate(float yawDelta, float pitchDelta);
    void SetRotation(float yaw, float pitch);

    // Getters
    const Math::Vector3& GetPosition() const;
    const Math::Vector3& GetForward() const;
    const Math::Vector3& GetRight() const;
    const Math::Vector3& GetUp() const;
    float GetYaw() const { return yaw; }
    float GetPitch() const { return pitch; }

    const Math::Matrix4x4& GetViewMatrix();
    const Math::Matrix4x4& GetProjectionMatrix();
    const Math::Matrix4x4& GetViewProjectionMatrix();
};


#endif //DIRECTX11_CAMERA_H