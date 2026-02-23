//
// VoxelRenderer — Encapsulates GPU/CPU chunk rendering pipeline
//

#include "VoxelRenderer.h"

#include <iostream>

#include "Core/Voxel/Chunk.h"
#include "Core/Math/MathTypes.h"
#include "Core/Profiler/Profiler.h"
#include "Renderer/RenderApi/IRendererApi.h"
#include "Renderer/Shaders/ShaderCollection.h"
#include "Renderer/Shaders/ShaderProgram.h"
#include "Resources/BlockRegistry.h"
#include "Resources/TextureArray.h"

VoxelRenderer::VoxelRenderer(IRendererApi* renderer, ShaderCollection* shaderCollection, BlockRegistry* blockRegistry)
    : _renderer(renderer)
    , _shaderCollection(shaderCollection)
    , _blockRegistry(blockRegistry)
{
}

VoxelRenderer::~VoxelRenderer()
{
    Shutdown();
}

void VoxelRenderer::Initialize(bool useGPUMeshing)
{
    _useGPUMeshing = useGPUMeshing;
    _chunkMeshConstantsCB = _renderer->CreateConstantBuffer(sizeof(ChunkMeshConstants));

    if (_useGPUMeshing) {
        InitializeComputeResources();
    }

    std::cout << "VoxelRenderer initialized. GPU meshing: " << (_useGPUMeshing ? "ON" : "OFF") << std::endl;
}

void VoxelRenderer::Shutdown()
{
    ShutdownComputeResources();

    if (_chunkMeshConstantsCB) {
        _renderer->DestroyConstantBuffer(_chunkMeshConstantsCB);
        _chunkMeshConstantsCB = nullptr;
    }
}

// Chunk Registration

void VoxelRenderer::RegisterChunk(Chunk* chunk, const Math::Vector3& worldPos)
{
    if (_chunkGpuData.find(chunk) != _chunkGpuData.end()) {
        return;  // Already registered
    }

    if (chunk->IsEmpty()) {
        return;
    }

    ChunkGpuData data;
    data.worldPos = worldPos;
    data.isDirty = true;

    if (_useGPUMeshing) {
        CreateChunkGpuBuffers(data);
    }

    _chunkGpuData[chunk] = data;
    _meshQueue.push(chunk);
    _cullDataDirty = true;
}

void VoxelRenderer::UnregisterChunk(Chunk* chunk)
{
    auto it = _chunkGpuData.find(chunk);
    if (it != _chunkGpuData.end()) {
        DestroyChunkGpuBuffers(it->second);
        _chunkGpuData.erase(it);
        _cullDataDirty = true;
    }
}

void VoxelRenderer::MarkChunkDirty(Chunk* chunk)
{
    auto it = _chunkGpuData.find(chunk);
    if (it != _chunkGpuData.end()) {
        // Already registered — just mark dirty
        if (!it->second.isDirty) {
            it->second.isDirty = true;
            _meshQueue.push(chunk);
        }
    } else if (!chunk->IsEmpty()) {
        // Was previously empty but now has blocks — register it
        // Need worldPos... This requires either storing it or
        // computing it from the chunk. For now, use chunk's own position:
        Math::Vector3 chunkPos = chunk->GetWorldPosition();
        Math::Vector3 worldPos(
            chunkPos.x * Chunk::CHUNK_SIZE,
            chunkPos.y * Chunk::CHUNK_SIZE,
            chunkPos.z * Chunk::CHUNK_SIZE
        );
        RegisterChunk(chunk, worldPos);
    }
}

// Mesh Queue Processing

void VoxelRenderer::ProcessMeshQueue(uint32_t maxPerFrame)
{
    if (!_useGPUMeshing) return;

    PROFILE_SCOPE("ProcessGpuMeshQueue");

    uint32_t processed = 0;

    while (!_meshQueue.empty() && processed < maxPerFrame) {
        Chunk* chunk = _meshQueue.front();
        _meshQueue.pop();

        auto it = _chunkGpuData.find(chunk);
        if (it == _chunkGpuData.end()) {
            continue;  // Chunk was destroyed
        }

        ChunkGpuData& data = it->second;
        if (!data.isDirty) {
            continue;  // Already up to date
        }

        // Upload voxel data from CPU to GPU
        UploadVoxelData(chunk, data);

        // Dispatch mesh generation compute shader
        DispatchChunkMesh(data);

        // Dispatch draw args builder
        DispatchBuildDrawArgs(data);

        // Read back face count for culling
        data.faceCount = ReadBackFaceCount(data);
        if (data.faceCount == 0) {
            DestroyChunkGpuBuffers(data);
            // Keep in _chunkGpuData but with isInitialized=false
            // so it's skipped in render loop
        }
        data.isDirty = false;
        _cullDataDirty = true;
        processed++;
    }

    // Unbind compute resources so buffers can be used for rendering
    if (processed > 0) {
        _renderer->UnbindComputeResources();
    }
}

