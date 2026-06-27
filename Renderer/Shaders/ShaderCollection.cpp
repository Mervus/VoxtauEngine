//
// Created by Marvin on 28/01/2026.
//

#include "ShaderCollection.h"
#include <Renderer/RenderApi/IRendererApi.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <d3dcompiler.h>

static std::string shaderPath = "Assets/Shaders/";

ShaderCollection::ShaderCollection(IRendererApi* renderer)
    : renderer(renderer)
    , voxelShader(nullptr)
    , waterShader(nullptr)
    , skyShader(nullptr)
    , uiShader(nullptr)
    , shadowShader(nullptr)
    , ssaoShader(nullptr)
    , ssrShader(nullptr)
    , bloomShader(nullptr)
    , toneMappingShader(nullptr)
    , volumetricLightShader(nullptr)
{
}

ShaderCollection::~ShaderCollection() {
    UnloadAll();
}

void ShaderCollection::Initialize() {
    _logger.Info("[ShaderCollection] Initializing...");

    // LOAD COMMON SHADERS

    // Voxel Shader (CPU mesh path — IA input with POSITION/TEXCOORD/PACKED)
    //voxelShader = LoadShader("voxel",
    //   shaderPath + "Voxel/voxel.vert.hlsl",
    //   shaderPath + "Voxel/voxel.pixel.hlsl");

    voxelShader = LoadShader("voxel",
        shaderPath + "Voxel/voxel_gpu.vert.hlsl",
        shaderPath + "Voxel/voxel_gpu.pixel.hlsl");

    // Water Shader
    // waterShader = LoadShader("water",
    //     shaderPath + "Water/water.vert.hlsl",
    //     shaderPath + "Water/water.pixel.hlsl");
    //
    // // Sky Shader
    skyShader = LoadShader("sky",
    shaderPath + "Sky/sky.vert.hlsl",
    shaderPath + "Sky/sky.pixel.hlsl");
    //
    // // Tone Mapping (Post-Processing)
    // toneMappingShader = LoadShader("tonemapping",
    //     shaderPath + "PostProcess/fullscreen.vert.hlsl",
    //     shaderPath + "PostProcess/tonemapping.pixel.hlsl");

    _skinnedEntityShader = LoadShader("skinned_entity",
     shaderPath + "Entity/skinned_entity.vert.hlsl",
     shaderPath + "Entity/entity.pixel.hlsl");

    _entityShader = LoadShader("entity",
     shaderPath + "Entity/entity.vert.hlsl",
     shaderPath + "Entity/entity.pixel.hlsl");

    // Load compute shaders.
    LoadComputeShader("ChunkMesh", shaderPath + "Compute/ChunkMesh.hlsl");
    LoadComputeShader("BuildDrawArgs",shaderPath + "Compute/BuildDrawArgs.hlsl");
    LoadComputeShader("FrustumCull", shaderPath + "Compute/Culling/FrustumCull.hlsl");
}

ShaderProgram* ShaderCollection::LoadShader(
    const std::string& name,
    const std::string& vertexPath,
    const std::string& pixelPath,
    const std::string& geometryPath)
{
    // Check if already loaded in custom shaders
    auto it = customShaders.find(name);
    if (it != customShaders.end()) {
        _logger.Error("[ShaderCollection] Shader already loaded, returning existing.");
        return it->second;
    }

    // Load and compile vertex shader
    Shader* vs = LoadShaderFromFile(vertexPath, ShaderType::Vertex);
    if (!vs) {
        _logger.Error("[ShaderCollection] Failed to load vertex shader: %s", vertexPath.c_str());
        return nullptr;
    }

    // Load and compile pixel shader
    Shader* ps = LoadShaderFromFile(pixelPath, ShaderType::Pixel);
    if (!ps) {
        _logger.Error("[ShaderCollection] Failed to load pixel shader: %s", pixelPath.c_str());
        delete vs;
        return nullptr;
    }

    // Optional geometry shader
    Shader* gs = nullptr;
    if (!geometryPath.empty()) {
        gs = LoadShaderFromFile(geometryPath, ShaderType::Geometry);
        if (!gs) {
            _logger.Error("[ShaderCollection] Failed to load geometry shader: %s", geometryPath.c_str());
        }
    }

    // Create shader program
    ShaderProgram* program = new ShaderProgram(name);
    program->SetVertexShader(vs);
    program->SetPixelShader(ps);
    if (gs) {
        program->SetGeometryShader(gs);
    }

    // Validate
    if (!program->IsValid()) {
        _logger.Error("[ShaderCollection] Shader program is invalid!");
        delete program;
        return nullptr;
    }

    // Store in custom shaders map
    customShaders[name] = program;

    return program;
}

Shader* ShaderCollection::LoadComputeShader(
    const std::string& name,
    const std::string& computePath)
{
    // Check if already loaded
    auto it = computeShaders.find(name);
    if (it != computeShaders.end()) {
        _logger.Error("[ShaderCollection] Compute shader already loaded, returning existing.");
        return it->second;
    }

    Shader* cs = LoadShaderFromFile(computePath, ShaderType::Compute);
    if (!cs) {
        _logger.Error("[ShaderCollection] Failed to load compute shader: %s", computePath.c_str());
        return nullptr;
    }

    // Store in map
    computeShaders[name] = cs;

    return cs;
}

