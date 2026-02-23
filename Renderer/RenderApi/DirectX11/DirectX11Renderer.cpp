//
// Created by Marvin on 28/01/2026.
//

#include "DirectX11Renderer.h"

#include <d3dcompiler.h>

#include "DirectX11Mesh.h"
#include "DirectX11Texture.h"
#include "DirectX11TextureArray.h"

#include "Renderer/Shaders/ShaderProgram.h"
#include "Resources/Mesh.h"
#include "Resources/Texture.h"
#include "Resources/TextureData.h"
#include "Resources/TextureArray.h"
#include "Renderer/RenderTarget.h"

bool DirectX11Renderer::Initialize(void* windowHandle, int width, int height)
{
    this->windowHandle = windowHandle;
    this->windowWidth = width;
    this->windowHeight = height;

    // Create device, context, swap chain
    if (!CreateDeviceAndSwapChain(windowHandle, width, height)) {
        return false;
    }

    // Create render targets
    if (!CreateRenderTargets()) {
        return false;
    }

    // Create depth/stencil buffer
    if (!CreateDepthStencilBuffer(width, height)) {
        return false;
    }

    // Create all states
    if (!CreateRasterizerStates()) return false;
    if (!CreateDepthStencilStates()) return false;
    if (!CreateBlendStates()) return false;
    if (!CreateSamplerStates()) return false;

    // Set default viewport
    SetViewport(0, 0, width, height);

    // Set default render target
    SetDefaultRenderTarget();

    return true;
}

bool DirectX11Renderer::CreateRasterizerStates() {
    HRESULT hr;

    // SOLID (Default)
    D3D11_RASTERIZER_DESC solidDesc = {};
    solidDesc.FillMode = D3D11_FILL_SOLID;
    solidDesc.CullMode = D3D11_CULL_BACK;
    solidDesc.FrontCounterClockwise = FALSE;
    solidDesc.DepthBias = 0;
    solidDesc.DepthBiasClamp = 0.0f;
    solidDesc.SlopeScaledDepthBias = 0.0f;
    solidDesc.DepthClipEnable = TRUE;
    solidDesc.ScissorEnable = FALSE;
    solidDesc.MultisampleEnable = FALSE;
    solidDesc.AntialiasedLineEnable = FALSE;

    hr = device->CreateRasterizerState(&solidDesc, &rasterizerStates[RasterizerMode::Solid]);
    if (FAILED(hr)) return false;

    // WIREFRAME
    D3D11_RASTERIZER_DESC wireframeDesc = solidDesc;
    wireframeDesc.FillMode = D3D11_FILL_WIREFRAME;

    hr = device->CreateRasterizerState(&wireframeDesc, &rasterizerStates[RasterizerMode::Wireframe]);
    if (FAILED(hr)) return false;

    // NO CULL
    D3D11_RASTERIZER_DESC noCullDesc = solidDesc;
    noCullDesc.CullMode = D3D11_CULL_NONE;

    hr = device->CreateRasterizerState(&noCullDesc, &rasterizerStates[RasterizerMode::NoCull]);
    if (FAILED(hr)) return false;

    // CULL FRONT
    D3D11_RASTERIZER_DESC cullFrontDesc = solidDesc;
    cullFrontDesc.CullMode = D3D11_CULL_FRONT;

    hr = device->CreateRasterizerState(&cullFrontDesc, &rasterizerStates[RasterizerMode::CullFront]);
    if (FAILED(hr)) return false;

    // CULL BACK (same as solid, for completeness)
    rasterizerStates[RasterizerMode::CullBack] = rasterizerStates[RasterizerMode::Solid];

    return true;
}

bool DirectX11Renderer::CreateBlendStates() {
    HRESULT hr;

    // NO BLEND (Opaque)
    D3D11_BLEND_DESC noBlendDesc = {};
    noBlendDesc.AlphaToCoverageEnable = FALSE;
    noBlendDesc.IndependentBlendEnable = FALSE;
    noBlendDesc.RenderTarget[0].BlendEnable = FALSE;
    noBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = device->CreateBlendState(&noBlendDesc, &blendStates[BlendMode::None]);
    if (FAILED(hr)) return false;

    // ALPHA BLEND (Standard transparency)
    D3D11_BLEND_DESC alphaBlendDesc = {};
    alphaBlendDesc.AlphaToCoverageEnable = FALSE;
    alphaBlendDesc.IndependentBlendEnable = FALSE;
    alphaBlendDesc.RenderTarget[0].BlendEnable = TRUE;
    alphaBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    alphaBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    alphaBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    alphaBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    alphaBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    alphaBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    alphaBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = device->CreateBlendState(&alphaBlendDesc, &blendStates[BlendMode::Alpha]);
    if (FAILED(hr)) return false;

    // ADDITIVE BLEND (Glowing effects)
    D3D11_BLEND_DESC additiveBlendDesc = {};
    additiveBlendDesc.AlphaToCoverageEnable = FALSE;
    additiveBlendDesc.IndependentBlendEnable = FALSE;
    additiveBlendDesc.RenderTarget[0].BlendEnable = TRUE;
    additiveBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    additiveBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    additiveBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    additiveBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    additiveBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    additiveBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    additiveBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = device->CreateBlendState(&additiveBlendDesc, &blendStates[BlendMode::Additive]);
    if (FAILED(hr)) return false;

    // MULTIPLY BLEND
    D3D11_BLEND_DESC multiplyBlendDesc = {};
    multiplyBlendDesc.AlphaToCoverageEnable = FALSE;
    multiplyBlendDesc.IndependentBlendEnable = FALSE;
    multiplyBlendDesc.RenderTarget[0].BlendEnable = TRUE;
    multiplyBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_COLOR;
    multiplyBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
    multiplyBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    multiplyBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    multiplyBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    multiplyBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    multiplyBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = device->CreateBlendState(&multiplyBlendDesc, &blendStates[BlendMode::Multiply]);
    if (FAILED(hr)) return false;

    // PREMULTIPLIED ALPHA
    D3D11_BLEND_DESC premultipliedDesc = {};
    premultipliedDesc.AlphaToCoverageEnable = FALSE;
    premultipliedDesc.IndependentBlendEnable = FALSE;
    premultipliedDesc.RenderTarget[0].BlendEnable = TRUE;
    premultipliedDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    premultipliedDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    premultipliedDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    premultipliedDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    premultipliedDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    premultipliedDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    premultipliedDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = device->CreateBlendState(&premultipliedDesc, &blendStates[BlendMode::Premultiplied]);
    if (FAILED(hr)) return false;

    return true;
}

bool DirectX11Renderer::CreateDepthStencilStates() {
    HRESULT hr;

    // DEPTH ENABLED (Default)
    D3D11_DEPTH_STENCIL_DESC depthEnabledDesc = {};
    depthEnabledDesc.DepthEnable = TRUE;
    depthEnabledDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthEnabledDesc.DepthFunc = D3D11_COMPARISON_LESS;
    depthEnabledDesc.StencilEnable = FALSE;

    hr = device->CreateDepthStencilState(&depthEnabledDesc, &depthStencilStates[DepthMode::Less]);
    if (FAILED(hr)) return false;

    // DEPTH DISABLED
    D3D11_DEPTH_STENCIL_DESC depthDisabledDesc = {};
    depthDisabledDesc.DepthEnable = FALSE;
    depthDisabledDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depthDisabledDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    depthDisabledDesc.StencilEnable = FALSE;

    hr = device->CreateDepthStencilState(&depthDisabledDesc, &depthStencilStates[DepthMode::None]);
    if (FAILED(hr)) return false;

    // LESS EQUAL
    D3D11_DEPTH_STENCIL_DESC lessEqualDesc = depthEnabledDesc;
    lessEqualDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

    hr = device->CreateDepthStencilState(&lessEqualDesc, &depthStencilStates[DepthMode::LessEqual]);
    if (FAILED(hr)) return false;

    // GREATER
    D3D11_DEPTH_STENCIL_DESC greaterDesc = depthEnabledDesc;
    greaterDesc.DepthFunc = D3D11_COMPARISON_GREATER;

    hr = device->CreateDepthStencilState(&greaterDesc, &depthStencilStates[DepthMode::Greater]);
    if (FAILED(hr)) return false;

    // GREATER EQUAL
    D3D11_DEPTH_STENCIL_DESC greaterEqualDesc = depthEnabledDesc;
    greaterEqualDesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;

    hr = device->CreateDepthStencilState(&greaterEqualDesc, &depthStencilStates[DepthMode::GreaterEqual]);
    if (FAILED(hr)) return false;

    // EQUAL
    D3D11_DEPTH_STENCIL_DESC equalDesc = depthEnabledDesc;
    equalDesc.DepthFunc = D3D11_COMPARISON_EQUAL;

    hr = device->CreateDepthStencilState(&equalDesc, &depthStencilStates[DepthMode::Equal]);
    if (FAILED(hr)) return false;

    // NOT EQUAL
    D3D11_DEPTH_STENCIL_DESC notEqualDesc = depthEnabledDesc;
    notEqualDesc.DepthFunc = D3D11_COMPARISON_NOT_EQUAL;

    hr = device->CreateDepthStencilState(&notEqualDesc, &depthStencilStates[DepthMode::NotEqual]);
    if (FAILED(hr)) return false;

    // ALWAYS
    D3D11_DEPTH_STENCIL_DESC alwaysDesc = depthEnabledDesc;
    alwaysDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;

    hr = device->CreateDepthStencilState(&alwaysDesc, &depthStencilStates[DepthMode::Always]);
    if (FAILED(hr)) return false;

    // READ-ONLY (test depth but don't write - for occlusion queries)
    D3D11_DEPTH_STENCIL_DESC readOnlyDesc = depthEnabledDesc;
    readOnlyDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;  // Don't write to depth buffer
    readOnlyDesc.DepthFunc = D3D11_COMPARISON_LESS;

    hr = device->CreateDepthStencilState(&readOnlyDesc, &depthStencilStates[DepthMode::ReadOnly]);
    if (FAILED(hr)) return false;

    return true;
}

