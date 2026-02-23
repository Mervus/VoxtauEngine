//
// Created by Marvin on 14/02/2026.
//

#ifndef VOXTAU_ANIMATIONCLIP_H
#define VOXTAU_ANIMATIONCLIP_H
#pragma once

#include <EngineApi.h>
#include <Core/Math/Vector3.h>
#include <Core/Math/Quaternion.h>
#include <string>
#include <vector>
#include <unordered_map>

// A single keyframe value at a specific time
template<typename T>
struct Keyframe {
    float time;
    T     value;
};

using PositionKey = Keyframe<Math::Vector3>;
using RotationKey = Keyframe<Math::Quaternion>;
using ScaleKey    = Keyframe<Math::Vector3>;

// All keyframe channels for one bone within one animation
struct BoneAnimation {
    std::string boneName;

    std::vector<PositionKey> positionKeys;
    std::vector<RotationKey> rotationKeys;
    std::vector<ScaleKey>    scaleKeys;

    // Sample position at time t (with linear interpolation)
    Math::Vector3 SamplePosition(float t) const;

    // Sample rotation at time t (with slerp)
    Math::Quaternion SampleRotation(float t) const;

    // Sample scale at time t (with linear interpolation)
    Math::Vector3 SampleScale(float t) const;

private:
    // Find the two keyframes surrounding time t
    template<typename T>
    static int FindKeyIndex(const std::vector<Keyframe<T>>& keys, float t);
};

class ENGINE_API AnimationClip {
public:
    AnimationClip() : duration(0.0f), ticksPerSecond(25.0f) {}
    ~AnimationClip() = default;

    std::string name;
    float duration;         // Total duration in ticks
    float ticksPerSecond;   // Playback speed (Mixamo default is 24 or 30)

    // Bone channels — indexed by bone name for easy lookup
    std::vector<BoneAnimation> channels;
    std::unordered_map<std::string, int> boneNameToChannel;

    // Add a bone channel
    void AddChannel(const BoneAnimation& channel);

    // Get channel for a specific bone (nullptr if not found)
    const BoneAnimation* GetChannel(const std::string& boneName) const;

    // Duration in seconds
    float GetDurationSeconds() const;
};

#endif //VOXTAU_ANIMATIONCLIP_H