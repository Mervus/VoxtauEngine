//
// Created by Marvin on 14/02/2026.
//

#include "AnimationClip.h"
#include <Core/Math/MathUtils.h>

//  BoneAnimation sampling 
template<typename T>
int BoneAnimation::FindKeyIndex(const std::vector<Keyframe<T>>& keys, float t) {
    // Returns the index of the keyframe just BEFORE time t
    // (so we interpolate between keys[result] and keys[result+1])
    for (int i = 0; i < static_cast<int>(keys.size()) - 1; i++) {
        if (t < keys[i + 1].time)
            return i;
    }
    return static_cast<int>(keys.size()) - 2; // clamp to second-to-last
}

Math::Vector3 BoneAnimation::SamplePosition(float t) const {
    if (positionKeys.empty())
        return Math::Vector3(0, 0, 0);
    if (positionKeys.size() == 1)
        return positionKeys[0].value;

    int idx = FindKeyIndex(positionKeys, t);
    if (idx < 0) idx = 0;

    int nextIdx = idx + 1;
    if (nextIdx >= static_cast<int>(positionKeys.size()))
        return positionKeys.back().value;

    float dt = positionKeys[nextIdx].time - positionKeys[idx].time;
    float factor = (dt > 0.0f) ? (t - positionKeys[idx].time) / dt : 0.0f;
    factor = Math::Clamp01(factor);

    const auto& a = positionKeys[idx].value;
    const auto& b = positionKeys[nextIdx].value;
    return Math::Vector3(
        Math::Lerp(a.x, b.x, factor),
        Math::Lerp(a.y, b.y, factor),
        Math::Lerp(a.z, b.z, factor)
    );
}

Math::Quaternion BoneAnimation::SampleRotation(float t) const {
    if (rotationKeys.empty())
        return Math::Quaternion::Identity;
    if (rotationKeys.size() == 1)
        return rotationKeys[0].value;

    int idx = FindKeyIndex(rotationKeys, t);
    if (idx < 0) idx = 0;

    int nextIdx = idx + 1;
    if (nextIdx >= static_cast<int>(rotationKeys.size()))
        return rotationKeys.back().value;

    float dt = rotationKeys[nextIdx].time - rotationKeys[idx].time;
    float factor = (dt > 0.0f) ? (t - rotationKeys[idx].time) / dt : 0.0f;
    factor = Math::Clamp01(factor);

    return Math::Quaternion::Slerp(
        rotationKeys[idx].value,
        rotationKeys[nextIdx].value,
        factor
    );
}

Math::Vector3 BoneAnimation::SampleScale(float t) const {
    if (scaleKeys.empty())
        return Math::Vector3(1, 1, 1);
    if (scaleKeys.size() == 1)
        return scaleKeys[0].value;

    int idx = FindKeyIndex(scaleKeys, t);
    if (idx < 0) idx = 0;

    int nextIdx = idx + 1;
    if (nextIdx >= static_cast<int>(scaleKeys.size()))
        return scaleKeys.back().value;

    float dt = scaleKeys[nextIdx].time - scaleKeys[idx].time;
    float factor = (dt > 0.0f) ? (t - scaleKeys[idx].time) / dt : 0.0f;
    factor = Math::Clamp01(factor);

    const auto& a = scaleKeys[idx].value;
    const auto& b = scaleKeys[nextIdx].value;
    return Math::Vector3(
        Math::Lerp(a.x, b.x, factor),
        Math::Lerp(a.y, b.y, factor),
        Math::Lerp(a.z, b.z, factor)
    );
}

void AnimationClip::AddChannel(const BoneAnimation& channel) {
    int index = static_cast<int>(channels.size());
    boneNameToChannel[channel.boneName] = index;
    channels.push_back(channel);
}

const BoneAnimation* AnimationClip::GetChannel(const std::string& boneName) const {
    auto it = boneNameToChannel.find(boneName);
    if (it != boneNameToChannel.end())
        return &channels[it->second];
    return nullptr;
}

float AnimationClip::GetDurationSeconds() const {
    if (ticksPerSecond > 0.0f)
        return duration / ticksPerSecond;
    return duration / 25.0f; // fallback
}