// Rendering

void VoxelRenderer::Render(const Math::Matrix4x4& viewProjection, void* perFrameBuffer, void* perObjectBuffer, TextureArray* blockTextureArray)
{
    if (_useGPUMeshing) {
        RenderChunksGPU(viewProjection, perFrameBuffer, perObjectBuffer, blockTextureArray);
    } else {
        RenderChunksCPU(perFrameBuffer, perObjectBuffer, blockTextureArray);
    }
}

void VoxelRenderer::RenderChunksGPU(const Math::Matrix4x4& viewProjection, void* perFrameBuffer, void* perObjectBuffer, TextureArray* textureArray)
{
    PROFILE_SCOPE("RenderChunksGPU");

    ShaderProgram* voxelShader = _shaderCollection->GetVoxelShader();
    if (!voxelShader) return;

    _renderer->BindShader(voxelShader);

    if (textureArray) {
        _renderer->BindTextureArray(0, textureArray);
    }

    // Run frustum cull compute pass
    DispatchFrustumCull(viewProjection);

    // Bind shared quad index buffer and set topology for indirect drawing
    _renderer->BindIndexBuffer(_quadIndexBuffer);
    _renderer->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Render each chunk via shared draw args buffer
    for (auto& [chunk, data] : _chunkGpuData) {
        if (data.isDirty || !data.isInitialized) continue;
        if (data.faceCount == 0) continue;

        // Update per-object constants with chunk world position (int3)
        ChunkPositionBuffer chunkPos;
        chunkPos.chunkWorldPos[0] = static_cast<int32_t>(data.worldPos.x);
        chunkPos.chunkWorldPos[1] = static_cast<int32_t>(data.worldPos.y);
        chunkPos.chunkWorldPos[2] = static_cast<int32_t>(data.worldPos.z);
        chunkPos.voxelScale = VOXEL_SCALE;
        _renderer->UpdateConstantBuffer(perObjectBuffer, &chunkPos, sizeof(ChunkPositionBuffer));
        _renderer->BindConstantBuffer(1, perObjectBuffer, ShaderStage::Vertex);

        // Bind this chunk's vertex buffer as SRV for vertex shader
        _renderer->BindStructuredBufferSRV(0, data.vertexSRV, ShaderStage::Vertex);

        // Draw using shared indirect args — frustum cull zeroed out culled chunks
        uint32_t drawArgsOffset = data.chunkIndex * DRAW_ARGS_STRIDE;
        _renderer->DrawIndexedInstancedIndirect(_sharedDrawArgsBuffer, drawArgsOffset);
    }

    _renderer->UnbindShader();
}

void VoxelRenderer::RenderChunksCPU(void* perFrameBuffer, void* perObjectBuffer, TextureArray* textureArray)
{
    PROFILE_SCOPE("RenderChunksCPU");

    ShaderProgram* voxelShader = _shaderCollection->GetVoxelShader();
    if (!voxelShader) {
        std::cerr << "ERROR: Voxel shader is null!" << std::endl;
        return;
    }

    _renderer->BindShader(voxelShader);

    if (textureArray) {
        _renderer->BindTextureArray(0, textureArray);
    }

    Shader* vertexShader = voxelShader->GetVertexShader();

    for (auto& [chunk, data] : _chunkGpuData) {
        if (!chunk) continue;

        PerObjectBuffer perObject;
        perObject.world = Math::Matrix4x4::Identity;
        perObject.worldInverseTranspose = Math::Matrix4x4::Identity;
        _renderer->UpdateConstantBuffer(perObjectBuffer, &perObject, sizeof(PerObjectBuffer));
        _renderer->BindConstantBuffer(1, perObjectBuffer, ShaderStage::Vertex);

        chunk->Render(_renderer, vertexShader);
    }

    _renderer->UnbindShader();
}

