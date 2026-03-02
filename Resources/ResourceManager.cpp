//
// Created by Marvin on 28/01/2026.
//

#include "ResourceManager.h"

#include "ModelLoader/ModelLoader.h"
#include "Texture.h"
#include "TextureData.h"
#include "Renderer/RenderApi/IRendererApi.h"
#include "Renderer/Shaders/ShaderCollection.h"

#include <iostream>

ResourceManager::ResourceManager(IRendererApi* renderer, ShaderCollection* shaderCollection) : _renderer(renderer), _shaderCollection(shaderCollection)
{
    _blockRegistry = new BlockRegistry();
}

ModelData ResourceManager::LoadModel(const std::string& filepathModel)
{
    assert(_renderer != nullptr && _shaderCollection != nullptr);

    ModelResult mr = ModelLoader::LoadModel(filepathModel);

    if (!mr.mesh) {
        std::cerr << "ResourceManager: Failed to load model " << filepathModel << std::endl;
        return {};
    }

    // Create GPU mesh + input layout with correct shader
    ShaderProgram* shader = mr.isSkinned
        ? _shaderCollection->GetSkinnedEntityShader()
        : _shaderCollection->GetEntityShader();

    _renderer->CreateMesh(mr.mesh);
    _renderer->CreateMeshInputLayout(mr.mesh, shader->GetVertexShader());

    // Create GPU texture if the model had an embedded/referenced texture
    Texture* gpuTexture = nullptr;
    if (mr.diffuseTexture) {
        gpuTexture = new Texture(filepathModel + "_diffuse");
        _renderer->CreateTexture(gpuTexture, *mr.diffuseTexture);
    }

    ModelData data;
    data.mesh = mr.mesh;
    data.texture = gpuTexture;
    data.isSkinned = mr.isSkinned;
    data.skeleton = std::move(mr.skeleton);
    data.animations = std::move(mr.animations);
    return data;
}

ModelData ResourceManager::LoadModel(const std::string& filepathModel, const std::string& filepathTexture)
{
    ModelData data = LoadModel(filepathModel);

    // Override texture with the explicit file
    TextureData texData;
    if (texData.LoadFromFile(filepathTexture)) {
        Texture* tex = new Texture(filepathTexture);
        _renderer->CreateTexture(tex, texData);
        // TODO: delete old texture if one was extracted from model
        data.texture = tex;
    } else {
        std::cerr << "ResourceManager: Failed to load texture " << filepathTexture << std::endl;
    }

    return data;
}

void ResourceManager::LoadBlockRegistry(const std::string& filepath)
{
    assert(_blockRegistry);
    _blockRegistry->LoadFromFile(filepath);
}