bool DirectX11Renderer::CreateSamplerStates() {
    HRESULT hr;

    // POINT (Pixelated - good for voxels)
    D3D11_SAMPLER_DESC pointSamplerDesc = {};
    pointSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    pointSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    pointSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    pointSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    pointSamplerDesc.MipLODBias = 0.0f;
    pointSamplerDesc.MaxAnisotropy = 16;
    pointSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    pointSamplerDesc.MinLOD = 0.0f;
    pointSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = device->CreateSamplerState(&pointSamplerDesc, &samplerStates[TextureFilter::Point]);
    if (FAILED(hr)) return false;

    // LINEAR (Smooth)
    D3D11_SAMPLER_DESC linearSamplerDesc = {};
    linearSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    linearSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    linearSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    linearSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    linearSamplerDesc.MipLODBias = 0.0f;
    linearSamplerDesc.MaxAnisotropy = 1;
    linearSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    linearSamplerDesc.MinLOD = 0.0f;
    linearSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = device->CreateSamplerState(&linearSamplerDesc, &samplerStates[TextureFilter::Linear]);
    if (FAILED(hr)) return false;

    // ANISOTROPIC (High quality)
    D3D11_SAMPLER_DESC anisotropicSamplerDesc = {};
    anisotropicSamplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    anisotropicSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    anisotropicSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    anisotropicSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    anisotropicSamplerDesc.MipLODBias = 0.0f;
    anisotropicSamplerDesc.MaxAnisotropy = 16;
    anisotropicSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    anisotropicSamplerDesc.MinLOD = 0.0f;
    anisotropicSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = device->CreateSamplerState(&anisotropicSamplerDesc, &samplerStates[TextureFilter::Anisotropic]);
    if (FAILED(hr)) return false;

    return true;
}

void DirectX11Renderer::SetRasterizerMode(RasterizerMode mode) {
    if (currentRasterizerMode == mode) return;

    auto it = rasterizerStates.find(mode);
    if (it != rasterizerStates.end()) {
        context->RSSetState(it->second.Get());
        currentRasterizerMode = mode;
    }
}

void DirectX11Renderer::SetBlendMode(BlendMode mode) {
    if (currentBlendMode == mode) return;

    auto it = blendStates.find(mode);
    if (it != blendStates.end()) {
        float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        context->OMSetBlendState(it->second.Get(), blendFactor, 0xFFFFFFFF);
        currentBlendMode = mode;
    }
}

void DirectX11Renderer::SetDepthMode(DepthMode mode) {
    if (currentDepthMode == mode) return;

    auto it = depthStencilStates.find(mode);
    if (it != depthStencilStates.end()) {
        context->OMSetDepthStencilState(it->second.Get(), 0);
        currentDepthMode = mode;
    }
}

void DirectX11Renderer::SetDepthTestEnabled(bool enabled) {
    if (depthTestEnabled == enabled) return;

    depthTestEnabled = enabled;
    SetDepthMode(enabled ? DepthMode::Less : DepthMode::None);
}

void DirectX11Renderer::SetDepthWriteEnabled(bool enabled) {
    if (depthWriteEnabled == enabled) return;

    depthWriteEnabled = enabled;

    // Would need to create states with different write masks
    // For now, simplified
}
bool DirectX11Renderer::CreateDeviceAndSwapChain(
    void* windowHandle, int width, int height)
{
    HWND hwnd = static_cast<HWND>(windowHandle);

    UINT createFlags = 0;
#ifdef _DEBUG
    createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // Create DXGI Factory
    ComPtr<IDXGIFactory4> factory4;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory4));
    if (FAILED(hr))
        return false;

    // Choose hardware adapter
    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; factory4->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        // Skip software adapters
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        break; // First real GPU
    }

    // Create D3D11 Device
    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    D3D_FEATURE_LEVEL featureLevel;

    hr = D3D11CreateDevice(
        adapter.Get(),
        adapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createFlags,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &device,
        &featureLevel,
        &context
    );

    if (FAILED(hr))
        return false;

    // Create Swap Chain (FLIP MODEL)
    ComPtr<IDXGIFactory2> factory2;
    hr = factory4.As(&factory2); // Factory4 inherits from Factory2
    if (FAILED(hr))
        return false;

    DXGI_SWAP_CHAIN_DESC1 swapDesc = {};
    swapDesc.Width = width;    // 0 = auto from window
    swapDesc.Height = height;  // 0 = auto from window
    swapDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapDesc.Stereo = FALSE;
    swapDesc.SampleDesc.Count = 1;              // MSAA must be off for flip model
    swapDesc.SampleDesc.Quality = 0;
    swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapDesc.BufferCount = 2;                   // 2 or 3 for flip model
    swapDesc.Scaling = DXGI_SCALING_STRETCH;
    swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    swapDesc.Flags = 0; // we’ll talk about tearing below

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsDesc = {};
    fsDesc.Windowed = TRUE;

    ComPtr<IDXGISwapChain1> swapChain1;
    hr = factory2->CreateSwapChainForHwnd(
        device.Get(),
        hwnd,
        &swapDesc,
        &fsDesc,
        nullptr,
        &swapChain1
    );

    if (FAILED(hr))
        return false;

    // Disable Alt+Enter fullscreen toggle
    //factory4->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

    // Store final swap chain as IDXGISwapChain3 if available
    swapChain1.As(&swapChain);

    return true;
}

bool DirectX11Renderer::CreateRenderTargets() {
    // Get back buffer from swap chain
    ComPtr<ID3D11Texture2D> backBuffer;
    HRESULT hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
        (void**)&backBuffer);

    if (FAILED(hr)) {
        return false;
    }

    // Create render target view
    hr = device->CreateRenderTargetView(backBuffer.Get(), nullptr,
        &backBufferRTV);

    if (FAILED(hr)) {
        return false;
    }

    return true;
}

bool DirectX11Renderer::CreateDepthStencilBuffer(int width, int height) {
    // Depth buffer texture description
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.MipLevels = 1;  // Depth buffers don't use mipmaps
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 24-bit depth, 8-bit stencil
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthDesc.MiscFlags = 0;

    HRESULT hr = device->CreateTexture2D(&depthDesc, nullptr,
        &depthStencilBuffer);

    if (FAILED(hr)) {
        return false;
    }

    // Create depth stencil view
    hr = device->CreateDepthStencilView(depthStencilBuffer.Get(),
        nullptr, &depthStencilView);

    if (FAILED(hr)) {
        return false;
    }

    return true;
}

void DirectX11Renderer::BeginFrame() {
    // Bind back buffer + depth buffer every frame
    context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthStencilView.Get());

    // Set viewport every frame (safe & common practice)
    context->RSSetViewports(1, &viewport);
}

void DirectX11Renderer::Clear(float r, float g, float b, float a) {
    float clearColor[4] = { r, g, b, a };
    if (currentRenderTarget) {
        ID3D11RenderTargetView* rtv = static_cast<ID3D11RenderTargetView*>(currentRenderTarget->GetColorRTV());
        context->ClearRenderTargetView(rtv, clearColor);
    } else {
        context->ClearRenderTargetView(backBufferRTV.Get(), clearColor);
    }
}

