//
// Created by Marvin on 12/02/2026.
//

#ifndef VOXTAU_POSTPROCESSPIPELINE_H
#define VOXTAU_POSTPROCESSPIPELINE_H
#pragma once

#include <vector>
#include <string>
#include <functional>
#include "Core/Math/MathTypes.h"

class IRendererApi;
class ShaderCollection;
class ShaderProgram;
class RenderTarget;
class Mesh;
class Texture;

// A single post-process effect in the chain
struct PostProcessEffect {
    std::string name;
    ShaderProgram* shader = nullptr;
    bool enabled = true;

    // Resolution scale: 1.0 = full res, 0.5 = half res, 0.25 = quarter res
    // Each effect can run at its own resolution. The pipeline creates
    // appropriately sized RTs and upscales automatically.
    float resolutionScale = 1.0f;

    // Optional per-effect constant buffer (effect manages its own data)
    void* constantBuffer = nullptr;
    const void* constantData = nullptr;
    size_t constantSize = 0;
    int constantSlot = 2;  // b2 by default (b0 = per-frame, b1 = post-process)

    // Optional extra texture bindings (e.g., noise texture on t2)
    struct TextureBinding {
        int slot;
        Texture* texture;
    };
    std::vector<TextureBinding> extraTextures;
};

class PostProcessPipeline {
public:
    PostProcessPipeline(IRendererApi* renderer, ShaderCollection* shaderCollection);
    ~PostProcessPipeline();

    void Initialize(int width, int height);
    void Shutdown();
    void Resize(int width, int height);

    // Call BEFORE scene rendering — redirects output to offscreen RT
    void BeginSceneCapture();

    // Call AFTER scene rendering — runs effect chain, outputs to backbuffer
    void EndSceneCapture(const Math::Matrix4x4& viewProjection,
                         const Math::Matrix4x4& inverseViewProjection,
                         const Math::Vector3& cameraPosition,
                         float time);

    // Effect management
    void AddEffect(const std::string& name, const std::string& pixelShaderPath,
                   float resolutionScale = 1.0f);
    void RemoveEffect(const std::string& name);
    void SetEffectEnabled(const std::string& name, bool enabled);
    void SetEffectResolution(const std::string& name, float scale);
    bool IsEffectEnabled(const std::string& name) const;

    // Access the scene render target (for systems that need the depth SRV)
    RenderTarget* GetSceneRT() const { return _sceneRT; }

    // Get scene color/depth textures for custom effects
    Texture* GetSceneColorTexture() const;
    Texture* GetSceneDepthTexture() const;

    int GetWidth() const { return _width; }
    int GetHeight() const { return _height; }

private:
    IRendererApi* _renderer;
    ShaderCollection* _shaderCollection;

    int _width = 0;
    int _height = 0;

    // Offscreen render targets
    RenderTarget* _sceneRT = nullptr;    // Scene renders here (color + readable depth)
    RenderTarget* _pingRT = nullptr;     // Ping-pong for multi-effect chains
    RenderTarget* _pongRT = nullptr;

    // Fullscreen quad
    Mesh* _fullscreenQuad = nullptr;

    // Shared constant buffer for post-process passes
    void* _postProcessCB = nullptr;

    // Effect chain (executed in order)
    std::vector<PostProcessEffect> _effects;

    // Final blit shader (just copies to backbuffer, or does tone mapping)
    ShaderProgram* _blitShader = nullptr;

    // Composite shader (blends low-res effect result over full-res scene)
    ShaderProgram* _compositeShader = nullptr;

    // Scaled render targets cache (keyed by resolution: width<<16|height)
    struct ScaledRT {
        int width;
        int height;
        RenderTarget* rt;
    };
    std::vector<ScaledRT> _scaledRTs;

    void CreateRenderTargets();
    void DestroyRenderTargets();
    void BlitToBackbuffer(Texture* sourceColor);
    void RunEffect(PostProcessEffect& effect, Texture* inputColor, RenderTarget* output);
    RenderTarget* GetOrCreateScaledRT(int width, int height);
};

#endif //VOXTAU_POSTPROCESSPIPELINE_H