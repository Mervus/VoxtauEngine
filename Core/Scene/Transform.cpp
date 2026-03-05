//
// Created by Marvin on 28/01/2026.
//

#include "Transform.h"

Transform::Transform()
    : position(0, 0, 0)
    , rotation(Math::Quaternion::Identity)
    , scale(1, 1, 1)
    , parent(nullptr)
    , isDirty(true)
{
}

void Transform::SetPosition(const Math::Vector3& pos) {
    position = pos;
    MarkDirty();
}

void Transform::SetLocalPosition(const Math::Vector3& pos) {
    position = pos;
    MarkDirty();
}

const Math::Vector3& Transform::GetPosition() const {
    return position;
}

Math::Vector3 Transform::GetWorldPosition() const {
    if (parent) {
        return (parent->GetWorldMatrix() * Math::Vector4(position, 1.0f)).ToVector3();
    }
    return position;
}

void Transform::SetRotation(const Math::Quaternion& rot) {
    rotation = rot;
    MarkDirty();
}

void Transform::SetRotation(const Math::Vector3& eulerAngles) {
    rotation = Math::Quaternion::FromEulerAngles(eulerAngles);
    MarkDirty();
}

const Math::Quaternion& Transform::GetRotation() const {
    return rotation;
}

Math::Quaternion Transform::GetWorldRotation() const {
    if (parent) {
        return parent->GetWorldRotation() * rotation;
    }
    return rotation;
}

void Transform::SetScale(const Math::Vector3& scl) {
    scale = scl;
    MarkDirty();
}

void Transform::SetScale(float uniformScale) {
    scale = Math::Vector3(uniformScale, uniformScale, uniformScale);
    MarkDirty();
}

const Math::Vector3& Transform::GetScale() const {
    return scale;
}

Math::Vector3 Transform::GetForward() const {
    return rotation.RotateVector(Math::Vector3::Forward);
}

Math::Vector3 Transform::GetRight() const {
    return rotation.RotateVector(Math::Vector3::Right);
}

Math::Vector3 Transform::GetUp() const {
    return rotation.RotateVector(Math::Vector3::Up);
}

void Transform::Translate(const Math::Vector3& translation) {
    position += translation;
    MarkDirty();
}

void Transform::Rotate(const Math::Quaternion& rot) {
    rotation = rotation * rot;
    MarkDirty();
}

void Transform::Rotate(const Math::Vector3& axis, float angle) {
    Rotate(Math::Quaternion::FromAxisAngle(axis, angle));
}

void Transform::RotateAround(const Math::Vector3& point, const Math::Vector3& axis, float angle) {
    Math::Quaternion rot = Math::Quaternion::FromAxisAngle(axis, angle);
    Math::Vector3 diff = position - point;
    diff = rot.RotateVector(diff);
    position = point + diff;
    rotation = rot * rotation;
    MarkDirty();
}

void Transform::LookAt(const Math::Vector3& target, const Math::Vector3& up) {
    Math::Vector3 forward = (target - position).Normalized();
    rotation = Math::Quaternion::LookRotation(forward, up);
    MarkDirty();
}

void Transform::SetParent(Transform* newParent) {
    parent = newParent;
    MarkDirty();
}

Transform* Transform::GetParent() const {
    return parent;
}

const Math::Matrix4x4& Transform::GetLocalMatrix() const {
    if (isDirty) {
        UpdateMatrices();
    }
    return localMatrix;
}

const Math::Matrix4x4& Transform::GetWorldMatrix() const {
    if (isDirty) {
        UpdateMatrices();
    }
    return worldMatrix;
}

void Transform::MarkDirty() {
    isDirty = true;
}

void Transform::UpdateMatrices() const {
    Math::Matrix4x4 S = Math::Matrix4x4::Scale(scale);
    Math::Matrix4x4 R = rotation.ToMatrix();
    Math::Matrix4x4 T = Math::Matrix4x4::Translation(position);

    // Row-vector convention: Scale → Rotate → Translate
    localMatrix = S * R * T;

    if (parent) {
        worldMatrix = parent->GetWorldMatrix() * localMatrix;
    } else {
        worldMatrix = localMatrix;
    }

    isDirty = false;
}