void DirectX11Renderer::ClearDepth() {
    if (currentRenderTarget && currentRenderTarget->HasDepth()) {
        ID3D11DepthStencilView* dsv = static_cast<ID3D11DepthStencilView*>(currentRenderTarget->GetDepthDSV());
        context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    } else if (!currentRenderTarget) {
        context->ClearDepthStencilView(depthStencilView.Get(),
            D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    }
}

void DirectX11Renderer::EndFrame() {
    // Nothing specific needed
}

void DirectX11Renderer::Present(bool vsync) {
    // Present with VSync (1)
    // Use 0 for no VSync (uncapped framerate)
    swapChain->Present(vsync, 0);
}

bool DirectX11Renderer::CreateMesh(Mesh* mesh) {
    if (!mesh) return false;

    // Create DirectX11-specific mesh
    DirectX11Mesh* d3dMesh = new DirectX11Mesh();

    bool success = d3dMesh->CreateBuffers(
         device.Get(),
         mesh->GetRawVertexData(),
         static_cast<UINT>(mesh->GetVertexCount()),
         static_cast<UINT>(mesh->GetVertexStride()),
         mesh->GetRawIndexData(),
         static_cast<UINT>(mesh->GetIndexCount())
     );

    if (!success) {
        delete d3dMesh;
        return false;
    }

    // Transfer topology from high-level Mesh
    d3dMesh->SetTopology(mesh->GetTopology());

    // Store DirectX11Mesh in high-level Mesh's GPU handle
    mesh->SetGPUHandle(d3dMesh);
    mesh->ClearDirty();

    return true;
}

bool DirectX11Renderer::CreateMeshInputLayout(Mesh* mesh, Shader* vertexShader) {
    if (!mesh || !vertexShader) return false;
    if (vertexShader->GetType() != ShaderType::Vertex) return false;

    DirectX11Mesh* d3dMesh = static_cast<DirectX11Mesh*>(mesh->GetGPUHandle());
    if (!d3dMesh) return false;

    const void* bytecode = vertexShader->GetBytecode();
    size_t bytecodeSize = vertexShader->GetBytecodeSize();

    if (!bytecode || bytecodeSize == 0) {
        std::cerr << "Vertex shader has no bytecode for input layout creation!" << std::endl;
        return false;
    }

    // Get layout description from the mesh (virtual - each mesh type provides its own)
    size_t layoutCount = 0;
    const VertexLayoutElement* layoutDesc = mesh->GetInputLayoutDesc(layoutCount);

    if (!layoutDesc || layoutCount == 0) {
        std::cerr << "Mesh has no input layout description!" << std::endl;
        return false;
    }

    // Convert VertexLayoutElement to D3D11_INPUT_ELEMENT_DESC
    std::vector<D3D11_INPUT_ELEMENT_DESC> d3dLayout(layoutCount);
    for (size_t i = 0; i < layoutCount; i++) {
        d3dLayout[i] = {};
        d3dLayout[i].SemanticName = layoutDesc[i].semanticName;
        d3dLayout[i].SemanticIndex = layoutDesc[i].semanticIndex;
        d3dLayout[i].Format = layoutDesc[i].format;
        d3dLayout[i].InputSlot = 0;
        d3dLayout[i].AlignedByteOffset = layoutDesc[i].offset;
        d3dLayout[i].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        d3dLayout[i].InstanceDataStepRate = 0;
    }

    return d3dMesh->CreateInputLayoutFromDesc(
        device.Get(),
        d3dLayout.data(),
        static_cast<UINT>(layoutCount),
        bytecode,
        bytecodeSize
    );
}

bool DirectX11Renderer::CreateTexture(Texture* texture, const TextureData& data) {
    if (!texture) return false;

    // Create DirectX11-specific texture
    DirectX11Texture* d3dTexture = new DirectX11Texture();

    // Convert TextureFormat to DXGI_FORMAT
    DXGI_FORMAT dxgiFormat = ConvertToDXGIFormat(data.GetFormat());

    bool success = d3dTexture->Create(
        device.Get(),
        context.Get(),
        data.GetWidth(),
        data.GetHeight(),
        dxgiFormat,
        data.GetPixels(),
        data.GetChannels(),
        true  // Generate mipmaps
    );

    if (!success) {
        delete d3dTexture;
        return false;
    }

    // Store DirectX11Texture in high-level Texture's GPU handle
    texture->SetGPUHandle(d3dTexture);
    texture->SetDimensions(data.GetWidth(), data.GetHeight(), data.GetChannels());
    texture->SetFormat(data.GetFormat());

    return true;
}

bool DirectX11Renderer::CompileShader(Shader* shader, const std::string& source) {
    ID3DBlob* shaderBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    // Determine shader profile
    const char* profile = nullptr;
    switch (shader->GetType()) {
        case ShaderType::Vertex:   profile = "vs_5_0"; break;
        case ShaderType::Pixel:    profile = "ps_5_0"; break;
        case ShaderType::Geometry: profile = "gs_5_0"; break;
        case ShaderType::Compute:  profile = "cs_5_0"; break;
    }

    // Compile shader
    HRESULT hr = D3DCompile(
        source.c_str(),
        source.size(),
        nullptr,                     // Optional source name
        nullptr,                     // Optional defines
        D3D_COMPILE_STANDARD_FILE_INCLUDE, // Include handler
        "main",                      // Entry point
        profile,
        D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS,
        0,
        &shaderBlob,
        &errorBlob
    );

    if (FAILED(hr)) {
        if (errorBlob) {
            const char* errorMsg = (const char*)errorBlob->GetBufferPointer();
            // Log error message
            std::cerr << "Shader compilation error: " << errorMsg << std::endl;
            errorBlob->Release();
        }
        return false;
    }

    // Create DirectX shader object
    void* shaderHandle = nullptr;

    switch (shader->GetType()) {
        case ShaderType::Vertex: {
            ID3D11VertexShader* vs = nullptr;
            hr = device->CreateVertexShader(
                shaderBlob->GetBufferPointer(),
                shaderBlob->GetBufferSize(),
                nullptr,
                &vs
            );
            shaderHandle = vs;

            // Store bytecode for input layout creation
            shader->SetBytecode(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize());
            break;
        }
        case ShaderType::Pixel: {
            ID3D11PixelShader* ps = nullptr;
            hr = device->CreatePixelShader(
                shaderBlob->GetBufferPointer(),
                shaderBlob->GetBufferSize(),
                nullptr,
                &ps
            );
            shaderHandle = ps;
            break;
        }
    case ShaderType::Geometry: {
        ID3D11GeometryShader* gs = nullptr;
        hr = device->CreateGeometryShader(
            shaderBlob->GetBufferPointer(),
            shaderBlob->GetBufferSize(),
            nullptr,
            &gs
        );
        shaderHandle = gs;
        break;
    }
    case ShaderType::Compute: {
        ID3D11ComputeShader* cs = nullptr;
        hr = device->CreateComputeShader(
            shaderBlob->GetBufferPointer(),
            shaderBlob->GetBufferSize(),
            nullptr,
            &cs
        );
        shaderHandle = cs;
        break;
    }
    }

    shaderBlob->Release();

    if (FAILED(hr)) {
        return false;
    }

    // Store handle in shader
    shader->SetHandle(shaderHandle);

    return true;
}

void DirectX11Renderer::BindShader(ShaderProgram* program) {
    if (!program || !program->IsValid()) {
        return;
    }

    currentShader = program;

    // Bind vertex shader
    Shader* vs = program->GetVertexShader();
    if (vs) {
        context->VSSetShader(
            static_cast<ID3D11VertexShader*>(vs->GetHandle()),
            nullptr,
            0
        );
    }

    // Bind pixel shader
    Shader* ps = program->GetPixelShader();
    if (ps) {
        context->PSSetShader(
            static_cast<ID3D11PixelShader*>(ps->GetHandle()),
            nullptr,
            0
        );
    }

    // Bind geometry shader (if present)
    Shader* gs = program->GetGeometryShader();
    if (gs) {
        context->GSSetShader(
            static_cast<ID3D11GeometryShader*>(gs->GetHandle()),
            nullptr,
            0
        );
    } else {
        // Unbind geometry shader if not needed
        context->GSSetShader(nullptr, nullptr, 0);
    }
}

void DirectX11Renderer::Draw(Mesh* mesh) {
    if (!mesh) return;

    DirectX11Mesh* d3dMesh = static_cast<DirectX11Mesh*>(mesh->GetGPUHandle());
    if (!d3dMesh) return;

    // Bind buffers, input layout, and topology
    d3dMesh->Bind(context.Get());

    // Draw indexed, use Gpu count because CPU is cleared after Render.
    context->DrawIndexed(d3dMesh->GetIndexCount(), 0, 0);
}

void DirectX11Renderer::UnbindShader()
{
    context->VSSetShader(nullptr, nullptr, 0);
    context->PSSetShader(nullptr, nullptr, 0);
    context->GSSetShader(nullptr, nullptr, 0);
    currentShader = nullptr;
}

DirectX11Renderer::DirectX11Renderer()
    : device(nullptr)
    , context(nullptr)
    , swapChain(nullptr)
    , backBufferRTV(nullptr)
    , depthStencilView(nullptr)
    , depthStencilBuffer(nullptr)
    , windowWidth(0)
    , windowHeight(0)
    , windowHandle(nullptr)
    , currentRasterizerMode(RasterizerMode::Solid)
    , currentDepthMode(DepthMode::Less)
    , depthTestEnabled(true)
    , depthWriteEnabled(true)
    , currentBlendMode(BlendMode::None)
    , currentTextureFilter(TextureFilter::Point)
    , currentShader(nullptr)
    , currentRenderTarget(nullptr)
{
    viewport = {};
}

DirectX11Renderer::~DirectX11Renderer()
{
    DirectX11Renderer::Shutdown();
}

void DirectX11Renderer::Shutdown()
{
    CleanupDeviceResources();
}

void DirectX11Renderer::CleanupDeviceResources()
{
    if (context) {
        context->ClearState();
    }

    rasterizerStates.clear();
    depthStencilStates.clear();
    blendStates.clear();
    samplerStates.clear();

    depthStencilView.Reset();
    depthStencilBuffer.Reset();
    backBufferRTV.Reset();
    swapChain.Reset();
    context.Reset();
    device.Reset();
}

void DirectX11Renderer::SetViewport(int x, int y, int width, int height)
{
    viewport.TopLeftX = static_cast<float>(x);
    viewport.TopLeftY = static_cast<float>(y);
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    context->RSSetViewports(1, &viewport);
}

void DirectX11Renderer::ResizeBuffers(int width, int height)
{
    if (width == 0 || height == 0) return;

    windowWidth = width;
    windowHeight = height;

    // Release existing views
    context->OMSetRenderTargets(0, nullptr, nullptr);
    backBufferRTV.Reset();
    depthStencilView.Reset();
    depthStencilBuffer.Reset();

    // Resize swap chain
    HRESULT hr = swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr)) return;

    // Recreate render targets
    CreateRenderTargets();
    CreateDepthStencilBuffer(width, height);

    // Update viewport
    SetViewport(0, 0, width, height);

    // Rebind render target
    SetDefaultRenderTarget();
}

