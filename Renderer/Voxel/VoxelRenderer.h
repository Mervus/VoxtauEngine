//
// Created by Marvin on 06/02/2026.
//

#ifndef VOXTAU_VOXELRENDERER_H
#define VOXTAU_VOXELRENDERER_H
#pragma once

#include <queue>
#include <unordered_map>
#include <vector>
#include <cstdint>

#include "Core/Math/Frustum.h"
#include "Renderer/Shaders/ShaderTypes.h"
#include <Core/Voxel/ChunkManager.h>

class IRendererApi;
class ShaderCollection;
class BlockRegistry;
class TextureArray;
class Chunk;

class VoxelRenderer {
public:
    VoxelRenderer(IRendererApi* renderer, ShaderCollection* shaderCollection, BlockRegistry* blockRegistry);
    VoxelRenderer(IRendererApi* renderer, ShaderCollection* shaderCollection);
    ~VoxelRenderer();

    void Initialize(bool useGPUMeshing = true);
    void Shutdown();

    // Chunk registration (idempotent — safe to call every frame)
    void RegisterChunk(Chunk* chunk, const Math::Vector3& worldPos);
    void UnregisterChunk(Chunk* chunk);
    void MarkChunkDirty(Chunk* chunk);

    // Process dirty chunks through compute pipeline (call from Update)
    void ProcessMeshQueue(uint32_t maxPerFrame = 4);

    // Render all registered chunks
    void Render(const Math::Matrix4x4& viewProjection, void* perFrameBuffer, void* perObjectBuffer, TextureArray* blockTextureArray);

    bool IsGPUMeshing() const { return _useGPUMeshing; }
    size_t GetRegisteredChunkCount() const { return _chunkGpuData.size(); }

    void SetFrustumCullingEnabled(bool enabled) { _frustumCullingEnabled = enabled; }
    void SetBlockTextureArray(TextureArray* array) { _blockTextures = array; }
    void SetChunkManager(ChunkManager* cm) { _chunkManager = cm; }
    [[nodiscard]] TextureArray* GetBlockTextureArray() const { return _blockTextures; }
    [[nodiscard]] BlockRegistry* GetBlockRegistry() const { return _blockRegistry; }
private:
    IRendererApi* _renderer;
    ShaderCollection* _shaderCollection;
    BlockRegistry* _blockRegistry;
    ChunkManager* _chunkManager;
    TextureArray* _blockTextures;

    bool _useGPUMeshing = true;
    bool _frustumCullingEnabled = true;

    // Per-chunk GPU data
    std::unordered_map<Chunk*, ChunkGpuData> _chunkGpuData;
    std::queue<Chunk*> _meshQueue;

    // Shared GPU resources (meshing)
    void* _chunkMeshConstantsCB = nullptr;
    void* _quadIndexBuffer = nullptr;
    void* _blockTextureMapBuffer = nullptr;
    void* _blockTextureMapSRV = nullptr;

    // Shared GPU resources (frustum culling)
    void* _frustumCullCB = nullptr;
    void* _chunkCullDataBuffer = nullptr;
    void* _chunkCullDataSRV = nullptr;
    void* _sharedDrawArgsBuffer = nullptr;
    void* _sharedDrawArgsUAV = nullptr;
    void* _counterStagingBuffer = nullptr;

    bool _cullDataDirty = true;
    Math::Frustum _frustum;

    static constexpr uint32_t MAX_CHUNKS = 20480;
    static constexpr float VOXEL_SCALE = 1.0f;

    // Internal lifecycle
    void InitializeComputeResources();
    void ShutdownComputeResources();

    // Per-chunk buffer management
    void CreateChunkGpuBuffers(ChunkGpuData& data);
    void DestroyChunkGpuBuffers(ChunkGpuData& data);

    // Compute dispatch
    void UploadVoxelData(Chunk* chunk, ChunkGpuData& data);
    void DispatchChunkMesh(ChunkGpuData& data);
    void DispatchBuildDrawArgs(ChunkGpuData& data);
    void DispatchFrustumCull(const Math::Matrix4x4& viewProjection);

    void UpdateChunkCullData();
    uint32_t ReadBackFaceCount(ChunkGpuData& data);

    // Render paths
    void RenderChunksGPU(const Math::Matrix4x4& viewProjection, void* perFrameBuffer, void* perObjectBuffer, TextureArray* textureArray);
    void RenderChunksCPU(void* perFrameBuffer, void* perObjectBuffer, TextureArray* textureArray);

    // Helpers
    void BuildBlockTextureMap();
    static std::vector<uint32_t> GenerateQuadIndices(uint32_t maxFaces);
    std::vector<uint32_t> PackVoxelData(Chunk* chunk);
};

#endif //VOXTAU_VOXELRENDERER_H