// Compute Resources

void VoxelRenderer::InitializeComputeResources()
{
    std::cout << "Initializing GPU compute resources..." << std::endl;

    // Generate shared quad index buffer
    auto indices = GenerateQuadIndices(Chunk::MAX_FACES_PER_CHUNK);

    _quadIndexBuffer = _renderer->CreateIndexBuffer(
        indices.data(),
        indices.size()
    );

    std::cout << "Quad index buffer created: " << indices.size() << " indices" << std::endl;

    // Build block texture lookup table
    BuildBlockTextureMap();

    // ── Frustum culling resources ──
    _frustumCullCB = _renderer->CreateConstantBuffer(sizeof(FrustumCullConstants));

    // Chunk cull data (StructuredBuffer<ChunkCullData>)
    _chunkCullDataBuffer = _renderer->CreateStructuredBuffer(
        sizeof(ChunkCullData), MAX_CHUNKS, true, false);
    _chunkCullDataSRV = _renderer->CreateStructuredBufferSRV(_chunkCullDataBuffer);

    // Shared draw args (RWByteAddressBuffer + DrawIndexedInstancedIndirect)
    size_t sharedArgsSize = MAX_CHUNKS * DRAW_ARGS_STRIDE;
    _sharedDrawArgsBuffer = _renderer->CreateRawIndirectArgsBuffer(sharedArgsSize);
    _sharedDrawArgsUAV = _renderer->CreateRawIndirectArgsBufferUAV(_sharedDrawArgsBuffer);

    // Staging buffer for vertex count readback (4 bytes)
    _counterStagingBuffer = _renderer->CreateStagingBuffer(4);

    std::cout << "Frustum culling resources created (max " << MAX_CHUNKS << " chunks)" << std::endl;
}

void VoxelRenderer::ShutdownComputeResources()
{
    // Destroy all per-chunk GPU data
    for (auto& [chunk, data] : _chunkGpuData) {
        DestroyChunkGpuBuffers(data);
    }
    _chunkGpuData.clear();

    // Clear mesh queue
    while (!_meshQueue.empty()) {
        _meshQueue.pop();
    }

    // Destroy shared meshing resources
    if (_quadIndexBuffer) {
        _renderer->DestroyIndexBuffer(_quadIndexBuffer);
        _quadIndexBuffer = nullptr;
    }

    if (_blockTextureMapBuffer) {
        _renderer->DestroyStructuredBuffer(_blockTextureMapBuffer);
        _blockTextureMapBuffer = nullptr;
        _blockTextureMapSRV = nullptr;
    }

    // Destroy frustum culling resources
    if (_frustumCullCB) {
        _renderer->DestroyConstantBuffer(_frustumCullCB);
        _frustumCullCB = nullptr;
    }

    if (_chunkCullDataBuffer) {
        _renderer->DestroyStructuredBuffer(_chunkCullDataBuffer);
        _chunkCullDataBuffer = nullptr;
        _chunkCullDataSRV = nullptr;
    }

    if (_sharedDrawArgsUAV) {
        auto* uav = static_cast<ID3D11UnorderedAccessView*>(_sharedDrawArgsUAV);
        uav->Release();
        _sharedDrawArgsUAV = nullptr;
    }

    if (_sharedDrawArgsBuffer) {
        auto* buf = static_cast<ID3D11Buffer*>(_sharedDrawArgsBuffer);
        buf->Release();
        _sharedDrawArgsBuffer = nullptr;
    }

    if (_counterStagingBuffer) {
        _renderer->DestroyStagingBuffer(_counterStagingBuffer);
        _counterStagingBuffer = nullptr;
    }
}

// Per-Chunk Buffer Management