RenderTarget* DirectX11Renderer::CreateRenderTarget(int width, int height, bool withDepth)
{
    RenderTarget* rt = new RenderTarget("RenderTarget", width, height, withDepth);

    // Create HDR color texture (RGBA16F for HDR rendering)
    DirectX11Texture* colorTex = new DirectX11Texture();
    if (!colorTex->CreateRenderTarget(device.Get(), width, height, DXGI_FORMAT_R16G16B16A16_FLOAT)) {
        delete colorTex;
        delete rt;
        return nullptr;
    }

    // Create high-level Texture wrapper
    Texture* colorTexture = new Texture("RT_Color");
    colorTexture->SetDimensions(width, height, 4);
    colorTexture->SetFormat(TextureFormat::RGBA16F);
    colorTexture->SetGPUHandle(colorTex);

    rt->SetColorTexture(colorTexture);
    rt->SetColorRTV(colorTex->GetRTV());

    // Create depth buffer if requested
    if (withDepth) {
        // Create depth stencil texture
        D3D11_TEXTURE2D_DESC depthDesc = {};
        depthDesc.Width = width;
        depthDesc.Height = height;
        depthDesc.MipLevels = 1;
        depthDesc.ArraySize = 1;
        depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthDesc.SampleDesc.Count = 1;
        depthDesc.SampleDesc.Quality = 0;
        depthDesc.Usage = D3D11_USAGE_DEFAULT;
        depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        depthDesc.CPUAccessFlags = 0;
        depthDesc.MiscFlags = 0;

        ComPtr<ID3D11Texture2D> depthBuffer;
        HRESULT hr = device->CreateTexture2D(&depthDesc, nullptr, &depthBuffer);
        if (FAILED(hr)) {
            delete rt;
            return nullptr;
        }

        // Create depth stencil view
        ComPtr<ID3D11DepthStencilView> dsv;
        hr = device->CreateDepthStencilView(depthBuffer.Get(), nullptr, &dsv);
        if (FAILED(hr)) {
            delete rt;
            return nullptr;
        }

        // Store the DSV - we need to prevent it from being released
        // Store raw pointer; the RenderTarget takes ownership
        ID3D11DepthStencilView* dsvPtr = dsv.Detach();
        rt->SetDepthDSV(dsvPtr);
    }

    return rt;
}

void* DirectX11Renderer::CreateConstantBuffer(size_t size) {
    // Ensure size is a multiple of 16 bytes (DirectX requirement)
    size_t alignedSize = (size + 15) & ~15;

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;           // CPU writes, GPU reads
    bufferDesc.ByteWidth = static_cast<UINT>(alignedSize);
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags = 0;
    bufferDesc.StructureByteStride = 0;

    ID3D11Buffer* constantBuffer = nullptr;
    HRESULT hr = device->CreateBuffer(&bufferDesc, nullptr, &constantBuffer);

    if (FAILED(hr)) {
        std::cerr << "Failed to create constant buffer!" << std::endl;
        return nullptr;
    }

    return constantBuffer;
}

void DirectX11Renderer::UpdateConstantBuffer(void* buffer, const void* data, size_t size) {
    if (!buffer || !data) {
        std::cerr << "Invalid constant buffer or data!" << std::endl;
        return;
    }

    ID3D11Buffer* constantBuffer = static_cast<ID3D11Buffer*>(buffer);

    // Map the buffer for writing
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

    if (FAILED(hr)) {
        std::cerr << "Failed to map constant buffer!" << std::endl;
        return;
    }

    // Copy data to buffer
    memcpy(mappedResource.pData, data, size);

    // Unmap the buffer
    context->Unmap(constantBuffer, 0);
}

void DirectX11Renderer::BindConstantBuffer(int slot, void* buffer, ShaderStage stage) {
    if (!buffer) {
        std::cerr << "Invalid constant buffer!" << std::endl;
        return;
    }

    ID3D11Buffer* constantBuffer = static_cast<ID3D11Buffer*>(buffer);

    // Bind to appropriate shader stages
    if (stage & ShaderStage::Vertex) {
        context->VSSetConstantBuffers(slot, 1, &constantBuffer);
    }

    if (stage & ShaderStage::Pixel) {
        context->PSSetConstantBuffers(slot, 1, &constantBuffer);
    }

    if (stage & ShaderStage::Geometry) {
        context->GSSetConstantBuffers(slot, 1, &constantBuffer);
    }

    if (stage & ShaderStage::Compute) {
        context->CSSetConstantBuffers(slot, 1, &constantBuffer);
    }
}

void DirectX11Renderer::DestroyConstantBuffer(void* buffer) {
    if (buffer) {
        ID3D11Buffer* constantBuffer = static_cast<ID3D11Buffer*>(buffer);
        constantBuffer->Release();
    }
}

void DirectX11Renderer::BindTexture(int slot, Texture* texture)
{
    if (!texture) return;

    DirectX11Texture* d3dTexture = static_cast<DirectX11Texture*>(texture->GetGPUHandle());
    if (!d3dTexture) return;

    ID3D11ShaderResourceView* srv = d3dTexture->GetSRV();
    context->PSSetShaderResources(slot, 1, &srv);

    // Bind sampler with current texture filter
    auto it = samplerStates.find(currentTextureFilter);
    if (it != samplerStates.end()) {
        ID3D11SamplerState* sampler = it->second.Get();
        context->PSSetSamplers(slot, 1, &sampler);
    }
}

void DirectX11Renderer::UnbindTexture(int slot)
{
    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->PSSetShaderResources(slot, 1, &nullSRV);
}

void DirectX11Renderer::SetTextureFilter(TextureFilter filter)
{
    currentTextureFilter = filter;
}

void DirectX11Renderer::DrawInstanced(Mesh* mesh, int instanceCount)
{
    if (!mesh || instanceCount <= 0) return;

    DirectX11Mesh* d3dMesh = static_cast<DirectX11Mesh*>(mesh->GetGPUHandle());
    if (!d3dMesh) return;

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    ID3D11Buffer* vb = d3dMesh->GetVertexBuffer();
    context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    context->IASetIndexBuffer(d3dMesh->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->DrawIndexedInstanced(d3dMesh->GetIndexCount(), instanceCount, 0, 0, 0);
}

void DirectX11Renderer::DrawIndexed(int indexCount)
{
    context->DrawIndexed(indexCount, 0, 0);
}

void DirectX11Renderer::SetRenderTarget(RenderTarget* target)
{
    if (!target) {
        SetDefaultRenderTarget();
        return;
    }

    currentRenderTarget = target;

    ID3D11RenderTargetView* rtv = static_cast<ID3D11RenderTargetView*>(target->GetColorRTV());
    ID3D11DepthStencilView* dsv = target->HasDepth()
        ? static_cast<ID3D11DepthStencilView*>(target->GetDepthDSV())
        : nullptr;

    context->OMSetRenderTargets(1, &rtv, dsv);

    // Set viewport to match render target size
    D3D11_VIEWPORT rtViewport = {};
    rtViewport.TopLeftX = 0.0f;
    rtViewport.TopLeftY = 0.0f;
    rtViewport.Width = static_cast<float>(target->GetWidth());
    rtViewport.Height = static_cast<float>(target->GetHeight());
    rtViewport.MinDepth = 0.0f;
    rtViewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &rtViewport);
}

