//
// Created by Marvin on 12/02/2026.
//

#ifndef VOXTAU_RENDERPIPELINE_H
#define VOXTAU_RENDERPIPELINE_H
#pragma once

#include <vector>
#include <functional>
#include "Core/Math/MathTypes.h"
#include "Core/Math/Matrix4x4.h"

// Forward declarations
class IRendererApi;
class ShaderCollection;
class Camera;
class Mesh;
class Texture;
class ShaderProgram;
class TextureArray;

class ChunkManager;
class VoxelRenderer;
class DistantTerrainRenderer;
class SkyRenderer;
class PostProcessPipeline;
class DebugLineRenderer;
class EntityManager;
class BlockRegistry;
class EntityRenderer;

struct RenderData;

// RenderPipeline
class RenderPipeline {
public:
    RenderPipeline(IRendererApi* renderer, ShaderCollection* shaderCollection);
    ~RenderPipeline();

    // Lifecycle
    void Initialize(int width, int height);
    void Shutdown();
    void Resize(int width, int height);

    // Per-frame submission (call from scene)

    // Set the camera for this frame
    void SetCamera(Camera* camera);

    // Distant terrain
    void SetDistantTerrain(DistantTerrainRenderer* distantTerrain);

    // Sky
    void SetSkyRenderer(SkyRenderer* sky);

    // Debug rendering
    void SetDebugRenderer(DebugLineRenderer* debug);
    void SetDebugCameraMode(bool enabled);

    // Execute
    // Call once per frame — runs the full render sequence
    void Execute(float deltaTime, float totalTime);

    // Sub-system access
    PostProcessPipeline* GetPostProcessPipeline() { return _postProcessPipeline; }
    SkyRenderer* GetSky() { return _skyRenderer; }
    VoxelRenderer* GetVoxelRenderer() { return _voxelRenderer; }
    EntityRenderer* GetEntityRenderer() { return _entityRenderer; }

    // Constant buffer access (for systems that need them)
    void* GetPerFrameBuffer() { return _perFrameBuffer; }
    void* GetPerObjectBuffer() { return _perObjectBuffer; }

    void SetChunkManager(ChunkManager* cm) { _chunkManager = cm; }
private:
    IRendererApi* _renderer;
    ShaderCollection* _shaderCollection;

    int _width = 0;
    int _height = 0;

    // Owned sub-systems
    PostProcessPipeline* _postProcessPipeline = nullptr;

    // Per-frame constant buffers (owned)
    void* _perFrameBuffer = nullptr;
    void* _perObjectBuffer = nullptr;

    // References (not owned — set each frame or once)
    Camera* _camera = nullptr;
    ChunkManager* _chunkManager = nullptr;

    // Renderer
    VoxelRenderer* _voxelRenderer = nullptr;
    DistantTerrainRenderer* _distantTerrain = nullptr;
    SkyRenderer* _skyRenderer = nullptr;
    DebugLineRenderer* _debugRenderer = nullptr;
    EntityRenderer* _entityRenderer = nullptr;

    bool _debugCameraMode = false;



    // Render passes
    void UpdatePerFrameBuffer(float totalTime);
    void RenderSky(float totalTime);
    void RenderVoxels();
    void RenderEntities();
    void RenderDebug();
};

#endif //VOXTAU_RENDERPIPELINE_H