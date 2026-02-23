//
// Created by Marvin on 12/02/2026.
//

#include "PostProcessPipeline.h"
#include "Renderer/RenderApi/IRendererApi.h"
#include "Renderer/RenderTarget.h"
#include "Renderer/Shaders/ShaderCollection.h"
#include "Renderer/Shaders/ShaderProgram.h"
#include "Resources/MeshGenerator.h"
#include "Resources/Texture.h"
#include "Core/Profiler/Profiler.h"
#include <iostream>
#include <algorithm>
#include <cmath>

PostProcessPipeline::PostProcessPipeline(IRendererApi* renderer, ShaderCollection* shaderCollection)
    : _renderer(renderer)
    , _shaderCollection(shaderCollection)
{
}

PostProcessPipeline::~PostProcessPipeline()
{
    Shutdown();
}

void PostProcessPipeline::Initialize(int width, int height)
{
    _width = width;
    _height = height;

    // Create fullscreen quad mesh
    _fullscreenQuad = MeshGenerator::CreateFullscreenQuad();
    _renderer->CreateMesh(_fullscreenQuad);

    // Create shared post-process constant buffer
    _postProcessCB = _renderer->CreateConstantBuffer(sizeof(PostProcessConstants));

    // Create render targets
    CreateRenderTargets();

    // Load shaders
    static std::string shaderPath = "Assets/Shaders/";

    _blitShader = _shaderCollection->LoadShader("pp_blit",
        shaderPath + "PostProcess/fullscreen.vert.hlsl",
        shaderPath + "PostProcess/blit.pixel.hlsl");

    _compositeShader = _shaderCollection->LoadShader("pp_composite",
        shaderPath + "PostProcess/fullscreen.vert.hlsl",
        shaderPath + "PostProcess/composite.pixel.hlsl");

    if (_blitShader) {
        _renderer->CreateMeshInputLayout(_fullscreenQuad, _blitShader->GetVertexShader());
    }

    std::cout << "PostProcessPipeline initialized (" << width << "x" << height << ")" << std::endl;
}

void PostProcessPipeline::Shutdown()
{
    DestroyRenderTargets();

    for (auto& srt : _scaledRTs) {
        delete srt.rt;
    }
    _scaledRTs.clear();

    if (_fullscreenQuad) {
        delete _fullscreenQuad;
        _fullscreenQuad = nullptr;
    }

    if (_postProcessCB) {
        _renderer->DestroyConstantBuffer(_postProcessCB);
        _postProcessCB = nullptr;
    }

    for (auto& effect : _effects) {
        if (effect.constantBuffer) {
            _renderer->DestroyConstantBuffer(effect.constantBuffer);
            effect.constantBuffer = nullptr;
        }
    }
    _effects.clear();
}

void PostProcessPipeline::CreateRenderTargets()
{
    _sceneRT = _renderer->CreateRenderTargetWithReadableDepth(_width, _height);
    if (!_sceneRT) {
        std::cerr << "ERROR: Failed to create scene render target!" << std::endl;
        return;
    }

    _pingRT = _renderer->CreateRenderTarget(_width, _height, false);
    _pongRT = _renderer->CreateRenderTarget(_width, _height, false);
}

void PostProcessPipeline::DestroyRenderTargets()
{
    delete _sceneRT;  _sceneRT = nullptr;
    delete _pingRT;   _pingRT = nullptr;
    delete _pongRT;   _pongRT = nullptr;
}

void PostProcessPipeline::Resize(int width, int height)
{
    if (width == _width && height == _height) return;
    _width = width;
    _height = height;

    DestroyRenderTargets();
    CreateRenderTargets();

    for (auto& srt : _scaledRTs) {
        delete srt.rt;
    }
    _scaledRTs.clear();

    std::cout << "PostProcessPipeline resized to " << width << "x" << height << std::endl;
}

RenderTarget* PostProcessPipeline::GetOrCreateScaledRT(int width, int height)
{
    for (auto& srt : _scaledRTs) {
        if (srt.width == width && srt.height == height) {
            return srt.rt;
        }
    }

    RenderTarget* rt = _renderer->CreateRenderTarget(width, height, false);
    if (rt) {
        _scaledRTs.push_back({width, height, rt});
        std::cout << "Created scaled RT: " << width << "x" << height << std::endl;
    }
    return rt;
}

// Scene Capture
void PostProcessPipeline::BeginSceneCapture()
{
    if (!_sceneRT) return;
    _renderer->SetRenderTarget(_sceneRT);
    _renderer->Clear(0.0f, 0.0f, 0.0f, 1.0f);
    _renderer->ClearDepth();
}