void DirectX11Renderer::SetDefaultRenderTarget()
{
    currentRenderTarget = nullptr;

    context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthStencilView.Get());

    // Restore viewport to window size
    context->RSSetViewports(1, &viewport);
}

ID3D11Device* DirectX11Renderer::GetDevice() const
{
    return device.Get();
}

ID3D11DeviceContext* DirectX11Renderer::GetContext() const
{
    return context.Get();
}

int DirectX11Renderer::GetViewportWidth() const
{
    return static_cast<int>(viewport.Width);
}

int DirectX11Renderer::GetViewportHeight() const
{
    return static_cast<int>(viewport.Height);
}

DXGI_FORMAT DirectX11Renderer::ConvertToDXGIFormat(TextureFormat format)
{
    switch (format) {
        case TextureFormat::R8:       return DXGI_FORMAT_R8_UNORM;
        case TextureFormat::RG8:      return DXGI_FORMAT_R8G8_UNORM;
        case TextureFormat::RGB8:     return DXGI_FORMAT_R8G8B8A8_UNORM; // No RGB8 in DXGI, use RGBA8
        case TextureFormat::RGBA8:    return DXGI_FORMAT_R8G8B8A8_UNORM;
        case TextureFormat::R16F:     return DXGI_FORMAT_R16_FLOAT;
        case TextureFormat::RGBA16F:  return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case TextureFormat::RGBA32F:  return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case TextureFormat::Depth24Stencil8: return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case TextureFormat::Unknown:
        default:                      return DXGI_FORMAT_R8G8B8A8_UNORM;
    }
}

bool DirectX11Renderer::CreateTextureArray(TextureArray* textureArray, const std::vector<TextureData*>& textures)
{
    if (!textureArray || textures.empty()) {
        return false;
    }

    DirectX11TextureArray* d3dTexArray = new DirectX11TextureArray();

    bool success = d3dTexArray->Create(
        device.Get(),
        context.Get(),
        textures,
        true  // Generate mipmaps
    );

    if (!success) {
        delete d3dTexArray;
        return false;
    }

    // Store in high-level wrapper
    textureArray->SetGPUHandle(d3dTexArray);
    textureArray->SetDimensions(
        textures[0]->GetWidth(),
        textures[0]->GetHeight(),
        static_cast<int>(textures.size())
    );

    return true;
}

void DirectX11Renderer::BindTextureArray(int slot, TextureArray* textureArray)
{
    if (!textureArray) return;

    DirectX11TextureArray* d3dTexArray = static_cast<DirectX11TextureArray*>(textureArray->GetGPUHandle());
    if (!d3dTexArray) return;

    d3dTexArray->Bind(context.Get(), slot);

    // Bind sampler with current texture filter
    auto it = samplerStates.find(currentTextureFilter);
    if (it != samplerStates.end()) {
        ID3D11SamplerState* sampler = it->second.Get();
        context->PSSetSamplers(slot, 1, &sampler);
    }
}

// ========================================
// COMPUTE SHADER SUPPORT
// ========================================

void DirectX11Renderer::BindComputeShader(Shader* shader)
{
    if (!shader || shader->GetType() != ShaderType::Compute) {
        std::cerr << "Invalid compute shader!" << std::endl;
        return;
    }

    ID3D11ComputeShader* cs = static_cast<ID3D11ComputeShader*>(shader->GetHandle());
    context->CSSetShader(cs, nullptr, 0);
}

void DirectX11Renderer::UnbindComputeShader()
{
    context->CSSetShader(nullptr, nullptr, 0);
}

void DirectX11Renderer::Dispatch(int groupsX, int groupsY, int groupsZ)
{
    context->Dispatch(groupsX, groupsY, groupsZ);
}

// UAV resource wrapper struct - stored as void* but cast back when needed
struct UAVResource {
    ComPtr<ID3D11Texture2D> texture;
    ComPtr<ID3D11UnorderedAccessView> uav;
    ComPtr<ID3D11ShaderResourceView> srv;
    int width;
    int height;
};

void* DirectX11Renderer::CreateTexture2DUAV(int width, int height, int format)
{
    UAVResource* resource = new UAVResource();
    resource->width = width;
    resource->height = height;

    // Determine DXGI format based on input
    DXGI_FORMAT dxgiFormat;
    switch (format) {
        case 0: dxgiFormat = DXGI_FORMAT_R32_FLOAT; break;      // Single float (shadow map)
        case 1: dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM; break; // RGBA8
        case 2: dxgiFormat = DXGI_FORMAT_R16G16B16A16_FLOAT; break; // RGBA16F
        default: dxgiFormat = DXGI_FORMAT_R32_FLOAT; break;
    }

    // Create texture with UAV and SRV bindings
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = dxgiFormat;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;

    HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, &resource->texture);
    if (FAILED(hr)) {
        std::cerr << "Failed to create UAV texture!" << std::endl;
        delete resource;
        return nullptr;
    }

    // Create UAV
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = dxgiFormat;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;

    hr = device->CreateUnorderedAccessView(resource->texture.Get(), &uavDesc, &resource->uav);
    if (FAILED(hr)) {
        std::cerr << "Failed to create UAV!" << std::endl;
        delete resource;
        return nullptr;
    }

    // Create SRV for reading the UAV result as a texture
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = dxgiFormat;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    hr = device->CreateShaderResourceView(resource->texture.Get(), &srvDesc, &resource->srv);
    if (FAILED(hr)) {
        std::cerr << "Failed to create SRV for UAV!" << std::endl;
        delete resource;
        return nullptr;
    }

    return resource;
}

void DirectX11Renderer::BindUAV(int slot, void* uav)
{
    if (!uav) return;

    UAVResource* resource = static_cast<UAVResource*>(uav);
    ID3D11UnorderedAccessView* uavView = resource->uav.Get();
    context->CSSetUnorderedAccessViews(slot, 1, &uavView, nullptr);
}

void DirectX11Renderer::UnbindUAV(int slot)
{
    ID3D11UnorderedAccessView* nullUAV = nullptr;
    context->CSSetUnorderedAccessViews(slot, 1, &nullUAV, nullptr);
}

void* DirectX11Renderer::GetUAVTexture(void* uav)
{
    if (!uav) return nullptr;
    UAVResource* resource = static_cast<UAVResource*>(uav);
    return resource->texture.Get();
}

void* DirectX11Renderer::GetUAVSRV(void* uav)
{
    if (!uav) return nullptr;
    UAVResource* resource = static_cast<UAVResource*>(uav);
    return resource->srv.Get();
}

void DirectX11Renderer::DestroyUAV(void* uav)
{
    if (uav) {
        UAVResource* resource = static_cast<UAVResource*>(uav);
        delete resource;
    }
}

// ========================================
// 3D TEXTURE SUPPORT
// ========================================

struct Texture3DResource {
    ComPtr<ID3D11Texture3D> texture;
    ComPtr<ID3D11ShaderResourceView> srv;
    int width;
    int height;
    int depth;
};

void* DirectX11Renderer::CreateTexture3D(int width, int height, int depth, const void* data)
{
    Texture3DResource* resource = new Texture3DResource();
    resource->width = width;
    resource->height = height;
    resource->depth = depth;

    // Create 3D texture (R8_UINT for voxel type storage)
    D3D11_TEXTURE3D_DESC texDesc = {};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.Depth = depth;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R8_UINT;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    D3D11_SUBRESOURCE_DATA* pInitData = nullptr;

    if (data) {
        initData.pSysMem = data;
        initData.SysMemPitch = width;           // Row pitch (bytes per row)
        initData.SysMemSlicePitch = width * height; // Slice pitch (bytes per depth slice)
        pInitData = &initData;
    }

    HRESULT hr = device->CreateTexture3D(&texDesc, pInitData, &resource->texture);
    if (FAILED(hr)) {
        std::cerr << "Failed to create 3D texture!" << std::endl;
        delete resource;
        return nullptr;
    }

    // Create SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8_UINT;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
    srvDesc.Texture3D.MipLevels = 1;
    srvDesc.Texture3D.MostDetailedMip = 0;

    hr = device->CreateShaderResourceView(resource->texture.Get(), &srvDesc, &resource->srv);
    if (FAILED(hr)) {
        std::cerr << "Failed to create SRV for 3D texture!" << std::endl;
        delete resource;
        return nullptr;
    }

    return resource;
}

void DirectX11Renderer::UpdateTexture3DRegion(void* texture, int x, int y, int z, int w, int h, int d, const void* data)
{
    if (!texture || !data) return;

    Texture3DResource* resource = static_cast<Texture3DResource*>(texture);

    D3D11_BOX box;
    box.left = x;
    box.top = y;
    box.front = z;
    box.right = x + w;
    box.bottom = y + h;
    box.back = z + d;

    // RowPitch = width of data region (bytes per row)
    // DepthPitch = width * height of data region (bytes per slice)
    context->UpdateSubresource(
        resource->texture.Get(),
        0,      // Subresource index
        &box,
        data,
        w,      // Row pitch
        w * h   // Depth pitch
    );
}

