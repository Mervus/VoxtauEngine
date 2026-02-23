//
// Created by Marvin on 14/02/2026.
// Uses Assimp
//

#ifndef VOXTAU_MODELLOADER_H
#define VOXTAU_MODELLOADER_H
#pragma once

#include <EngineApi.h>
#include <string>
#include <vector>
#include <memory>

class SkinnedMesh;
class Skeleton;
class AnimationClip;
class Mesh;

#include "Resources/TextureData.h"

// Result from loading a full skinned model
struct SkinnedModelData {
    SkinnedMesh*                               mesh = nullptr;
    std::shared_ptr<Skeleton>                  skeleton;
    std::vector<std::shared_ptr<AnimationClip>> animations; // embedded clips
};

// Result from unified LoadModel (auto-detects skinned vs static)
struct ENGINE_API ModelResult {
    Mesh* mesh = nullptr;             // Mesh or SkinnedMesh (polymorphic)
    bool isSkinned = false;
    std::shared_ptr<Skeleton> skeleton;
    std::vector<std::shared_ptr<AnimationClip>> animations;
    std::unique_ptr<TextureData> diffuseTexture;  // null if no texture found
};

class ENGINE_API ModelLoader
{
public:
    // Load a full skinned model (mesh + skeleton + any embedded animations)
    // This is what you call for the primary FBX (the one with skin/mesh data)
    static SkinnedModelData LoadSkinnedModel(const std::string& filepath);

    // Load only animation clips from an FBX file
    // Maps animation bone names to the provided skeleton
    // Use this for additional Mixamo animation FBX files (animation-only)
    static std::vector<std::shared_ptr<AnimationClip>>
        LoadAnimations(const std::string& filepath,
                       const std::shared_ptr<Skeleton>& skeleton);

    // Load a static (non-skinned) mesh via Assimp
    // Can replace/augment your existing OBJ loader
    static Mesh* LoadStaticMesh(const std::string& filepath);

    // Unified loader: auto-detects skinned vs static, extracts embedded textures
    static ModelResult LoadModel(const std::string& filepath);
};


#endif //VOXTAU_MODELLOADER_H
