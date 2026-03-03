//
// Created by Marvin on 12/02/2026.
//

#include "RenderPipeline.h"

#include "PostProcess/PostProcessPipeline.h"
#include "Voxel/VoxelRenderer.h"
#include "Voxel/DistantTerrainRenderer.h"
#include "Sky/SkyRenderer.h"
#include "Debug/DebugLineRenderer.h"
#include "RenderApi/IRendererApi.h"
#include "Shaders/ShaderCollection.h"
#include "Shaders/ShaderProgram.h"
#include "Shaders/ShaderTypes.h"
#include "RenderStates.h"

#include "Core/Scene/Camera.h"
#include "Core/Voxel/ChunkManager.h"
#include "Core/Voxel/Chunk.h"
#include "Core/Entity/EntityManager.h"
#include "Core/Entity/Entity.h"
#include "Core/Math/MathTypes.h"
#include "Core/Profiler/Profiler.h"
#include "Resources/TextureArray.h"
#include "Entity/EntityRenderer.h"

#include <iostream>


RenderPipeline::RenderPipeline(IRendererApi* renderer, ShaderCollection* shaderCollection)
    : _renderer(renderer)
    , _shaderCollection(shaderCollection)
{
}

RenderPipeline::~RenderPipeline()
{
    Shutdown();
}

// Lifecycle
void RenderPipeline::Initialize(int width, int height)
{
    _width = width;
    _height = height;

    // Create shared constant buffers
    _perFrameBuffer = _renderer->CreateConstantBuffer(sizeof(PerFrameBuffer));
    _perObjectBuffer = _renderer->CreateConstantBuffer(sizeof(PerObjectBuffer));

    // Create post-process pipeline
    _postProcessPipeline = new PostProcessPipeline(_renderer, _shaderCollection);
    _postProcessPipeline->Initialize(width, height);

    // Create EntityPipeline
    _entityRenderer = new EntityRenderer(_renderer);
    _entityRenderer->Initialize();

    _skyRenderer = new SkyRenderer(_renderer, _shaderCollection);
    _skyRenderer->Initialize();
    _skyRenderer->SetTimeOfDay(0.35f);
    _skyRenderer->SetDaySpeed(0.0f);

    _voxelRenderer = new VoxelRenderer(_renderer, _shaderCollection);
    _voxelRenderer->Initialize(true);

    // Create ParticlePipeline

    static std::string shaderPath = "Assets/Shaders/";
    _shaderCollection->LoadShader("debug_line",
        shaderPath + "Debug/debug_line.vert.hlsl",
        shaderPath + "Debug/debug_line.pixel.hlsl");
    _shaderCollection->LoadShader("entity",
        shaderPath + "Entity/entity.vert.hlsl",
        shaderPath + "Entity/entity.pixel.hlsl");

    std::cout << "[Client] RenderPipeline initialized (" << width << "x" << height << ")" << std::endl;
}

void RenderPipeline::Shutdown()
{
    if (_postProcessPipeline) {
        _postProcessPipeline->Shutdown();
        delete _postProcessPipeline;
        _postProcessPipeline = nullptr;
    }

    if (_perFrameBuffer) {
        _renderer->DestroyConstantBuffer(_perFrameBuffer);
        _perFrameBuffer = nullptr;
    }
    if (_perObjectBuffer) {
        _renderer->DestroyConstantBuffer(_perObjectBuffer);
        _perObjectBuffer = nullptr;
    }

    if (_entityRenderer)
    {
        _entityRenderer->Shutdown();
    }
}

void RenderPipeline::Resize(int width, int height)
{
    _width = width;
    _height = height;

    if (_postProcessPipeline) {
        _postProcessPipeline->Resize(width, height);
    }
}

// Per-frame submission
void RenderPipeline::SetCamera(Camera* camera)
{
    _camera = camera;
}

void RenderPipeline::SetDistantTerrain(DistantTerrainRenderer* distantTerrain)
{
    _distantTerrain = distantTerrain;
}