void DirectX11Renderer::BindTexture3DToCompute(int slot, void* texture)
{
    if (!texture) return;

    Texture3DResource* resource = static_cast<Texture3DResource*>(texture);
    ID3D11ShaderResourceView* srv = resource->srv.Get();
    context->CSSetShaderResources(slot, 1, &srv);
}

void DirectX11Renderer::DestroyTexture3D(void* texture)
{
    if (texture) {
        Texture3DResource* resource = static_cast<Texture3DResource*>(texture);
        delete resource;
    }
}

void DirectX11Renderer::BindTextureToCompute(int slot, Texture* texture)
{
    if (!texture) return;

    DirectX11Texture* d3dTexture = static_cast<DirectX11Texture*>(texture->GetGPUHandle());
    if (!d3dTexture) return;

    ID3D11ShaderResourceView* srv = d3dTexture->GetSRV();
    context->CSSetShaderResources(slot, 1, &srv);
}

void DirectX11Renderer::BindSamplerToCompute(int slot)
{
    auto it = samplerStates.find(currentTextureFilter);
    if (it != samplerStates.end()) {
        ID3D11SamplerState* sampler = it->second.Get();
        context->CSSetSamplers(slot, 1, &sampler);
    }
}

// ========================================
// READABLE DEPTH BUFFER SUPPORT
// ========================================

RenderTarget* DirectX11Renderer::CreateRenderTargetWithReadableDepth(int width, int height)
{
    RenderTarget* rt = new RenderTarget("RenderTargetWithDepth", width, height, true);

    // Create HDR color texture (same as regular render target)
    DirectX11Texture* colorTex = new DirectX11Texture();
    if (!colorTex->CreateRenderTarget(device.Get(), width, height, DXGI_FORMAT_R16G16B16A16_FLOAT)) {
        delete colorTex;
        delete rt;
        return nullptr;
    }

    Texture* colorTexture = new Texture("RT_Color");
    colorTexture->SetDimensions(width, height, 4);
    colorTexture->SetFormat(TextureFormat::RGBA16F);
    colorTexture->SetGPUHandle(colorTex);

    rt->SetColorTexture(colorTexture);
    rt->SetColorRTV(colorTex->GetRTV());

    // Create depth buffer with TYPELESS format so we can create both DSV and SRV
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_R32_TYPELESS;  // TYPELESS for dual views
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    depthDesc.CPUAccessFlags = 0;
    depthDesc.MiscFlags = 0;

    ComPtr<ID3D11Texture2D> depthBuffer;
    HRESULT hr = device->CreateTexture2D(&depthDesc, nullptr, &depthBuffer);
    if (FAILED(hr)) {
        delete rt;
        return nullptr;
    }

    // Create Depth Stencil View (for writing depth)
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    ComPtr<ID3D11DepthStencilView> dsv;
    hr = device->CreateDepthStencilView(depthBuffer.Get(), &dsvDesc, &dsv);
    if (FAILED(hr)) {
        delete rt;
        return nullptr;
    }

    // Create Shader Resource View (for reading depth)
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    ComPtr<ID3D11ShaderResourceView> depthSRV;
    hr = device->CreateShaderResourceView(depthBuffer.Get(), &srvDesc, &depthSRV);
    if (FAILED(hr)) {
        delete rt;
        return nullptr;
    }

    // Store the DSV
    ID3D11DepthStencilView* dsvPtr = dsv.Detach();
    rt->SetDepthDSV(dsvPtr);

    // Store the depth SRV in our map
    depthSRVs[rt] = depthSRV;

    return rt;
}

void* DirectX11Renderer::GetDepthSRV(RenderTarget* target)
{
    if (!target) return nullptr;

    auto it = depthSRVs.find(target);
    if (it != depthSRVs.end()) {
        return it->second.Get();
    }
    return nullptr;
}

void DirectX11Renderer::BindDepthToCompute(int slot, RenderTarget* target)
{
    if (!target) return;

    auto it = depthSRVs.find(target);
    if (it != depthSRVs.end()) {
        ID3D11ShaderResourceView* srv = it->second.Get();
        context->CSSetShaderResources(slot, 1, &srv);
    }
}

// ========================================
// OCCLUSION QUERIES
// ========================================

void* DirectX11Renderer::CreateOcclusionQuery()
{
    D3D11_QUERY_DESC queryDesc = {};
    queryDesc.Query = D3D11_QUERY_OCCLUSION;
    queryDesc.MiscFlags = 0;

    ID3D11Query* query = nullptr;
    HRESULT hr = device->CreateQuery(&queryDesc, &query);
    if (FAILED(hr)) {
        std::cerr << "Failed to create occlusion query!" << std::endl;
        return nullptr;
    }

    return query;
}

void DirectX11Renderer::DestroyOcclusionQuery(void* query)
{
    if (query) {
        ID3D11Query* d3dQuery = static_cast<ID3D11Query*>(query);
        d3dQuery->Release();
    }
}

void DirectX11Renderer::BeginOcclusionQuery(void* query)
{
    if (!query) return;
    ID3D11Query* d3dQuery = static_cast<ID3D11Query*>(query);
    context->Begin(d3dQuery);
}

void DirectX11Renderer::EndOcclusionQuery(void* query)
{
    if (!query) return;
    ID3D11Query* d3dQuery = static_cast<ID3D11Query*>(query);
    context->End(d3dQuery);
}

bool DirectX11Renderer::GetOcclusionQueryResult(void* query, uint64_t& pixelCount, bool wait)
{
    if (!query) {
        pixelCount = 0;
        return false;
    }

    ID3D11Query* d3dQuery = static_cast<ID3D11Query*>(query);
    UINT64 result = 0;

    HRESULT hr;
    if (wait) {
        // Block until result is available
        while ((hr = context->GetData(d3dQuery, &result, sizeof(result), 0)) == S_FALSE) {
            // Spin-wait (could add Sleep(0) for less CPU usage)
        }
    } else {
        // Non-blocking check
        hr = context->GetData(d3dQuery, &result, sizeof(result), D3D11_ASYNC_GETDATA_DONOTFLUSH);
        if (hr == S_FALSE) {
            return false; // Result not ready
        }
    }

    if (FAILED(hr)) {
        pixelCount = 0;
        return false;
    }

    pixelCount = result;
    return true;
}

// ========================================
// STRUCTURED BUFFER SUPPORT
// ========================================

struct StructuredBufferResource {
    ComPtr<ID3D11Buffer> buffer;
    ComPtr<ID3D11ShaderResourceView> srv;
    ComPtr<ID3D11UnorderedAccessView> uav;
    size_t elementSize;
    size_t elementCount;
    bool isAppendConsume;
};

void* DirectX11Renderer::CreateStructuredBuffer(size_t elemSize, size_t count, bool cpuWrite, bool gpuWrite, const void* initialData)
{
    StructuredBufferResource* resource = new StructuredBufferResource();
    resource->elementSize = elemSize;
    resource->elementCount = count;
    resource->isAppendConsume = false;

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = static_cast<UINT>(elemSize * count);
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufferDesc.StructureByteStride = static_cast<UINT>(elemSize);

    if (cpuWrite && !gpuWrite) {
        // CPU writes, GPU reads only
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    } else if (gpuWrite && !cpuWrite) {
        // GPU writes (UAV), no CPU access
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
        bufferDesc.CPUAccessFlags = 0;
    } else if (!cpuWrite && !gpuWrite) {
        // Immutable - no writes after creation (requires initial data)
        if (!initialData) {
            std::cerr << "Failed to create structured buffer: immutable buffer requires initialData!" << std::endl;
            delete resource;
            return nullptr;
        }
        bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
        bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        bufferDesc.CPUAccessFlags = 0;
    } else {
        // Both CPU and GPU write - not supported directly, use staging
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
        bufferDesc.CPUAccessFlags = 0;
    }

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = initialData;

    HRESULT hr = device->CreateBuffer(&bufferDesc, initialData ? &initData : nullptr, &resource->buffer);
    if (FAILED(hr)) {
        std::cerr << "Failed to create structured buffer! HRESULT: " << std::hex << hr << std::endl;
        delete resource;
        return nullptr;
    }

    return resource;
}

