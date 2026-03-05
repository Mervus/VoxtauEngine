//
// Created by Marvin on 14/02/2026.
//

#ifndef VOXTAU_SKELETON_H
#define VOXTAU_SKELETON_H
#pragma once

#include <EngineApi.h>
#include <Core/Math/Matrix4x4.h>
#include <Core/Math/Quaternion.h>
#include <Core/Math/Vector3.h>
#include <string>
#include <vector>
#include <unordered_map>

// A single bone in the skeleton hierarchy
struct Bone {
    std::string name;
    int         parentIndex;          // -1 if root

    // Local-space bind pose (rest pose)
    Math::Vector3    localPosition;
    Math::Quaternion localRotation;
    Math::Vector3    localScale;

    // Precomputed inverse bind-pose matrix (world space)
    // Used in skinning: finalMatrix = inverseBindPose * currentWorldPose
    Math::Matrix4x4  inverseBindPose;

    // True if this bone actually deforms vertices (has weights in the mesh).
    // False for ancestor nodes (e.g. Armature/RootNode) that exist only to
    // maintain the hierarchy.  Animation channels on non-deforming nodes may
    // carry a different scale than the bind pose (common Mixamo FBX issue),
    // so the Animator ignores their scale channel.
    bool isDeforming = false;
};

class Skeleton {
public:
    Skeleton() = default;
    ~Skeleton() = default;

    // Add a bone to the skeleton. Returns the bone index.
    int AddBone(const std::string& name, int parentIndex,
                const Math::Vector3& localPos,
                const Math::Quaternion& localRot,
                const Math::Vector3& localScale,
                const Math::Matrix4x4& inverseBindPose,
                bool isDeforming = false);

    // Lookup
    int GetBoneIndex(const std::string& name) const;
    const Bone* GetBone(int index) const;
    const Bone* GetBone(const std::string& name) const;

    int GetBoneCount() const { return static_cast<int>(bones.size()); }
    const std::vector<Bone>& GetBones() const { return bones; }

    // Compute the bind-pose world matrix for a bone (walks up the hierarchy)
    Math::Matrix4x4 ComputeBindPoseWorld(int boneIndex) const;

    // Apply a uniform scale to root bone(s) — used to fix FBX unit mismatches
    void ApplyRootScale(float factor);

private:
    std::vector<Bone> bones;
    std::unordered_map<std::string, int> boneNameToIndex;
};

#endif //VOXTAU_SKELETON_H