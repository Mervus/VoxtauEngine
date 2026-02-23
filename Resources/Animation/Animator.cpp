//
// Created by Marvin on 14/02/2026.
//

#include "Animator.h"
#include <iostream>

Animator::Animator() {
    // Pre-allocate bone matrices to identity
    finalBoneMatrices.resize(MAX_BONES, Math::Matrix4x4::Identity);
}

void Animator::SetSkeleton(std::shared_ptr<Skeleton> skel) {
    skeleton = skel;

    // Resize to actual bone count (clamped to MAX_BONES)
    int count = (std::min)(skeleton->GetBoneCount(), MAX_BONES);
    finalBoneMatrices.resize(count, Math::Matrix4x4::Identity);
}

void Animator::AddClip(const std::string& name, std::shared_ptr<AnimationClip> clip) {
    clips[name] = clip;
}

void Animator::Play(const std::string& clipName, bool loop) {
    auto it = clips.find(clipName);
    if (it == clips.end()) {
        std::cerr << "Animator: Clip not found: " << clipName << std::endl;
        return;
    }

    currentClipName = clipName;
    currentClip = it->second.get();
    currentTime = 0.0f;
    looping = loop;
    isPlaying = true;
}

void Animator::Stop() {
    isPlaying = false;
    currentClip = nullptr;
    currentClipName.clear();
    currentTime = 0.0f;

    // Reset to identity (bind pose)
    for (auto& mat : finalBoneMatrices)
        mat = Math::Matrix4x4::Identity;
}

float Animator::GetNormalizedTime() const {
    if (!currentClip || currentClip->duration <= 0.0f)
        return 0.0f;
    return currentTime / currentClip->duration;
}

void Animator::Update(float deltaTime) {
    if (!isPlaying || !currentClip || !skeleton)
        return;

    // Advance time in ticks
    currentTime += deltaTime * currentClip->ticksPerSecond * playbackSpeed;

    if (looping) {
        if (currentClip->duration > 0.0f)
            currentTime = std::fmod(currentTime, currentClip->duration);
    } else {
        if (currentTime >= currentClip->duration) {
            currentTime = currentClip->duration;
            isPlaying = false;
        }
    }

    // Walk the bone hierarchy from all root bones
    // Bones are stored in parent-first order (parent always has lower index),
    // so we can iterate linearly instead of recursing.
    const auto& bones = skeleton->GetBones();
    int boneCount = (std::min)(static_cast<int>(bones.size()), MAX_BONES);

    // Temporary array for world-space transforms (before multiplying by inverse bind pose)
    std::vector<Math::Matrix4x4> worldTransforms(boneCount);

    for (int i = 0; i < boneCount; i++) {
        const Bone& bone = bones[i];

        // Sample animation for this bone (or use bind pose if no channel exists)
        Math::Vector3    pos   = bone.localPosition;
        Math::Quaternion rot   = bone.localRotation;
        Math::Vector3    scale = bone.localScale;

        const BoneAnimation* channel = currentClip->GetChannel(bone.name);
        if (channel) {
            pos   = channel->SamplePosition(currentTime);
            rot   = channel->SampleRotation(currentTime);
            scale = channel->SampleScale(currentTime);
        }

        // Build local transform: S * R * T (row-vector convention)
        Math::Matrix4x4 S = Math::Matrix4x4::Scale(scale);
        Math::Matrix4x4 R = rot.ToMatrix();
        Math::Matrix4x4 T = Math::Matrix4x4::Translation(pos);
        Math::Matrix4x4 localTransform = S * R * T;

        // World transform = local * parent's world
        if (bone.parentIndex >= 0 && bone.parentIndex < boneCount) {
            worldTransforms[i] = localTransform * worldTransforms[bone.parentIndex];
        } else {
            worldTransforms[i] = localTransform;
        }

        // Final matrix = inverseBindPose * worldTransform
        // This transforms a vertex from bind-pose space into current-pose space
        finalBoneMatrices[i] = bone.inverseBindPose * worldTransforms[i];
    }
}