void* DirectX11Renderer::CreateStructuredBufferSRV(void* buffer)
{
    if (!buffer) return nullptr;

    StructuredBufferResource* resource = static_cast<StructuredBufferResource*>(buffer);

    if (resource->srv) {
        return resource->srv.Get();
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = static_cast<UINT>(resource->elementCount);

    HRESULT hr = device->CreateShaderResourceView(resource->buffer.Get(), &srvDesc, &resource->srv);
    if (FAILED(hr)) {
        std::cerr << "Failed to create structured buffer SRV!" << std::endl;
        return nullptr;
    }

    return resource->srv.Get();
}

void* DirectX11Renderer::CreateStructuredBufferUAV(void* buffer, bool appendConsume)
{
    if (!buffer) return nullptr;

    StructuredBufferResource* resource = static_cast<StructuredBufferResource*>(buffer);
    resource->isAppendConsume = appendConsume;

    if (resource->uav) {
        return resource->uav.Get();
    }

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = static_cast<UINT>(resource->elementCount);
    uavDesc.Buffer.Flags = appendConsume ? D3D11_BUFFER_UAV_FLAG_APPEND : 0;

    HRESULT hr = device->CreateUnorderedAccessView(resource->buffer.Get(), &uavDesc, &resource->uav);
    if (FAILED(hr)) {
        std::cerr << "Failed to create structured buffer UAV!" << std::endl;
        return nullptr;
    }

    return resource->uav.Get();
}

void DirectX11Renderer::UpdateStructuredBuffer(void* buffer, const void* data, size_t size)
{
    if (!buffer || !data) return;

    StructuredBufferResource* resource = static_cast<StructuredBufferResource*>(buffer);

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = context->Map(resource->buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(hr)) {
        std::cerr << "Failed to map structured buffer!" << std::endl;
        return;
    }

    memcpy(mappedResource.pData, data, size);
    context->Unmap(resource->buffer.Get(), 0);
}

void DirectX11Renderer::BindStructuredBufferSRV(int slot, void* srv, ShaderStage stage)
{
    ID3D11ShaderResourceView* srvView = static_cast<ID3D11ShaderResourceView*>(srv);
    // Allow null to unbind

    if (stage & ShaderStage::Vertex) {
        context->VSSetShaderResources(slot, 1, &srvView);
    }
    if (stage & ShaderStage::Pixel) {
        context->PSSetShaderResources(slot, 1, &srvView);
    }
    if (stage & ShaderStage::Geometry) {
        context->GSSetShaderResources(slot, 1, &srvView);
    }
    if (stage & ShaderStage::Compute) {
        context->CSSetShaderResources(slot, 1, &srvView);
    }
}

void DirectX11Renderer::BindStructuredBufferUAV(int slot, void* uav, uint32_t initialCount)
{
    if (!uav) return;

    ID3D11UnorderedAccessView* uavView = static_cast<ID3D11UnorderedAccessView*>(uav);
    context->CSSetUnorderedAccessViews(slot, 1, &uavView, &initialCount);
}

void DirectX11Renderer::UnbindStructuredBufferUAV(int slot)
{
    ID3D11UnorderedAccessView* nullUAV = nullptr;
    UINT initialCount = 0;
    context->CSSetUnorderedAccessViews(slot, 1, &nullUAV, &initialCount);
}

void DirectX11Renderer::DestroyStructuredBuffer(void* buffer)
{
    if (buffer) {
        StructuredBufferResource* resource = static_cast<StructuredBufferResource*>(buffer);
        delete resource;
    }
}

// ========================================
// STAGING BUFFER SUPPORT
// ========================================

void* DirectX11Renderer::CreateStagingBuffer(size_t size)
{
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = static_cast<UINT>(size);
    bufferDesc.Usage = D3D11_USAGE_STAGING;
    bufferDesc.BindFlags = 0;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    bufferDesc.MiscFlags = 0;
    bufferDesc.StructureByteStride = 0;

    ID3D11Buffer* stagingBuffer = nullptr;
    HRESULT hr = device->CreateBuffer(&bufferDesc, nullptr, &stagingBuffer);
    if (FAILED(hr)) {
        std::cerr << "Failed to create staging buffer!" << std::endl;
        return nullptr;
    }

    return stagingBuffer;
}

void DirectX11Renderer::CopyBufferToStaging(void* staging, void* gpuBuffer, size_t size)
{
    if (!staging || !gpuBuffer) return;

    ID3D11Buffer* stagingBuffer = static_cast<ID3D11Buffer*>(staging);
    StructuredBufferResource* resource = static_cast<StructuredBufferResource*>(gpuBuffer);

    // Use CopyResource to copy entire buffer, or CopySubresourceRegion for partial
    D3D11_BOX srcBox = {};
    srcBox.left = 0;
    srcBox.right = static_cast<UINT>(size);
    srcBox.top = 0;
    srcBox.bottom = 1;
    srcBox.front = 0;
    srcBox.back = 1;

    context->CopySubresourceRegion(stagingBuffer, 0, 0, 0, 0, resource->buffer.Get(), 0, &srcBox);
}

void DirectX11Renderer::CopyStructureCount(void* staging, void* uav)
{
    if (!staging || !uav) return;

    ID3D11Buffer* stagingBuffer = static_cast<ID3D11Buffer*>(staging);
    ID3D11UnorderedAccessView* uavView = static_cast<ID3D11UnorderedAccessView*>(uav);

    // CopyStructureCount copies the internal counter from an append/consume buffer
    context->CopyStructureCount(stagingBuffer, 0, uavView);
}

void* DirectX11Renderer::MapStagingBuffer(void* staging)
{
    if (!staging) return nullptr;

    ID3D11Buffer* stagingBuffer = static_cast<ID3D11Buffer*>(staging);

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = context->Map(stagingBuffer, 0, D3D11_MAP_READ, 0, &mappedResource);
    if (FAILED(hr)) {
        std::cerr << "Failed to map staging buffer!" << std::endl;
        return nullptr;
    }

    return mappedResource.pData;
}

void DirectX11Renderer::UnmapStagingBuffer(void* staging)
{
    if (!staging) return;

    ID3D11Buffer* stagingBuffer = static_cast<ID3D11Buffer*>(staging);
    context->Unmap(stagingBuffer, 0);
}

void DirectX11Renderer::DestroyStagingBuffer(void* staging)
{
    if (staging) {
        ID3D11Buffer* stagingBuffer = static_cast<ID3D11Buffer*>(staging);
        stagingBuffer->Release();
    }
}

// Draws using arguments stored in a GPU buffer.
// The buffer must contain a D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS structure:
//   uint IndexCountPerInstance
//   uint InstanceCount
//   uint StartIndexLocation
//   int  BaseVertexLocation
//   uint StartInstanceLocation
// Total: 20 bytes per draw call
void DirectX11Renderer::DrawIndexedInstancedIndirect(void* argsBuffer, uint32_t byteOffset)
{
    if (!argsBuffer) return;

    ID3D11Buffer* d3dBuffer = static_cast<ID3D11Buffer*>(argsBuffer);
    context->DrawIndexedInstancedIndirect(d3dBuffer, byteOffset);
}

void* DirectX11Renderer::CreateIndexBuffer(const uint32_t* data, size_t indexCount)
{
    if (!data || indexCount == 0) return nullptr;

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = static_cast<UINT>(indexCount * sizeof(uint32_t));
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = data;

    ID3D11Buffer* buffer = nullptr;
    HRESULT hr = device->CreateBuffer(&desc, &initData, &buffer);
    if (FAILED(hr)) {
        std::cerr << "Failed to create index buffer. HRESULT: " << std::hex << hr << std::endl;
        return nullptr;
    }

    return buffer;
}

void DirectX11Renderer::BindIndexBuffer(void* indexBuffer)
{
    if (!indexBuffer) return;
    ID3D11Buffer* d3dBuffer = static_cast<ID3D11Buffer*>(indexBuffer);
    context->IASetIndexBuffer(d3dBuffer, DXGI_FORMAT_R32_UINT, 0);
}

void DirectX11Renderer::DestroyIndexBuffer(void* indexBuffer)
{
    if (indexBuffer) {
        ID3D11Buffer* d3dBuffer = static_cast<ID3D11Buffer*>(indexBuffer);
        d3dBuffer->Release();
    }
}

void DirectX11Renderer::SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topology)
{
    context->IASetPrimitiveTopology(topology);
}

// Creates a raw buffer that can be accessed as ByteAddressBuffer in HLSL.
// Used for atomic counters and unstructured data.
void* DirectX11Renderer::CreateByteAddressBuffer(size_t sizeBytes, bool cpuWrite, bool gpuWrite)
{
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = static_cast<UINT>(sizeBytes);

    // gpuWrite (UAV) requires DEFAULT usage - DYNAMIC cannot have UAV
    if (gpuWrite) {
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.CPUAccessFlags = 0;  // Use UpdateSubresource for CPU writes
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    } else if (cpuWrite) {
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    } else {
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.CPUAccessFlags = 0;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    }

    // ByteAddressBuffer requires RAW_VIEWS flag
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
    desc.StructureByteStride = 0;  // Not a structured buffer

    ID3D11Buffer* buffer = nullptr;
    HRESULT hr = device->CreateBuffer(&desc, nullptr, &buffer);

    if (FAILED(hr)) {
        std::cerr << "Failed to create ByteAddressBuffer. HRESULT: " << std::hex << hr << std::endl;
        return nullptr;
    }

    return buffer;
}