void VoxelRenderer::CreateChunkGpuBuffers(ChunkGpuData& data)
{
    // Padded voxel buffer: 34³ = 39304 voxels, packed 4 per uint = 9826 uints
    constexpr uint32_t PADDED_SIZE = 34;
    constexpr uint32_t PADDED_VOXEL_COUNT = PADDED_SIZE * PADDED_SIZE * PADDED_SIZE;
    constexpr uint32_t PADDED_PACKED_COUNT = (PADDED_VOXEL_COUNT + 3) / 4; // 9826

    data.voxelBuffer = _renderer->CreateStructuredBuffer(
        sizeof(uint32_t), PADDED_PACKED_COUNT, true, false);
    data.voxelSRV = _renderer->CreateStructuredBufferSRV(data.voxelBuffer);

    // Everything else stays the same:
    data.vertexBuffer = _renderer->CreateStructuredBuffer(
        4, Chunk::MAX_FACES_PER_CHUNK * 2, false, true);
    data.vertexSRV = _renderer->CreateStructuredBufferSRV(data.vertexBuffer);
    data.vertexUAV = _renderer->CreateStructuredBufferUAV(data.vertexBuffer, false);

    data.counterBuffer = _renderer->CreateByteAddressBuffer(4, true, true);
    data.counterUAV = _renderer->CreateByteAddressBufferUAV(data.counterBuffer);

    data.drawArgsBuffer = _renderer->CreateIndirectArgsBuffer(20, true);
    data.drawArgsUAV = _renderer->CreateIndirectArgsBufferUAV(data.drawArgsBuffer);

    data.isInitialized = true;
}

void VoxelRenderer::DestroyChunkGpuBuffers(ChunkGpuData& data)
{
    if (!data.isInitialized) return;

    if (data.voxelBuffer) _renderer->DestroyStructuredBuffer(data.voxelBuffer);
    if (data.vertexBuffer) _renderer->DestroyStructuredBuffer(data.vertexBuffer);
    if (data.counterBuffer) _renderer->DestroyStructuredBuffer(data.counterBuffer);
    if (data.drawArgsBuffer) _renderer->DestroyStructuredBuffer(data.drawArgsBuffer);

    data = ChunkGpuData();
}

// Compute Dispatch

void VoxelRenderer::UploadVoxelData(Chunk* chunk, ChunkGpuData& data)
{
    auto packedData = PackVoxelData(chunk);
    _renderer->UpdateStructuredBuffer(
        data.voxelBuffer,
        packedData.data(),
        packedData.size() * sizeof(uint32_t)
    );
}

void VoxelRenderer::DispatchChunkMesh(ChunkGpuData& data)
{
    uint32_t zero = 0;
    _renderer->UpdateByteAddressBuffer(data.counterBuffer, &zero, sizeof(zero));

    ChunkMeshConstants constants;
    constants.chunkWorldPos[0] = data.worldPos.x;
    constants.chunkWorldPos[1] = data.worldPos.y;
    constants.chunkWorldPos[2] = data.worldPos.z;
    constants.maxFaces = Chunk::MAX_FACES_PER_CHUNK;
    _renderer->UpdateConstantBuffer(_chunkMeshConstantsCB, &constants, sizeof(constants));

    Shader* meshShader = _shaderCollection->GetComputeShader("ChunkMesh");
    if (!meshShader) return;

    _renderer->BindComputeShader(meshShader);
    _renderer->BindStructuredBufferSRV(0, data.voxelSRV, ShaderStage::Compute);
    _renderer->BindStructuredBufferSRV(7, _blockTextureMapSRV, ShaderStage::Compute);
    _renderer->BindStructuredBufferUAV(0, data.vertexUAV, 0);
    _renderer->BindStructuredBufferUAV(1, data.counterUAV);
    _renderer->BindConstantBuffer(0, _chunkMeshConstantsCB, ShaderStage::Compute);

    _renderer->Dispatch(4, 4, 4);
}

void VoxelRenderer::DispatchBuildDrawArgs(ChunkGpuData& data)
{
    Shader* argsShader = _shaderCollection->GetComputeShader("BuildDrawArgs");
    if (!argsShader) {
        std::cerr << "ERROR: BuildDrawArgs compute shader not found!" << std::endl;
        return;
    }

    _renderer->BindComputeShader(argsShader);
    _renderer->BindStructuredBufferUAV(0, data.counterUAV);
    _renderer->BindStructuredBufferUAV(1, data.drawArgsUAV);

    _renderer->Dispatch(1, 1, 1);
}

