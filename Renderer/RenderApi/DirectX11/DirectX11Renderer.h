//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_DIRECTX11RENDERER_H
#define DIRECTX11_DIRECTX11RENDERER_H

#include <Renderer/RenderApi/IRendererApi.h>
#include <Resources/ResourceManager.h>

#include <d3d11_4.h>
#include <dxgi.h>
#include <map>
#include <vector>

#include "DirectX11Mesh.h"
#include "DirectX11Texture.h"
#include "DirectX11TextureArray.h"

enum class TextureFormat;
class ShaderCollection;
class IRenderable;
class RenderTarget;

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

struct ComputeShader {
    ID3D11ComputeShader* shader = nullptr;
};

struct GpuBuffer {
    ID3D11Buffer* buffer = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
    ID3D11UnorderedAccessView* uav = nullptr;
    uint32_t elementSize = 0;
    uint32_t elementCount = 0;
};

class ENGINE_API DirectX11Renderer : public IRendererApi
{
    private:
    // CORE D3D11 OBJECTS
    ComPtr<IDXGIFactory4> _dxgiFactory;
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGISwapChain3> swapChain;

    // Render targets
    ComPtr<ID3D11RenderTargetView> backBufferRTV;
    ComPtr<ID3D11DepthStencilView> depthStencilView;
    ComPtr<ID3D11Texture2D> depthStencilBuffer;

#if !defined(NDEBUG)
    ComPtr<ID3D11Debug> _debug;
#endif

    // VIEWPORT & WINDOW INFO
    D3D11_VIEWPORT viewport;
    int windowWidth;
    int windowHeight;
    void* windowHandle; // HWND

    // RASTERIZER STATES
    std::map<RasterizerMode, ComPtr<ID3D11RasterizerState>> rasterizerStates;
    RasterizerMode currentRasterizerMode;

    // DEPTH/STENCIL STATES
    std::map<DepthMode, ComPtr<ID3D11DepthStencilState>> depthStencilStates;
    DepthMode currentDepthMode;
    bool depthTestEnabled;
    bool depthWriteEnabled;

    // BLEND STATES
    std::map<BlendMode, ComPtr<ID3D11BlendState>> blendStates;
    BlendMode currentBlendMode;

    // SAMPLER STATES
    std::map<TextureFilter, ComPtr<ID3D11SamplerState>> samplerStates;
    TextureFilter currentTextureFilter;

    // CURRENTLY BOUND RESOURCES
    ShaderProgram* currentShader;
    RenderTarget* currentRenderTarget; // nullptr means backbuffer

    // Depth SRVs for readable depth buffers (keyed by RenderTarget pointer)
    std::map<RenderTarget*, ComPtr<ID3D11ShaderResourceView>> depthSRVs;


    // HELPER METHODS
    bool CreateDeviceAndSwapChain(void* windowHandle, int width, int height);
    bool CreateRenderTargets();
    bool CreateDepthStencilBuffer(int width, int height);
    bool CreateRasterizerStates();
    bool CreateDepthStencilStates();
    bool CreateBlendStates();
    bool CreateSamplerStates();

    DXGI_FORMAT ConvertToDXGIFormat(TextureFormat format);

public:
    void CleanupDeviceResources();

    DirectX11Renderer();
    ~DirectX11Renderer();

    // INITIALIZATION & CLEANUP
    bool Initialize(void* windowHandle, int width, int height) override;
    void Shutdown() override;

    // FRAME CONTROL
    void BeginFrame() override;
    void EndFrame() override;
    void Present(bool vsync) override;
    void Clear(float r, float g, float b, float a) override;
    void ClearDepth() override;

    // VIEWPORT
    void SetViewport(int x, int y, int width, int height) override;
    void ResizeBuffers(int width, int height) override;

    // Resource creation
    bool CreateMesh(Mesh* mesh) override;
    bool CreateMeshInputLayout(Mesh* mesh, Shader* vertexShader) override;
    bool CreateTexture(Texture* texture, const TextureData& data) override;
    RenderTarget* CreateRenderTarget(int width, int height, bool withDepth) override;

    // Constant Buffers
    void* CreateConstantBuffer(size_t size) override;
    void UpdateConstantBuffer(void* buffer, const void* data, size_t size) override;
    void BindConstantBuffer(int slot, void* buffer, ShaderStage stage) override;
    void DestroyConstantBuffer(void* buffer) override;

    // Shader management
    bool CompileShader(Shader* shader, const std::string& source) override;
    void BindShader(ShaderProgram* program) override;
    void UnbindShader() override;

    // Texture binding
    void BindTexture(int slot, Texture* texture) override;
    void UnbindTexture(int slot) override;
    void SetTextureFilter(TextureFilter filter) override;

    // Texture array support
    bool CreateTextureArray(TextureArray* textureArray, const std::vector<TextureData*>& textures) override;
    void BindTextureArray(int slot, TextureArray* textureArray) override;

