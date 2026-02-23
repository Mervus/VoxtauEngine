//
// Created by Marvin on 14/02/2026.
// Plays animation clips on a skeleton, producing the final bone matrix palette each frame for GPU skinning.
// Usage:
//   animator.SetSkeleton(skeleton);
//   animator.AddClip("Idle", idleClip);
//   animator.Play("Idle", true);
//   // each frame:
//   animator.Update(deltaTime);
//   auto& matrices = animator.GetBoneMatrices(); // upload to constant buffer
//

#ifndef VOXTAU_ANIMATOR_H
#define VOXTAU_ANIMATOR_H
#pragma once

#include <EngineApi.h>
#include <Core/Math/Matrix4x4.h>
#include "Skeleton.h"
#include "AnimationClip.h"
#include "Renderer/Vertex.h" // for MAX_BONES
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

class ENGINE_API Animator {
public:
    Animator();
    ~Animator() = default;

    // Set the skeleton this animator drives
    void SetSkeleton(std::shared_ptr<Skeleton> skeleton);

    // Add an animation clip (takes shared ownership)
    void AddClip(const std::string& name, std::shared_ptr<AnimationClip> clip);

    // Playback control
    void Play(const std::string& clipName, bool loop = true);
    void Stop();
    void SetSpeed(float speed) { playbackSpeed = speed; }

    // Call every frame
    void Update(float deltaTime);

    // Get the final bone matrices for GPU upload
    // These are: inverseBindPose * currentWorldPose for each bone
    // Already in the correct format for the vertex shader constant buffer
    const Math::Matrix4x4* GetBoneMatrices() const { return finalBoneMatrices.data(); }
    int GetBoneMatrixCount() const { return static_cast<int>(finalBoneMatrices.size()); }

    // State queries
    bool IsPlaying() const { return isPlaying; }
    const std::string& GetCurrentClipName() const { return currentClipName; }
    float GetCurrentTime() const { return currentTime; }
    float GetNormalizedTime() const; // 0..1

private:
    std::shared_ptr<Skeleton> skeleton;
    std::unordered_map<std::string, std::shared_ptr<AnimationClip>> clips;

    // Current playback state
    std::string currentClipName;
    AnimationClip* currentClip = nullptr;
    float currentTime = 0.0f;
    float playbackSpeed = 1.0f;
    bool isPlaying = false;
    bool looping = true;

    // Output bone matrices
    std::vector<Math::Matrix4x4> finalBoneMatrices;

    // Recursive: compute world-space transform for a bone at current animation time
    void ComputeBoneTransform(int boneIndex,
                               const Math::Matrix4x4& parentTransform,
                               float animTime);
};

#endif //VOXTAU_ANIMATOR_H