uint32_t VoxelRenderer::ReadBackFaceCount(ChunkGpuData& data)
{
    // Unbind compute UAVs so we can read from the counter buffer
    _renderer->UnbindComputeResources();

    // Direct D3D11 copy: counterBuffer (ByteAddressBuffer) → staging
    ID3D11DeviceContext* ctx = _renderer->GetContext();
    ID3D11Buffer* srcBuffer = static_cast<ID3D11Buffer*>(data.counterBuffer);
    ID3D11Buffer* stagingBuffer = static_cast<ID3D11Buffer*>(_counterStagingBuffer);

    D3D11_BOX srcBox = {};
    srcBox.left = 0;
    srcBox.right = 4;
    srcBox.top = 0;
    srcBox.bottom = 1;
    srcBox.front = 0;
    srcBox.back = 1;
    ctx->CopySubresourceRegion(stagingBuffer, 0, 0, 0, 0, srcBuffer, 0, &srcBox);

    // Map and read
    void* mapped = _renderer->MapStagingBuffer(_counterStagingBuffer);
    uint32_t count = 0;
    if (mapped) {
        count = *static_cast<uint32_t*>(mapped);
        _renderer->UnmapStagingBuffer(_counterStagingBuffer);
    }

    return count;
}

// Frustum Culling

void VoxelRenderer::UpdateChunkCullData()
{
    if (!_cullDataDirty) return;

    if (_chunkGpuData.size() > MAX_CHUNKS) {
        std::cerr << "WARNING: Chunk count (" << _chunkGpuData.size()
                  << ") exceeds MAX_CHUNKS (" << MAX_CHUNKS << "). Culling capped." << std::endl;
    }

    std::vector<ChunkCullData> cullData;
    cullData.reserve(std::min(static_cast<uint32_t>(_chunkGpuData.size()), MAX_CHUNKS));

    uint32_t index = 0;
    for (auto& [chunk, data] : _chunkGpuData) {
        if (index >= MAX_CHUNKS) break;

        data.chunkIndex = index;

        ChunkCullData cd;
        cd.aabbMin[0] = data.worldPos.x * VOXEL_SCALE;
        cd.aabbMin[1] = data.worldPos.y * VOXEL_SCALE;
        cd.aabbMin[2] = data.worldPos.z * VOXEL_SCALE;
        cd.drawArgsOffset = index * DRAW_ARGS_STRIDE;
        cd.aabbMax[0] = (data.worldPos.x + Chunk::CHUNK_SIZE) * VOXEL_SCALE;
        cd.aabbMax[1] = (data.worldPos.y + Chunk::CHUNK_SIZE) * VOXEL_SCALE;
        cd.aabbMax[2] = (data.worldPos.z + Chunk::CHUNK_SIZE) * VOXEL_SCALE;
        cd.faceCount = data.faceCount;

        cullData.push_back(cd);
        index++;
    }

    _renderer->UpdateStructuredBuffer(
        _chunkCullDataBuffer,
        cullData.data(),
        cullData.size() * sizeof(ChunkCullData)
    );

    _cullDataDirty = false;
}

void VoxelRenderer::DispatchFrustumCull(const Math::Matrix4x4& viewProjection)
{
    PROFILE_SCOPE("DispatchFrustumCull");

    if (_chunkGpuData.empty()) return;

    // Update chunk cull data if chunks changed
    UpdateChunkCullData();

    FrustumCullConstants cullConstants;

    if (_frustumCullingEnabled) {
        // Extract frustum planes from VP matrix
        _frustum.ExtractFromMatrix(viewProjection);
        _frustum.GetPlanes(cullConstants.frustumPlanes);
    } else {
        // All-pass planes: normal=(0,0,0), w=1e9 — dot is always huge, nothing culled
        for (int i = 0; i < 6; i++) {
            cullConstants.frustumPlanes[i] = Math::Vector4(0.0f, 0.0f, 0.0f, 1e9f);
        }
    }

    cullConstants.chunkCount = static_cast<uint32_t>(std::min(static_cast<size_t>(MAX_CHUNKS), _chunkGpuData.size()));
    cullConstants._pad[0] = 0;
    cullConstants._pad[1] = 0;
    cullConstants._pad[2] = 0;

    _renderer->UpdateConstantBuffer(_frustumCullCB, &cullConstants, sizeof(cullConstants));

    // Bind frustum cull compute shader
    Shader* cullShader = _shaderCollection->GetComputeShader("FrustumCull");
    if (!cullShader) {
        std::cerr << "ERROR: FrustumCull shader not found!" << std::endl;
        return;
    }

    _renderer->BindComputeShader(cullShader);
    _renderer->BindStructuredBufferSRV(0, _chunkCullDataSRV, ShaderStage::Compute);
    _renderer->BindStructuredBufferUAV(0, _sharedDrawArgsUAV);
    _renderer->BindConstantBuffer(0, _frustumCullCB, ShaderStage::Compute);

    // 1 thread per chunk, 64 threads per group
    uint32_t groupCount = (cullConstants.chunkCount + 63) / 64;
    _renderer->Dispatch(groupCount, 1, 1);

    _renderer->UnbindComputeResources();
}