void PostProcessPipeline::EndSceneCapture(
    const Math::Matrix4x4& viewProjection,
    const Math::Matrix4x4& inverseViewProjection,
    const Math::Vector3& cameraPosition,
    float time)
{
    PROFILE_SCOPE("PostProcess");

    if (!_sceneRT) return;

    // Update shared constants
    PostProcessConstants constants = {};
    constants.screenWidth = static_cast<float>(_width);
    constants.screenHeight = static_cast<float>(_height);
    constants.time = time;
    constants.cameraPosition[0] = cameraPosition.x;
    constants.cameraPosition[1] = cameraPosition.y;
    constants.cameraPosition[2] = cameraPosition.z;

    Math::Matrix4x4 invVPT = inverseViewProjection.Transposed();
    memcpy(constants.inverseViewProjection, &invVPT, sizeof(float) * 16);

    _renderer->UpdateConstantBuffer(_postProcessCB, &constants, sizeof(constants));

    // Count enabled effects
    int enabledCount = 0;
    for (const auto& effect : _effects) {
        if (effect.enabled && effect.shader) enabledCount++;
    }

    if (enabledCount == 0) {
        _renderer->SetDefaultRenderTarget();
        BlitToBackbuffer(_sceneRT->GetColorTexture());
        return;
    }

    // Run effect chain
    // Scaled effects: render at low res → composite over full-res scene
    // Full-res effects: render directly, replacing the buffer

    Texture* currentFullRes = _sceneRT->GetColorTexture();
    bool usePing = true;
    int remaining = enabledCount;

    for (auto& effect : _effects) {
        if (!effect.enabled || !effect.shader) continue;
        remaining--;
        bool isLast = (remaining == 0);

        float scale = effect.resolutionScale;
        bool isScaled = (scale < 0.99f);

        if (isScaled) {
            // ── SCALED EFFECT ──
            int scaledW = std::max(1, static_cast<int>(_width * scale));
            int scaledH = std::max(1, static_cast<int>(_height * scale));

            RenderTarget* scaledRT = GetOrCreateScaledRT(scaledW, scaledH);
            if (!scaledRT) continue;

            // 1. Run effect at reduced resolution
            _renderer->SetRenderTarget(scaledRT);
            _renderer->Clear(0.0f, 0.0f, 0.0f, 0.0f);
            _renderer->SetViewport(0, 0, scaledW, scaledH);

            RunEffect(effect, currentFullRes, scaledRT);

            // 2. Composite: blend upscaled effect over full-res scene
            _renderer->SetViewport(0, 0, _width, _height);

            if (isLast) {
                _renderer->SetDefaultRenderTarget();
            } else {
                RenderTarget* compositeTarget = usePing ? _pingRT : _pongRT;
                _renderer->SetRenderTarget(compositeTarget);
                _renderer->Clear(0.0f, 0.0f, 0.0f, 1.0f);
            }

            _renderer->SetDepthMode(DepthMode::None);
            _renderer->SetRasterizerMode(RasterizerMode::NoCull);
            _renderer->SetBlendMode(BlendMode::None);

            if (_compositeShader) {
                _renderer->BindShader(_compositeShader);

                // t0 = full-res scene
                _renderer->BindTexture(0, currentFullRes);

                // t2 = scaled effect (linear upscale)
                _renderer->SetTextureFilter(TextureFilter::Linear);
                _renderer->BindTexture(2, scaledRT->GetColorTexture());

                _renderer->Draw(_fullscreenQuad);

                _renderer->SetTextureFilter(TextureFilter::Point);
                _renderer->UnbindTexture(2);
            }

            if (!isLast) {
                RenderTarget* compositeTarget = usePing ? _pingRT : _pongRT;
                currentFullRes = compositeTarget->GetColorTexture();
                usePing = !usePing;
            }

        } else {
            // ── FULL-RES EFFECT ──
            if (isLast) {
                _renderer->SetDefaultRenderTarget();
                RunEffect(effect, currentFullRes, nullptr);
            } else {
                RenderTarget* outputRT = usePing ? _pingRT : _pongRT;
                _renderer->SetRenderTarget(outputRT);
                _renderer->Clear(0.0f, 0.0f, 0.0f, 1.0f);
                RunEffect(effect, currentFullRes, outputRT);
                currentFullRes = outputRT->GetColorTexture();
                usePing = !usePing;
            }
        }
    }

    // Restore
    _renderer->SetDepthMode(DepthMode::Less);
    _renderer->SetRasterizerMode(RasterizerMode::CullBack);
    _renderer->SetBlendMode(BlendMode::None);
}

