//
// Created by Marvin on 28/01/2026.
//

#include "Camera.h"

#include "Core/Math/MathTypes.h"

Camera::Camera()
    : position(0, 0, 0)
    , yaw(0.0f)
    , pitch(0.0f)
    , fov(Math::PI / 4.0f)
    , aspectRatio(16.0f / 9.0f)
    , nearPlane(0.1f)
    , farPlane(1000.0f)
    , needsUpdate(true)
{
    forward = Math::Vector3::Forward;
    right = Math::Vector3::Right;
    up = Math::Vector3::Up;

    UpdateMatrices();
}

void Camera::SetPosition(const Math::Vector3& pos) {
    position = pos;
    needsUpdate = true;
}

void Camera::SetLookAt(const Math::Vector3& target) {
    using namespace Math;

    const Vector3 worldUp = Vector3::Up;

    forward = (target - position).Normalized();

    // Handle forward parallel to up (singularity)
    if (fabs(Vector3::Dot(forward, worldUp)) > 0.999f)
    {
        Vector3 fallbackUp = Vector3::Forward;
        right = Vector3::Cross(worldUp, forward).Normalized();
    }
    else
    {
        right = Vector3::Cross(worldUp, forward).Normalized();
    }

    up = Vector3::Cross(forward, right);

    needsUpdate = true;
}

void Camera::SetPerspective(float fov, float aspect, float near, float far) {
    this->fov = fov;
    this->aspectRatio = aspect;
    this->nearPlane = near;
    this->farPlane = far;
    needsUpdate = true;
}

void Camera::UpdateMatrices() {
    if (!needsUpdate) return;

    viewMatrix = Math::Matrix4x4::LookAt(position, position + forward, up);
    projectionMatrix = Math::Matrix4x4::Perspective(fov, aspectRatio, nearPlane, farPlane);

    viewProjectionMatrix = viewMatrix * projectionMatrix;

    needsUpdate = false;
}

const Math::Vector3& Camera::GetPosition() const {
    return position;
}

const Math::Vector3& Camera::GetForward() const {
    return forward;
}

const Math::Vector3& Camera::GetRight() const {
    return right;
}

const Math::Vector3& Camera::GetUp() const {
    return up;
}

void Camera::Move(const Math::Vector3& offset) {
    position = position + offset;
    needsUpdate = true;
}

void Camera::MoveForward(float distance) {
    position = position + forward * distance;
    needsUpdate = true;
}

void Camera::MoveRight(float distance) {
    position = position + right * distance;
    needsUpdate = true;
}

void Camera::MoveUp(float distance) {
    position = position + Math::Vector3::Up * distance;
    needsUpdate = true;
}

void Camera::Rotate(float yawDelta, float pitchDelta) {
    yaw += yawDelta;
    pitch += pitchDelta;

    // Clamp pitch to avoid flipping
    const float maxPitch = Math::HALF_PI - 0.01f;
    if (pitch > maxPitch) pitch = maxPitch;
    if (pitch < -maxPitch) pitch = -maxPitch;

    UpdateVectors();
}

void Camera::SetRotation(float newYaw, float newPitch) {
    yaw = newYaw;
    pitch = newPitch;

    const float maxPitch = Math::HALF_PI - 0.01f;
    if (pitch > maxPitch) pitch = maxPitch;
    if (pitch < -maxPitch) pitch = -maxPitch;

    UpdateVectors();
}

void Camera::UpdateVectors() {
    // Calculate forward vector from yaw and pitch
    forward.x = cosf(pitch) * sinf(yaw);
    forward.y = sinf(pitch);
    forward.z = cosf(pitch) * cosf(yaw);
    forward = forward.Normalized();

    // Recalculate right and up
    right = Math::Vector3::Cross(Math::Vector3::Up, forward).Normalized();
    up = Math::Vector3::Cross(forward, right);

    needsUpdate = true;
}

const Math::Matrix4x4& Camera::GetViewMatrix() {
    UpdateMatrices();
    return viewMatrix;
}

const Math::Matrix4x4& Camera::GetProjectionMatrix() {
    UpdateMatrices();
    return projectionMatrix;
}

const Math::Matrix4x4& Camera::GetViewProjectionMatrix() {
    UpdateMatrices();
    return viewProjectionMatrix;
}