// Helpers

void VoxelRenderer::BuildBlockTextureMap()
{
    // Build a lookup table: blockTextureMap[blockType * 6 + computeFaceIdx] = textureArrayIndex
    // 256 possible block types x 6 faces = 1536 entries
    constexpr uint32_t TABLE_SIZE = 256 * 6;
    std::vector<uint32_t> textureMap(TABLE_SIZE, 0);

    // The compute shader uses face indices:
    //   0: +X, 1: -X, 2: +Y, 3: -Y, 4: +Z, 5: -Z
    // BlockRegistry uses BlockFace enum:
    //   Front=0(+Z), Back=1(-Z), Left=2(-X), Right=3(+X), Top=4(+Y), Bottom=5(-Y)
    // Remap: compute face -> BlockRegistry face
    static const int computeToBlockFace[6] = { 3, 2, 4, 5, 0, 1 };

    for (uint32_t blockType = 0; blockType < 256; blockType++) {
        for (uint32_t computeFace = 0; computeFace < 6; computeFace++) {
            int blockFace = computeToBlockFace[computeFace];
            textureMap[blockType * 6 + computeFace] =
                _blockRegistry->GetTextureIndex(static_cast<uint8_t>(blockType), blockFace);
        }
    }

    // Create GPU buffer (read-only structured buffer)
    _blockTextureMapBuffer = _renderer->CreateStructuredBuffer(
        sizeof(uint32_t), TABLE_SIZE, true, false, textureMap.data());
    _blockTextureMapSRV = _renderer->CreateStructuredBufferSRV(_blockTextureMapBuffer);

    std::cout << "Block texture map created: " << TABLE_SIZE << " entries" << std::endl;
}

std::vector<uint32_t> VoxelRenderer::GenerateQuadIndices(uint32_t maxFaces)
{
    std::vector<uint32_t> indices(maxFaces * 6);

    for (uint32_t i = 0; i < maxFaces; i++) {
        uint32_t base = i * 4;
        indices[i * 6 + 0] = base + 0;
        indices[i * 6 + 1] = base + 1;
        indices[i * 6 + 2] = base + 2;
        indices[i * 6 + 3] = base + 0;
        indices[i * 6 + 4] = base + 2;
        indices[i * 6 + 5] = base + 3;
    }

    return indices;
}