Shader* ShaderCollection::GetComputeShader(const std::string& name) {
    auto it = computeShaders.find(name);
    if (it != computeShaders.end()) {
        return it->second;
    }

    _logger.Error("[ShaderCollection] Compute shader not found: %s", name.c_str());
    return nullptr;
}

Shader* ShaderCollection::LoadShaderFromFile(
    const std::string& filepath,
    ShaderType type)
{
    // Read shader source from file
    std::ifstream file(filepath);
    if (!file.is_open()) {
        _logger.Error("[ShaderCollection] Cannot open shader file: %s", filepath.c_str());
        return nullptr;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string shaderSource = buffer.str();
    file.close();

    if (shaderSource.empty()) {
        _logger.Error("[ShaderCollection] Shader file is empty: %s", filepath.c_str());
        return nullptr;
    }

    // Create Shader object
    Shader* shader = new Shader(filepath, filepath, type);

    // Compile shader using renderer (platform-specific)
    if (!renderer->CompileShader(shader, shaderSource)) {
        _logger.Error("[ShaderCollection] Failed to compile shader: %s", filepath.c_str());
        delete shader;
        return nullptr;
    }

    return shader;
}

// GETTERS FOR COMMON SHADERS

ShaderProgram* ShaderCollection::GetVoxelShader() const {
    return voxelShader;
}

ShaderProgram* ShaderCollection::GetWaterShader() const {
    return waterShader;
}

ShaderProgram* ShaderCollection::GetSkyShader() const {
    return skyShader;
}

ShaderProgram* ShaderCollection::GetUIShader() const {
    return uiShader;
}

ShaderProgram* ShaderCollection::GetShadowShader() const {
    return shadowShader;
}

ShaderProgram* ShaderCollection::GetSSAOShader() const {
    return ssaoShader;
}

ShaderProgram* ShaderCollection::GetSSRShader() const {
    return ssrShader;
}

ShaderProgram* ShaderCollection::GetBloomShader() const {
    return bloomShader;
}

ShaderProgram* ShaderCollection::GetToneMappingShader() const {
    return toneMappingShader;
}

ShaderProgram* ShaderCollection::GetVolumetricLightShader() const {
    return volumetricLightShader;
}

ShaderProgram* ShaderCollection::GetSkinnedEntityShader() const
{
    return _skinnedEntityShader;
}

ShaderProgram* ShaderCollection::GetEntityShader() const
{
    return _entityShader;
};
// GET CUSTOM SHADER BY NAME

ShaderProgram* ShaderCollection::GetShader(const std::string& name) {
    auto it = customShaders.find(name);
    if (it != customShaders.end()) {
        return it->second;
    }

    _logger.Error("[ShaderCollection] Shader not found: %s", name.c_str());
    return nullptr;
}

// HOT-RELOAD

void ShaderCollection::ReloadShader(const std::string& name) {
    // Find shader in custom shaders
    auto it = customShaders.find(name);
    if (it == customShaders.end()) {
        _logger.Error("[ShaderCollection] Cannot reload shader, not found: %s", name.c_str());
        return;
    }

    ShaderProgram* program = it->second;

    // Get original paths from shaders
    std::string vertPath = program->GetVertexShader()->GetFilepath();
    std::string pixelPath = program->GetPixelShader()->GetFilepath();
    std::string geomPath = program->GetGeometryShader() ?
        program->GetGeometryShader()->GetFilepath() : "";

    // Delete old program
    customShaders.erase(it);
    delete program;

    // Reload
    LoadShader(name, vertPath, pixelPath, geomPath);

    _logger.Info("[ShaderCollection] Reloaded shader: %s", name.c_str());
}

void ShaderCollection::ReloadAll() {
    _logger.Info("[ShaderCollection] Reloading all shaders...");

    // Store all shader names
    std::vector<std::string> shaderNames;
    for (auto& pair : customShaders) {
        shaderNames.push_back(pair.first);
    }

    // Reload each
    for (const auto& name : shaderNames) {
        ReloadShader(name);
    }

    _logger.Info("[ShaderCollection] All shaders reloaded!");
}

// CLEANUP
void ShaderCollection::UnloadAll() {
    _logger.Info("[ShaderCollection] Unloading all shaders...");

    // Delete common shaders (they're also in customShaders map)
    voxelShader = nullptr;
    waterShader = nullptr;
    skyShader = nullptr;
    uiShader = nullptr;
    shadowShader = nullptr;
    ssaoShader = nullptr;
    ssrShader = nullptr;
    bloomShader = nullptr;
    toneMappingShader = nullptr;
    volumetricLightShader = nullptr;

    // Delete all custom shaders
    for (auto& pair : customShaders) {
        delete pair.second;
    }
    customShaders.clear();

    // Delete all compute shaders
    for (auto& pair : computeShaders) {
        delete pair.second;
    }
    computeShaders.clear();

    _logger.Info("[ShaderCollection] All shaders unloaded.");
}

