//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_SHADERCOLLECTION_H
#define DIRECTX11_SHADERCOLLECTION_H
#pragma once
#include <string>
#include <map>
#include "ShaderProgram.h"
#include <EngineApi.h>

class IRendererApi;

class ENGINE_API ShaderCollection
{
private:
    IRendererApi* renderer;

    // PRE-DEFINED COMMON SHADERS
    ShaderProgram* voxelShader;
    ShaderProgram* waterShader;
    ShaderProgram* skyShader;
    ShaderProgram* uiShader;
    ShaderProgram* shadowShader;
    ShaderProgram* _skinnedEntityShader;
    ShaderProgram* _entityShader;

    // Post-processing shaders
    ShaderProgram* ssaoShader;
    ShaderProgram* ssrShader;
    ShaderProgram* bloomShader;
    ShaderProgram* toneMappingShader;
    ShaderProgram* volumetricLightShader;

    // CUSTOM SHADERS (Dynamic Loading)
    std::map<std::string, ShaderProgram*> customShaders{};

    // Standalone compute shaders
    std::map<std::string, Shader*> computeShaders{};

    // Helper: Load shader from file and compile
    Shader* LoadShaderFromFile(const std::string& filepath, ShaderType type);

    public:
    ShaderCollection(IRendererApi* renderer);
    ~ShaderCollection();

    void Initialize();

    ShaderProgram* LoadShader(
            const std::string& name,
            const std::string& vertexPath,
            const std::string& pixelPath,
            const std::string& geometryPath = ""
        );

    // Load standalone compute shader
    Shader* LoadComputeShader(const std::string& name, const std::string& computePath);

    // GET COMMON SHADERS (Direct Access)
    [[nodiscard]] ShaderProgram* GetVoxelShader() const;
    [[nodiscard]] ShaderProgram* GetWaterShader() const;
    [[nodiscard]] ShaderProgram* GetSkyShader() const;
    [[nodiscard]] ShaderProgram* GetUIShader() const;
    [[nodiscard]] ShaderProgram* GetShadowShader() const;

    // Post-processing
    [[nodiscard]] ShaderProgram* GetSSAOShader() const;
    [[nodiscard]] ShaderProgram* GetSSRShader() const;
    [[nodiscard]] ShaderProgram* GetBloomShader() const;
    [[nodiscard]] ShaderProgram* GetToneMappingShader() const;
    [[nodiscard]] ShaderProgram* GetVolumetricLightShader() const;
    [[nodiscard]] ShaderProgram* GetSkinnedEntityShader() const;
    [[nodiscard]] ShaderProgram* GetEntityShader() const;

    // GET CUSTOM SHADER
    ShaderProgram* GetShader(const std::string& name);

    // Get standalone compute shaders by name
    Shader* GetComputeShader(const std::string& name);

    // HOT-RELOAD (Development)
    void ReloadShader(const std::string& name);
    void ReloadAll();

    // Cleanup
    void UnloadAll();
};


#endif //DIRECTX11_SHADERCOLLECTION_H