void RenderPipeline::SetSkyRenderer(SkyRenderer* sky)
{
    _skyRenderer = sky;
}

void RenderPipeline::SetDebugRenderer(DebugLineRenderer* debug)
{
    _debugRenderer = debug;
}

void RenderPipeline::SetDebugCameraMode(bool enabled)
{
    _debugCameraMode = enabled;
}

// Execute — the full render sequence
void RenderPipeline::Execute(float deltaTime, float totalTime)
{
    PROFILE_FUNCTION();

    if (!_camera) {
        std::cerr << "RenderPipeline::Execute — no camera set!" << std::endl;
        return;
    }

    // Process GPU mesh queue
    if (_voxelRenderer) {
        _voxelRenderer->ProcessMeshQueue(8);
    }

    // Configure frustum culling
    if (_voxelRenderer) {
        _voxelRenderer->SetFrustumCullingEnabled(!_debugCameraMode);
    }

    // Begin post-process capture (redirects to offscreen RT)
    if (_postProcessPipeline) {
        _postProcessPipeline->BeginSceneCapture();
    }

    // Update per-frame constants
    UpdatePerFrameBuffer(totalTime);

    // PASS 1: Sky
    RenderSky(totalTime);

    // PASS 3: Voxel world
    RenderVoxels();

    // Clean up voxel texture binding before entities
    _renderer->UnbindTexture(0);

    // PASS 4: Entities
    _entityRenderer->Render(_perObjectBuffer, _debugCameraMode);

    // PASS 5: Debug overlays
    RenderDebug();

    // End post-process (runs effect chain, blits to backbuffer)
    if (_postProcessPipeline) {
        Math::Matrix4x4 vp = _camera->GetViewProjectionMatrix();
        Math::Matrix4x4 invVP = vp.Inverted();
        _postProcessPipeline->EndSceneCapture(vp, invVP, _camera->GetPosition(), totalTime);
    }
}

// Per-frame constant buffer
void RenderPipeline::UpdatePerFrameBuffer(float totalTime)
{
    PerFrameBuffer perFrame;
    perFrame.viewProjection = _camera->GetViewProjectionMatrix().Transposed();
    perFrame.cameraPosition = _camera->GetPosition();

    _renderer->UpdateConstantBuffer(_perFrameBuffer, &perFrame, sizeof(PerFrameBuffer));
    _renderer->BindConstantBuffer(0, _perFrameBuffer, ShaderStage::Vertex | ShaderStage::Pixel);
}

// Render passes
void RenderPipeline::RenderSky(float totalTime)
{
    if (!_skyRenderer || !_camera) return;

    PROFILE_SCOPE("Sky");

    Math::Matrix4x4 vp = _camera->GetViewProjectionMatrix();
    _skyRenderer->Render(vp, _camera->GetPosition(), totalTime);
}

void RenderPipeline::RenderVoxels()
{
    if (!_chunkManager || !_voxelRenderer) return;
    PROFILE_SCOPE("Voxels");

    // Register chunks with the GPU renderer
    for (auto& [pos, chunk] : _chunkManager->GetChunks()) {
        if (chunk) {
            Math::Vector3 worldPos(
                static_cast<float>(pos.x) * Chunk::CHUNK_SIZE,
                static_cast<float>(pos.y) * Chunk::CHUNK_SIZE,
                static_cast<float>(pos.z) * Chunk::CHUNK_SIZE
            );
            _voxelRenderer->RegisterChunk(chunk, worldPos);
        }
    }

    Math::Matrix4x4 vp = _camera->GetViewProjectionMatrix();
    _voxelRenderer->Render(vp, _perFrameBuffer, _perObjectBuffer, _voxelRenderer->GetBlockTextureArray());
}


void RenderPipeline::RenderDebug()
{
    // Debug rendering placeholder — can be wired up later
    // if (_debugRenderer) { ... }
}