std::vector<uint32_t> VoxelRenderer::PackVoxelData(Chunk* chunk)
{
    constexpr uint32_t PAD = 34;
    constexpr uint32_t TOTAL_VOXELS = PAD * PAD * PAD;       // 39304
    constexpr uint32_t PACKED_COUNT = (TOTAL_VOXELS + 3) / 4; // 9826
    constexpr int CS = static_cast<int>(Chunk::CHUNK_SIZE);    // 32

    std::vector<uint8_t> voxels(TOTAL_VOXELS, 0);

    // Chunk position in chunk coordinates
    Math::Vector3 chunkPos = chunk->GetWorldPosition();
    int cx = static_cast<int>(chunkPos.x);
    int cy = static_cast<int>(chunkPos.y);
    int cz = static_cast<int>(chunkPos.z);

    // Cache 6 neighbor chunk pointers (one lookup each, not per-voxel)
    Chunk* nXPos = _chunkManager ? _chunkManager->GetChunk(cx + 1, cy, cz) : nullptr;
    Chunk* nXNeg = _chunkManager ? _chunkManager->GetChunk(cx - 1, cy, cz) : nullptr;
    Chunk* nYPos = _chunkManager ? _chunkManager->GetChunk(cx, cy + 1, cz) : nullptr;
    Chunk* nYNeg = _chunkManager ? _chunkManager->GetChunk(cx, cy - 1, cz) : nullptr;
    Chunk* nZPos = _chunkManager ? _chunkManager->GetChunk(cx, cy, cz + 1) : nullptr;
    Chunk* nZNeg = _chunkManager ? _chunkManager->GetChunk(cx, cy, cz - 1) : nullptr;

    // Fill interior (0..31) directly from chunk — no indirection
    for (int y = 0; y < CS; y++) {
        for (int z = 0; z < CS; z++) {
            for (int x = 0; x < CS; x++) {
                uint32_t px = static_cast<uint32_t>(x + 1);
                uint32_t py = static_cast<uint32_t>(y + 1);
                uint32_t pz = static_cast<uint32_t>(z + 1);
                voxels[px + pz * PAD + py * PAD * PAD] = chunk->GetBlock(x, y, z);
            }
        }
    }

    // Fill 6 border faces from cached neighbor pointers
    // Each face is 32x32 = 1024 voxels

    // -X face (x = -1 in local, x = 31 in neighbor)
    for (int y = 0; y < CS; y++) {
        for (int z = 0; z < CS; z++) {
            uint8_t val = nXNeg ? nXNeg->GetBlock(CS - 1, y, z) : 0;
            voxels[0 + (z + 1) * PAD + (y + 1) * PAD * PAD] = val;
        }
    }

    // +X face (x = 32 in local, x = 0 in neighbor)
    for (int y = 0; y < CS; y++) {
        for (int z = 0; z < CS; z++) {
            uint8_t val = nXPos ? nXPos->GetBlock(0, y, z) : 0;
            voxels[33 + (z + 1) * PAD + (y + 1) * PAD * PAD] = val;
        }
    }

    // -Y face (y = -1 in local, y = 31 in neighbor)
    for (int z = 0; z < CS; z++) {
        for (int x = 0; x < CS; x++) {
            uint8_t val = nYNeg ? nYNeg->GetBlock(x, CS - 1, z) : 0;
            voxels[(x + 1) + (z + 1) * PAD + 0 * PAD * PAD] = val;
        }
    }

    // +Y face (y = 32 in local, y = 0 in neighbor)
    for (int z = 0; z < CS; z++) {
        for (int x = 0; x < CS; x++) {
            uint8_t val = nYPos ? nYPos->GetBlock(x, 0, z) : 0;
            voxels[(x + 1) + (z + 1) * PAD + 33 * PAD * PAD] = val;
        }
    }

    // -Z face (z = -1 in local, z = 31 in neighbor)
    for (int y = 0; y < CS; y++) {
        for (int x = 0; x < CS; x++) {
            uint8_t val = nZNeg ? nZNeg->GetBlock(x, y, CS - 1) : 0;
            voxels[(x + 1) + 0 * PAD + (y + 1) * PAD * PAD] = val;
        }
    }

    // +Z face (z = 32 in local, z = 0 in neighbor)
    for (int y = 0; y < CS; y++) {
        for (int x = 0; x < CS; x++) {
            uint8_t val = nZPos ? nZPos->GetBlock(x, y, 0) : 0;
            voxels[(x + 1) + 33 * PAD + (y + 1) * PAD * PAD] = val;
        }
    }

    // Edge and corner border voxels (where 2 or 3 axes are out of bounds)
    // are left as 0 (air). These only affect AO at chunk corners/edges,
    // and the visual error is imperceptible. Filling them correctly would
    // require diagonal neighbor lookups which isn't worth the complexity.

    // Pack 4 voxels per uint32
    std::vector<uint32_t> packed(PACKED_COUNT, 0);
    for (uint32_t i = 0; i < TOTAL_VOXELS; i += 4) {
        uint32_t v0 = voxels[i];
        uint32_t v1 = (i + 1 < TOTAL_VOXELS) ? voxels[i + 1] : 0;
        uint32_t v2 = (i + 2 < TOTAL_VOXELS) ? voxels[i + 2] : 0;
        uint32_t v3 = (i + 3 < TOTAL_VOXELS) ? voxels[i + 3] : 0;
        packed[i / 4] = v0 | (v1 << 8) | (v2 << 16) | (v3 << 24);
    }

    return packed;
}