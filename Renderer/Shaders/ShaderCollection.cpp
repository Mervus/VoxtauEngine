//
// Created by Marvin on 28/01/2026.
//

#include "ShaderCollection.h"
#include <Renderer/RenderApi/IRendererApi.h>
#include <fstream>
#include <sstream>
#include <iostream>
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
    std::cout << "Initializing ShaderCollection..." << std::endl;

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

    std::cout << "ShaderCollection initialized successfully!" << std::endl;
}

ShaderProgram* ShaderCollection::LoadShader(
    const std::string& name,
    const std::string& vertexPath,
    const std::string& pixelPath,
    const std::string& geometryPath)
{
    std::cout << "Loading shader: " << name << std::endl;

    // Check if already loaded in custom shaders
    auto it = customShaders.find(name);
    if (it != customShaders.end()) {
        std::cout << "  Shader already loaded, returning existing." << std::endl;
        return it->second;
    }

    // Load and compile vertex shader
    Shader* vs = LoadShaderFromFile(vertexPath, ShaderType::Vertex);
    if (!vs) {
        std::cerr << "  ERROR: Failed to load vertex shader: " << vertexPath << std::endl;
        return nullptr;
    }

    // Load and compile pixel shader
    Shader* ps = LoadShaderFromFile(pixelPath, ShaderType::Pixel);
    if (!ps) {
        std::cerr << "  ERROR: Failed to load pixel shader: " << pixelPath << std::endl;
        delete vs;
        return nullptr;
    }

    // Optional geometry shader
    Shader* gs = nullptr;
    if (!geometryPath.empty()) {
        gs = LoadShaderFromFile(geometryPath, ShaderType::Geometry);
        if (!gs) {
            std::cerr << "  WARNING: Failed to load geometry shader: " << geometryPath << std::endl;
        }
    }

    std::cout << "Creating Shader Program " << name << std::endl;
    // Create shader program
    ShaderProgram* program = new ShaderProgram(name);
    program->SetVertexShader(vs);
    program->SetPixelShader(ps);
    if (gs) {
        program->SetGeometryShader(gs);
    }

    // Validate
    if (!program->IsValid()) {
        std::cerr << "  ERROR: Shader program is invalid!" << std::endl;
        delete program;
        return nullptr;
    }

    // Store in custom shaders map
    customShaders[name] = program;

    std::cout << "  Shader loaded successfully: " << name << std::endl;
    return program;
}

Shader* ShaderCollection::LoadComputeShader(
    const std::string& name,
    const std::string& computePath)
{
    std::cout << "Loading compute shader: " << name << std::endl;

    // Check if already loaded
    auto it = computeShaders.find(name);
    if (it != computeShaders.end()) {
        std::cout << "  Compute shader already loaded, returning existing." << std::endl;
        return it->second;
    }

    Shader* cs = LoadShaderFromFile(computePath, ShaderType::Compute);
    if (!cs) {
        std::cerr << "  ERROR: Failed to load compute shader: " << computePath << std::endl;
        return nullptr;
    }

    // Store in map
    computeShaders[name] = cs;

    std::cout << "  Compute shader loaded successfully: " << name << std::endl;
    return cs;
}

Shader* ShaderCollection::GetComputeShader(const std::string& name) {
    auto it = computeShaders.find(name);
    if (it != computeShaders.end()) {
        return it->second;
    }

    std::cerr << "WARNING: Compute shader not found: " << name << std::endl;
    return nullptr;
}

Shader* ShaderCollection::LoadShaderFromFile(
    const std::string& filepath,
    ShaderType type)
{
    // Read shader source from file
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "    ERROR: Cannot open shader file: " << filepath << std::endl;
        return nullptr;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string shaderSource = buffer.str();
    file.close();

    if (shaderSource.empty()) {
        std::cerr << "    ERROR: Shader file is empty: " << filepath << std::endl;
        return nullptr;
    }

    // Create Shader object
    Shader* shader = new Shader(filepath, filepath, type);

    // Compile shader using renderer (platform-specific)
    if (!renderer->CompileShader(shader, shaderSource)) {
        std::cerr << "    ERROR: Failed to compile shader: " << filepath << std::endl;
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

    std::cerr << "WARNING: Shader not found: " << name << std::endl;
    return nullptr;
}

// HOT-RELOAD

void ShaderCollection::ReloadShader(const std::string& name) {
    // Find shader in custom shaders
    auto it = customShaders.find(name);
    if (it == customShaders.end()) {
        std::cerr << "WARNING: Cannot reload shader, not found: " << name << std::endl;
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

    std::cout << "Reloaded shader: " << name << std::endl;
}

void ShaderCollection::ReloadAll() {
    std::cout << "Reloading all shaders..." << std::endl;

    // Store all shader names
    std::vector<std::string> shaderNames;
    for (auto& pair : customShaders) {
        shaderNames.push_back(pair.first);
    }

    // Reload each
    for (const auto& name : shaderNames) {
        ReloadShader(name);
    }

    std::cout << "All shaders reloaded!" << std::endl;
}

// CLEANUP
void ShaderCollection::UnloadAll() {
    std::cout << "Unloading all shaders..." << std::endl;

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

    std::cout << "All shaders unloaded." << std::endl;
}

