//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_IRENDERER_H
#define DIRECTX11_IRENDERER_H
#pragma once
#include <d3d11_4.h>
#include <Renderer/RenderStates.h>
#include <EngineApi.h>
#include <vector>

class Mesh;
class Texture;
class TextureData;
class TextureArray;
class RenderTarget;
class Shader;
class ShaderProgram;

enum class ShaderStage {
    Vertex   = 1 << 0,
    Pixel    = 1 << 1,
    Geometry = 1 << 2,
    Compute  = 1 << 3,
    All      = Vertex | Pixel | Geometry | Compute
};

// Allow bitwise operations on ShaderStage
inline ShaderStage operator|(ShaderStage a, ShaderStage b) {
    return static_cast<ShaderStage>(static_cast<int>(a) | static_cast<int>(b));
}

inline bool operator&(ShaderStage a, ShaderStage b) {
    return (static_cast<int>(a) & static_cast<int>(b)) != 0;
}

class IRendererApi
{
    public:
    virtual ~IRendererApi() = default;

    // Initialization
    virtual bool Initialize(void* windowHandle, int width, int height) = 0;
    virtual void Shutdown() = 0;

    // Frame control
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void Present(bool vsync) = 0;
    virtual void Clear(float r, float g, float b, float a) = 0;
    virtual void ClearDepth() = 0;

    // Viewport
    virtual void SetViewport(int x, int y, int width, int height) = 0;
    virtual void ResizeBuffers(int width, int height) = 0;

    // Resource creation
    virtual bool CreateMesh(Mesh* mesh) = 0;
    virtual bool CreateMeshInputLayout(Mesh* mesh, Shader* vertexShader) = 0;
    virtual bool CreateTexture(Texture* texture, const TextureData& data) = 0;
    virtual RenderTarget* CreateRenderTarget(int width, int height, bool withDepth) = 0;

    // CONSTANT BUFFERS
    virtual void* CreateConstantBuffer(size_t size) = 0;
    virtual void UpdateConstantBuffer(void* buffer, const void* data, size_t size) = 0;
    virtual void BindConstantBuffer(int slot, void* buffer, ShaderStage stage) = 0;
    virtual void DestroyConstantBuffer(void* buffer) = 0;

    // Shader management
    virtual bool CompileShader(Shader* shader, const std::string& source) = 0;
    virtual void BindShader(ShaderProgram* program) = 0;
    virtual void UnbindShader() = 0;

    // Texture binding
    virtual void BindTexture(int slot, Texture* texture) = 0;
    virtual void UnbindTexture(int slot) = 0;
    virtual void SetTextureFilter(TextureFilter filter) = 0;

    // Texture array support
    virtual bool CreateTextureArray(TextureArray* textureArray, const std::vector<TextureData*>& textures) = 0;
    virtual void BindTextureArray(int slot, TextureArray* textureArray) = 0;

    // Rendering
    virtual void Draw(Mesh* mesh) = 0;
    virtual void DrawInstanced(Mesh* mesh, int instanceCount) = 0;
    virtual void DrawIndexed(int indexCount) = 0;
    virtual void DrawIndexedInstancedIndirect(void* argsBuffer, uint32_t byteOffset = 0) = 0;

