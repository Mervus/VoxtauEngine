//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_RESOURCEMANAGER_H
#define DIRECTX11_RESOURCEMANAGER_H

#include <EngineApi.h>
#include <memory>
#include <vector>

#include "BlockRegistry.h"

class ShaderCollection;
class Mesh;
class Texture;
class IRendererApi;
class Skeleton;
class AnimationClip;

struct ModelData
{
    Mesh* mesh = nullptr;
    Texture* texture = nullptr;
    bool isSkinned = false;
    std::shared_ptr<Skeleton> skeleton;
    std::vector<std::shared_ptr<AnimationClip>> animations;
};

class ResourceManager
{
public:
    ResourceManager(IRendererApi* renderer, ShaderCollection* shaderCollection);

    ModelData LoadModel(const std::string& filepathModel);
    ModelData LoadModel(const std::string& filepathModel, const std::string& filepathTexture);
    void LoadBlockRegistry(const std::string& filepath);

private:
    IRendererApi* _renderer = nullptr;
    ShaderCollection* _shaderCollection = nullptr;
    BlockRegistry* _blockRegistry = nullptr;

};


#endif //DIRECTX11_RESOURCEMANAGER_H
