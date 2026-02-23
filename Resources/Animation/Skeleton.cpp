//
// Created by Marvin on 14/02/2026.
//

#include "Skeleton.h"
#include <iostream>

int Skeleton::AddBone(const std::string& name, int parentIndex,
                      const Math::Vector3& localPos,
                      const Math::Quaternion& localRot,
                      const Math::Vector3& localScale,
                      const Math::Matrix4x4& inverseBindPose) {
    int index = static_cast<int>(bones.size());

    Bone bone;
    bone.name = name;
    bone.parentIndex = parentIndex;
    bone.localPosition = localPos;
    bone.localRotation = localRot;
    bone.localScale = localScale;
    bone.inverseBindPose = inverseBindPose;

    bones.push_back(bone);
    boneNameToIndex[name] = index;

    return index;
}

int Skeleton::GetBoneIndex(const std::string& name) const {
    auto it = boneNameToIndex.find(name);
    if (it != boneNameToIndex.end())
        return it->second;
    return -1;
}

const Bone* Skeleton::GetBone(int index) const {
    if (index < 0 || index >= static_cast<int>(bones.size()))
        return nullptr;
    return &bones[index];
}

const Bone* Skeleton::GetBone(const std::string& name) const {
    int idx = GetBoneIndex(name);
    if (idx < 0) return nullptr;
    return &bones[idx];
}

Math::Matrix4x4 Skeleton::ComputeBindPoseWorld(int boneIndex) const {
    if (boneIndex < 0 || boneIndex >= static_cast<int>(bones.size()))
        return Math::Matrix4x4::Identity;

    const Bone& bone = bones[boneIndex];

    // Build local transform: S * R * T (row-vector convention, same as Transform)
    Math::Matrix4x4 S = Math::Matrix4x4::Scale(bone.localScale);
    Math::Matrix4x4 R = bone.localRotation.ToMatrix();
    Math::Matrix4x4 T = Math::Matrix4x4::Translation(bone.localPosition);
    Math::Matrix4x4 local = S * R * T;

    if (bone.parentIndex >= 0) {
        return local * ComputeBindPoseWorld(bone.parentIndex);
    }
    return local;
}