// Run a single effect pass
void PostProcessPipeline::RunEffect(PostProcessEffect& effect,
                                     Texture* inputColor,
                                     RenderTarget* output)
{
    _renderer->SetDepthMode(DepthMode::None);
    _renderer->SetRasterizerMode(RasterizerMode::NoCull);
    _renderer->SetBlendMode(BlendMode::None);

    _renderer->BindShader(effect.shader);

    // t0 = input color
    _renderer->SetTextureFilter(TextureFilter::Linear);
    _renderer->BindTexture(0, inputColor);

    // t1 = scene depth (always from original scene)
    void* depthSRV = _renderer->GetDepthSRV(_sceneRT);
    if (depthSRV) {
        ID3D11DeviceContext* ctx = _renderer->GetContext();
        ID3D11ShaderResourceView* srv = static_cast<ID3D11ShaderResourceView*>(depthSRV);
        ctx->PSSetShaderResources(1, 1, &srv);
    }

    // b1 = shared post-process constants
    _renderer->BindConstantBuffer(1, _postProcessCB, ShaderStage::Pixel);

    // Per-effect constants
    if (effect.constantBuffer && effect.constantData) {
        _renderer->UpdateConstantBuffer(effect.constantBuffer,
                                         effect.constantData,
                                         effect.constantSize);
        _renderer->BindConstantBuffer(effect.constantSlot,
                                       effect.constantBuffer,
                                       ShaderStage::Pixel);
    }

    // Extra textures
    for (const auto& binding : effect.extraTextures) {
        _renderer->BindTexture(binding.slot, binding.texture);
    }

    _renderer->Draw(_fullscreenQuad);

    // Unbind depth SRV
    if (depthSRV) {
        ID3D11DeviceContext* ctx = _renderer->GetContext();
        ID3D11ShaderResourceView* nullSRV = nullptr;
        ctx->PSSetShaderResources(1, 1, &nullSRV);
    }

    _renderer->SetTextureFilter(TextureFilter::Point);
}

// Effect Management
void PostProcessPipeline::AddEffect(const std::string& name,
                                     const std::string& pixelShaderPath,
                                     float resolutionScale)
{
    for (const auto& effect : _effects) {
        if (effect.name == name) {
            std::cerr << "PostProcess effect already exists: " << name << std::endl;
            return;
        }
    }

    static std::string shaderBase = "Assets/Shaders/";
    std::string fullVertPath = shaderBase + "PostProcess/fullscreen.vert.hlsl";

    ShaderProgram* shader = _shaderCollection->LoadShader(
        "pp_" + name, fullVertPath, pixelShaderPath);

    if (!shader) {
        std::cerr << "Failed to load post-process shader: " << pixelShaderPath << std::endl;
        return;
    }

    _renderer->CreateMeshInputLayout(_fullscreenQuad, shader->GetVertexShader());

    PostProcessEffect effect;
    effect.name = name;
    effect.shader = shader;
    effect.enabled = true;
    effect.resolutionScale = std::clamp(resolutionScale, 0.1f, 1.0f);

    _effects.push_back(effect);

    std::cout << "PostProcess effect added: " << name
              << " (scale: " << effect.resolutionScale << ")" << std::endl;
}

void PostProcessPipeline::RemoveEffect(const std::string& name)
{
    _effects.erase(
        std::remove_if(_effects.begin(), _effects.end(),
            [&name](const PostProcessEffect& e) { return e.name == name; }),
        _effects.end()
    );
}

void PostProcessPipeline::SetEffectEnabled(const std::string& name, bool enabled)
{
    for (auto& effect : _effects) {
        if (effect.name == name) {
            effect.enabled = enabled;
            return;
        }
    }
}

void PostProcessPipeline::SetEffectResolution(const std::string& name, float scale)
{
    for (auto& effect : _effects) {
        if (effect.name == name) {
            effect.resolutionScale = std::clamp(scale, 0.1f, 1.0f);
            return;
        }
    }
}

bool PostProcessPipeline::IsEffectEnabled(const std::string& name) const
{
    for (const auto& effect : _effects) {
        if (effect.name == name) return effect.enabled;
    }
    return false;
}

Texture* PostProcessPipeline::GetSceneColorTexture() const
{
    return _sceneRT ? _sceneRT->GetColorTexture() : nullptr;
}

Texture* PostProcessPipeline::GetSceneDepthTexture() const
{
    return _sceneRT ? _sceneRT->GetDepthTexture() : nullptr;
}

// Blit
void PostProcessPipeline::BlitToBackbuffer(Texture* sourceColor)
{
    if (!sourceColor || !_blitShader) {
        _renderer->SetDefaultRenderTarget();
        return;
    }

    _renderer->SetDefaultRenderTarget();

    _renderer->SetDepthMode(DepthMode::None);
    _renderer->SetRasterizerMode(RasterizerMode::NoCull);
    _renderer->SetBlendMode(BlendMode::None);

    _renderer->BindShader(_blitShader);
    _renderer->SetTextureFilter(TextureFilter::Linear);
    _renderer->BindTexture(0, sourceColor);

    _renderer->Draw(_fullscreenQuad);

    _renderer->SetTextureFilter(TextureFilter::Point);
    _renderer->SetDepthMode(DepthMode::Less);
    _renderer->SetRasterizerMode(RasterizerMode::CullBack);
}