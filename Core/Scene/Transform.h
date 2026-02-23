//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_TRANSFORM_H
#define DIRECTX11_TRANSFORM_H

#pragma once
#include "../Math/Vector3.h"
#include "../Math/Quaternion.h"
#include "../Math/Matrix4x4.h"

#include <EngineApi.h>

class ENGINE_API Transform {
private:
    Math::Vector3 position;
    Math::Quaternion rotation;
    Math::Vector3 scale;

    Transform* parent;

    mutable Math::Matrix4x4 localMatrix;
    mutable Math::Matrix4x4 worldMatrix;
    mutable bool isDirty;

    void MarkDirty();
    void UpdateMatrices() const;

public:
    Transform();

    // Position
    void SetPosition(const Math::Vector3& pos);
    void SetLocalPosition(const Math::Vector3& pos);
    const Math::Vector3& GetPosition() const;
    Math::Vector3 GetWorldPosition() const;

    // Rotation
    void SetRotation(const Math::Quaternion& rot);
    void SetRotation(const Math::Vector3& eulerAngles); // Pitch, Yaw, Roll
    const Math::Quaternion& GetRotation() const;
    Math::Quaternion GetWorldRotation() const;

    // Scale
    void SetScale(const Math::Vector3& scl);
    void SetScale(float uniformScale);
    const Math::Vector3& GetScale() const;

    // Direction vectors
    Math::Vector3 GetForward() const;
    Math::Vector3 GetRight() const;
    Math::Vector3 GetUp() const;

    // Movement
    void Translate(const Math::Vector3& translation);
    void Rotate(const Math::Quaternion& rotation);
    void Rotate(const Math::Vector3& axis, float angle);
    void RotateAround(const Math::Vector3& point, const Math::Vector3& axis, float angle);

    // Look at
    void LookAt(const Math::Vector3& target, const Math::Vector3& up = Math::Vector3::Up);

    // Hierarchy
    void SetParent(Transform* parent);
    Transform* GetParent() const;

    // Matrices
    const Math::Matrix4x4& GetLocalMatrix() const;
    const Math::Matrix4x4& GetWorldMatrix() const;
};


#endif //DIRECTX11_TRANSFORM_H