// CreateByteAddressBufferSRV
// Creates a shader resource view for reading a ByteAddressBuffer.
void* DirectX11Renderer::CreateByteAddressBufferSRV(void* buffer)
{
    if (!buffer) return nullptr;

    ID3D11Buffer* d3dBuffer = static_cast<ID3D11Buffer*>(buffer);

    // Get buffer size
    D3D11_BUFFER_DESC bufferDesc;
    d3dBuffer->GetDesc(&bufferDesc);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;  // Raw buffer uses typeless
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
    srvDesc.BufferEx.FirstElement = 0;
    srvDesc.BufferEx.NumElements = bufferDesc.ByteWidth / 4;  // Number of 32-bit elements
    srvDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;

    ID3D11ShaderResourceView* srv = nullptr;
    HRESULT hr = device->CreateShaderResourceView(d3dBuffer, &srvDesc, &srv);

    if (FAILED(hr)) {
        std::cerr << "Failed to create ByteAddressBuffer SRV. HRESULT: " << std::hex << hr << std::endl;
        return nullptr;
    }

    return srv;
}

// CreateByteAddressBufferUAV
// Creates an unordered access view for read/write access to a ByteAddressBuffer.
// This is what you bind as RWByteAddressBuffer in HLSL.
void* DirectX11Renderer::CreateByteAddressBufferUAV(void* buffer)
{
    if (!buffer) return nullptr;

    ID3D11Buffer* d3dBuffer = static_cast<ID3D11Buffer*>(buffer);

    // Get buffer size
    D3D11_BUFFER_DESC bufferDesc;
    d3dBuffer->GetDesc(&bufferDesc);

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;  // Raw buffer uses typeless
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = bufferDesc.ByteWidth / 4;  // Number of 32-bit elements
    uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;

    ID3D11UnorderedAccessView* uav = nullptr;
    HRESULT hr = device->CreateUnorderedAccessView(d3dBuffer, &uavDesc, &uav);

    if (FAILED(hr)) {
        std::cerr << "Failed to create ByteAddressBuffer UAV. HRESULT: " << std::hex << hr << std::endl;
        return nullptr;
    }

    return uav;
}

// CreateIndirectArgsBuffer
// Creates a buffer suitable for DrawIndexedInstancedIndirect.
// Must have DRAWINDIRECT_ARGS misc flag.
void* DirectX11Renderer::CreateIndirectArgsBuffer(size_t sizeBytes, bool gpuWrite)
{
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = static_cast<UINT>(sizeBytes);
    desc.Usage = D3D11_USAGE_DEFAULT;  // GPU needs to write, so can't be DYNAMIC
    desc.CPUAccessFlags = 0;

    // Bind flags - needs UAV if GPU writes to it
    desc.BindFlags = 0;
    if (gpuWrite) {
        desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
    }

    // Critical: this flag enables use with DrawIndexedInstancedIndirect
    desc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
    desc.StructureByteStride = 0;

    // Initialize to zeros (prevents garbage draws on first frame)
    std::vector<uint8_t> zeros(sizeBytes, 0);
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = zeros.data();

    ID3D11Buffer* buffer = nullptr;
    HRESULT hr = device->CreateBuffer(&desc, &initData, &buffer);

    if (FAILED(hr)) {
        std::cerr << "Failed to create IndirectArgsBuffer. HRESULT: " << std::hex << hr << std::endl;
        return nullptr;
    }

    return buffer;
}

// CreateIndirectArgsBufferUAV
// If you need GPU to write draw args, you need a UAV.
// The buffer contains raw uint32s (not structured), so use R32_UINT format.
void* DirectX11Renderer::CreateIndirectArgsBufferUAV(void* buffer)
{
    if (!buffer) return nullptr;

    ID3D11Buffer* d3dBuffer = static_cast<ID3D11Buffer*>(buffer);

    // Get buffer size
    D3D11_BUFFER_DESC bufferDesc;
    d3dBuffer->GetDesc(&bufferDesc);

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_R32_UINT;  // Indirect args are uint32s
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = bufferDesc.ByteWidth / 4;
    uavDesc.Buffer.Flags = 0;  // Not raw, not append/consume

    ID3D11UnorderedAccessView* uav = nullptr;
    HRESULT hr = device->CreateUnorderedAccessView(d3dBuffer, &uavDesc, &uav);

    if (FAILED(hr)) {
        std::cerr << "Failed to create IndirectArgsBuffer UAV. HRESULT: " << std::hex << hr << std::endl;
        return nullptr;
    }

    return uav;
}

// CreateRawIndirectArgsBuffer
// Buffer with BOTH DRAWINDIRECT_ARGS and ALLOW_RAW_VIEWS flags.
// This enables RWByteAddressBuffer in compute + DrawIndexedInstancedIndirect on the same buffer.
void* DirectX11Renderer::CreateRawIndirectArgsBuffer(size_t sizeBytes)
{
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = static_cast<UINT>(sizeBytes);
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.CPUAccessFlags = 0;
    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    desc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS | D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
    desc.StructureByteStride = 0;

    std::vector<uint8_t> zeros(sizeBytes, 0);
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = zeros.data();

    ID3D11Buffer* buffer = nullptr;
    HRESULT hr = device->CreateBuffer(&desc, &initData, &buffer);

    if (FAILED(hr)) {
        std::cerr << "Failed to create RawIndirectArgsBuffer. HRESULT: " << std::hex << hr << std::endl;
        return nullptr;
    }

    return buffer;
}

// CreateRawIndirectArgsBufferUAV
// Raw UAV (R32_TYPELESS + RAW flag) for a buffer that also has DRAWINDIRECT_ARGS.
void* DirectX11Renderer::CreateRawIndirectArgsBufferUAV(void* buffer)
{
    if (!buffer) return nullptr;

    ID3D11Buffer* d3dBuffer = static_cast<ID3D11Buffer*>(buffer);

    D3D11_BUFFER_DESC bufferDesc;
    d3dBuffer->GetDesc(&bufferDesc);

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = bufferDesc.ByteWidth / 4;
    uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;

    ID3D11UnorderedAccessView* uav = nullptr;
    HRESULT hr = device->CreateUnorderedAccessView(d3dBuffer, &uavDesc, &uav);

    if (FAILED(hr)) {
        std::cerr << "Failed to create RawIndirectArgsBuffer UAV. HRESULT: " << std::hex << hr << std::endl;
        return nullptr;
    }

    return uav;
}

// CopyStructureCount
// Copies the internal counter from an append/consume UAV into a destination buffer.
// This is how you get the vertex count from an append buffer into the indirect
// args buffer without a CPU readback.
//
// destBuffer: The buffer to copy into (typically your indirect args buffer)
// destByteOffset: Byte offset into destBuffer where the count should be written
// srcUAV: The UAV of an append/consume structured buffer
void DirectX11Renderer::CopyStructureCount(void* destBuffer, uint32_t destByteOffset, void* srcUAV)
{
    if (!destBuffer || !srcUAV) return;

    ID3D11Buffer* d3dDest = static_cast<ID3D11Buffer*>(destBuffer);
    ID3D11UnorderedAccessView* d3dSrcUAV = static_cast<ID3D11UnorderedAccessView*>(srcUAV);

    context->CopyStructureCount(d3dDest, destByteOffset, d3dSrcUAV);
}

// Helper: Update ByteAddressBuffer from CPU
// If your buffer was created with cpuWrite=true, use this to update it.
// Commonly used to reset an atomic counter to 0 before a dispatch.
void DirectX11Renderer::UpdateByteAddressBuffer(void* buffer, const void* data, size_t sizeBytes)
{
    if (!buffer || !data) return;

    ID3D11Buffer* d3dBuffer = static_cast<ID3D11Buffer*>(buffer);

    // Check buffer usage to determine update method
    D3D11_BUFFER_DESC desc;
    d3dBuffer->GetDesc(&desc);

    if (desc.Usage == D3D11_USAGE_DYNAMIC) {
        // DYNAMIC buffers use Map/Unmap
        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = context->Map(d3dBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (SUCCEEDED(hr)) {
            memcpy(mapped.pData, data, sizeBytes);
            context->Unmap(d3dBuffer, 0);
        }
    } else {
        // DEFAULT buffers use UpdateSubresource
        context->UpdateSubresource(d3dBuffer, 0, nullptr, data, 0, 0);
    }
}


// Helper: Unbind all compute shader resources
// CRITICAL: Call this after Dispatch() and before using any of those buffers
// for rendering. A buffer can't be bound as both UAV and SRV/VB simultaneously.
void DirectX11Renderer::UnbindComputeResources()
{
    // Unbind compute shader
    context->CSSetShader(nullptr, nullptr, 0);

    // Unbind SRVs (slots 0-7)
    ID3D11ShaderResourceView* nullSRVs[8] = { nullptr };
    context->CSSetShaderResources(0, 8, nullSRVs);

    // Unbind UAVs (slots 0-7)
    ID3D11UnorderedAccessView* nullUAVs[8] = { nullptr };
    context->CSSetUnorderedAccessViews(0, 8, nullUAVs, nullptr);

    // Unbind constant buffers (slots 0-3)
    ID3D11Buffer* nullCBs[4] = { nullptr };
    context->CSSetConstantBuffers(0, 4, nullCBs);
}