    // Standalone index buffer (for indirect drawing without a Mesh)
    virtual void* CreateIndexBuffer(const uint32_t* data, size_t indexCount) = 0;
    virtual void BindIndexBuffer(void* indexBuffer) = 0;
    virtual void DestroyIndexBuffer(void* indexBuffer) = 0;
    virtual void SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topology) = 0;

    // RENDER STATE MANAGEMENT
    virtual void SetRasterizerMode(RasterizerMode mode) = 0;
    virtual void SetBlendMode(BlendMode mode) = 0;
    virtual void SetDepthMode(DepthMode mode) = 0;
    virtual void SetDepthTestEnabled(bool enabled) = 0;
    virtual void SetDepthWriteEnabled(bool enabled) = 0;

    // Render targets
    virtual void SetRenderTarget(RenderTarget* target) = 0;
    virtual void SetDefaultRenderTarget() = 0;

    // Compute shader support
    virtual void BindComputeShader(Shader* shader) = 0;
    virtual void UnbindComputeShader() = 0;
    virtual void Dispatch(int groupsX, int groupsY, int groupsZ) = 0;

    // Byte address buffer (for atomic counters)
    virtual void* CreateByteAddressBuffer(size_t sizeBytes, bool cpuWrite, bool gpuWrite) = 0;
    virtual void* CreateByteAddressBufferUAV(void* buffer) = 0;
    virtual void* CreateByteAddressBufferSRV(void* buffer) = 0;

    // Indirect args buffer
    virtual void* CreateIndirectArgsBuffer(size_t sizeBytes, bool gpuWrite) = 0;
    virtual void* CreateIndirectArgsBufferUAV(void* buffer) = 0;

    // Raw indirect args buffer (DRAWINDIRECT_ARGS + ALLOW_RAW_VIEWS)
    // Needed for RWByteAddressBuffer usage in compute + DrawIndexedInstancedIndirect
    virtual void* CreateRawIndirectArgsBuffer(size_t sizeBytes) = 0;
    virtual void* CreateRawIndirectArgsBufferUAV(void* buffer) = 0;

    virtual void UpdateByteAddressBuffer(void* buffer, const void* data, size_t sizeBytes) = 0;
    // Copy append buffer counter -> another buffer
    virtual void CopyStructureCount(void* destBuffer, uint32_t destByteOffset, void* srcUAV) = 0;

    // UAV support for compute output
    virtual void* CreateTexture2DUAV(int width, int height, int format) = 0;
    virtual void BindUAV(int slot, void* uav) = 0;
    virtual void UnbindUAV(int slot) = 0;
    virtual void* GetUAVTexture(void* uav) = 0;  // Get the texture associated with UAV
    virtual void* GetUAVSRV(void* uav) = 0;      // Get SRV for reading UAV as texture
    virtual void DestroyUAV(void* uav) = 0;

    // 3D texture for voxel data
    virtual void* CreateTexture3D(int width, int height, int depth, const void* data) = 0;
    virtual void UpdateTexture3DRegion(void* texture, int x, int y, int z, int w, int h, int d, const void* data) = 0;
    virtual void BindTexture3DToCompute(int slot, void* texture) = 0;
    virtual void DestroyTexture3D(void* texture) = 0;

    // Bind SRV to compute stage
    virtual void BindTextureToCompute(int slot, Texture* texture) = 0;
    virtual void BindSamplerToCompute(int slot) = 0;

    // Readable depth buffer (can be used both as DSV and SRV)
    virtual RenderTarget* CreateRenderTargetWithReadableDepth(int width, int height) = 0;
    virtual void* GetDepthSRV(RenderTarget* target) = 0;
    virtual void BindDepthToCompute(int slot, RenderTarget* target) = 0;

    // Getters
    virtual ID3D11Device* GetDevice() const = 0;
    virtual ID3D11DeviceContext* GetContext() const = 0;
    virtual int GetViewportWidth() const = 0;
    virtual int GetViewportHeight() const = 0;

    // Occlusion queries
    virtual void* CreateOcclusionQuery() = 0;
    virtual void DestroyOcclusionQuery(void* query) = 0;
    virtual void BeginOcclusionQuery(void* query) = 0;
    virtual void EndOcclusionQuery(void* query) = 0;
    virtual bool GetOcclusionQueryResult(void* query, uint64_t& pixelCount, bool wait = false) = 0;

    // STRUCTURED BUFFER SUPPORT (for GPU culling)

    // Create a structured buffer for GPU use
    // elemSize: size of each element in bytes
    // count: maximum number of elements
    // cpuWrite: if true, CPU can write to buffer (D3D11_USAGE_DYNAMIC)
    // gpuWrite: if true, GPU can write to buffer (requires UAV)
    virtual void* CreateStructuredBuffer(size_t elemSize, size_t count, bool cpuWrite, bool gpuWrite, const void* initialData = nullptr) = 0;

    // Create shader resource view for reading structured buffer
    virtual void* CreateStructuredBufferSRV(void* buffer) = 0;

    // Create unordered access view for writing to structured buffer
    virtual void* CreateStructuredBufferUAV(void* buffer, bool appendConsume = false) = 0;

    // Update structured buffer data from CPU
    virtual void UpdateStructuredBuffer(void* buffer, const void* data, size_t size) = 0;

    // Bind structured buffer SRV to shader stage
    virtual void BindStructuredBufferSRV(int slot, void* srv, ShaderStage stage) = 0;

    // Bind structured buffer UAV for compute shader output
    virtual void BindStructuredBufferUAV(int slot, void* uav, uint32_t initialCount = 0xFFFFFFFF) = 0;

    // Unbind structured buffer UAV
    virtual void UnbindStructuredBufferUAV(int slot) = 0;

    // Destroy structured buffer and associated views
    virtual void DestroyStructuredBuffer(void* buffer) = 0;

    // STAGING BUFFER SUPPORT (GPU->CPU readback)

    // Create a staging buffer for GPU->CPU data transfer
    virtual void* CreateStagingBuffer(size_t size) = 0;

    // Copy data from GPU buffer to staging buffer
    virtual void CopyBufferToStaging(void* staging, void* gpuBuffer, size_t size) = 0;

    // Copy structured count from append/consume buffer to staging
    virtual void CopyStructureCount(void* staging, void* uav) = 0;

    // Map staging buffer for CPU read
    virtual void* MapStagingBuffer(void* staging) = 0;

    // Unmap staging buffer
    virtual void UnmapStagingBuffer(void* staging) = 0;

    // Destroy staging buffer
    virtual void DestroyStagingBuffer(void* staging) = 0;

    virtual void UnbindComputeResources() = 0;
};

#endif //DIRECTX11_IRENDERER_H