    // Rendering
    void Draw(Mesh* mesh) override;
    void DrawInstanced(Mesh* mesh, int instanceCount) override;
    void DrawIndexed(int indexCount) override;

    // RENDER TARGETS
    void SetRenderTarget(RenderTarget* target) override;
    void SetDefaultRenderTarget() override;

    // Render state management
    void SetRasterizerMode(RasterizerMode mode) override;
    void SetBlendMode(BlendMode mode) override;
    void SetDepthMode(DepthMode mode) override;
    void SetDepthTestEnabled(bool enabled) override;
    void SetDepthWriteEnabled(bool enabled) override;

    // GETTERS (for advanced use)
    ID3D11Device* GetDevice() const override;
    ID3D11DeviceContext* GetContext() const override;
    int GetViewportWidth() const override;
    int GetViewportHeight() const override;

    // Compute shader support
    void BindComputeShader(Shader* shader) override;
    void UnbindComputeShader() override;
    void Dispatch(int groupsX, int groupsY, int groupsZ) override;

    // UAV support
    void* CreateTexture2DUAV(int width, int height, int format) override;
    void BindUAV(int slot, void* uav) override;
    void UnbindUAV(int slot) override;
    void* GetUAVTexture(void* uav) override;
    void* GetUAVSRV(void* uav) override;
    void DestroyUAV(void* uav) override;

    // 3D texture support
    void* CreateTexture3D(int width, int height, int depth, const void* data) override;
    void UpdateTexture3DRegion(void* texture, int x, int y, int z, int w, int h, int d, const void* data) override;
    void BindTexture3DToCompute(int slot, void* texture) override;
    void DestroyTexture3D(void* texture) override;

    // Bind texture/sampler to compute stage
    void BindTextureToCompute(int slot, Texture* texture) override;
    void BindSamplerToCompute(int slot) override;

    // Readable depth buffer support
    RenderTarget* CreateRenderTargetWithReadableDepth(int width, int height) override;
    void* GetDepthSRV(RenderTarget* target) override;
    void BindDepthToCompute(int slot, RenderTarget* target) override;

    // Occlusion queries
    void* CreateOcclusionQuery() override;
    void DestroyOcclusionQuery(void* query) override;
    void BeginOcclusionQuery(void* query) override;
    void EndOcclusionQuery(void* query) override;
    bool GetOcclusionQueryResult(void* query, uint64_t& pixelCount, bool wait = false) override;

    // Structured buffer support
    void* CreateStructuredBuffer(size_t elemSize, size_t count, bool cpuWrite, bool gpuWrite, const void* initialData = nullptr) override;
    void* CreateStructuredBufferSRV(void* buffer) override;
    void* CreateStructuredBufferUAV(void* buffer, bool appendConsume = false) override;
    void UpdateStructuredBuffer(void* buffer, const void* data, size_t size) override;
    void BindStructuredBufferSRV(int slot, void* srv, ShaderStage stage) override;
    void BindStructuredBufferUAV(int slot, void* uav, uint32_t initialCount = 0xFFFFFFFF) override;
    void UnbindStructuredBufferUAV(int slot) override;
    void DestroyStructuredBuffer(void* buffer) override;

    // Staging buffer support
    void* CreateStagingBuffer(size_t size) override;
    void CopyBufferToStaging(void* staging, void* gpuBuffer, size_t size) override;
    void CopyStructureCount(void* staging, void* uav) override;
    void* MapStagingBuffer(void* staging) override;
    void UnmapStagingBuffer(void* staging) override;
    void DestroyStagingBuffer(void* staging) override;

    void DrawIndexedInstancedIndirect(void* argsBuffer, uint32_t byteOffset) override;
    void* CreateIndexBuffer(const uint32_t* data, size_t indexCount) override;
    void BindIndexBuffer(void* indexBuffer) override;
    void DestroyIndexBuffer(void* indexBuffer) override;
    void SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topology) override;
    void* CreateByteAddressBuffer(size_t sizeBytes, bool cpuWrite, bool gpuWrite) override;
    void* CreateByteAddressBufferSRV(void* buffer) override;
    void* CreateByteAddressBufferUAV(void* buffer) override;
    void* CreateIndirectArgsBuffer(size_t sizeBytes, bool gpuWrite) override;
    void* CreateIndirectArgsBufferUAV(void* buffer) override;
    void* CreateRawIndirectArgsBuffer(size_t sizeBytes) override;
    void* CreateRawIndirectArgsBufferUAV(void* buffer) override;
    void CopyStructureCount(void* destBuffer, uint32_t destByteOffset, void* srcUAV) override;
    void UpdateByteAddressBuffer(void* buffer, const void* data, size_t sizeBytes) override;
    void UnbindComputeResources() override;

private:

};


#endif //DIRECTX11_DIRECTX11RENDERER_H