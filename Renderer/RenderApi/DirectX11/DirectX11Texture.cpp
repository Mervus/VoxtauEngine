//
// Created by Marvin on 28/01/2026.
//

#include "DirectX11Texture.h"
#include <iostream>

DirectX11Texture::DirectX11Texture()
    : width(0)
    , height(0)
    , format(DXGI_FORMAT_UNKNOWN)
    , isRenderTarget(false)
{
}

DirectX11Texture::~DirectX11Texture() {
    // ComPtr automatically releases
}

bool DirectX11Texture::Create(
    ID3D11Device* device,
    ID3D11DeviceContext* context,
    int width,
    int height,
    DXGI_FORMAT format,
    const void* data,
    int channels,
    bool generateMips)
{
    this->width = width;
    this->height = height;
    this->format = format;
    this->isRenderTarget = false;

    // Create texture description
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.ArraySize = 1;
    texDesc.Format = format;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.CPUAccessFlags = 0;

    if (generateMips) {
        // For mip generation: MipLevels=0 auto-calculates, need RENDER_TARGET and GENERATE_MIPS
        texDesc.MipLevels = 0;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
    } else {
        texDesc.MipLevels = 1;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        texDesc.MiscFlags = 0;
    }

    // Create texture without initial data when generating mips
    // (we'll upload mip 0 separately then generate the rest)
    HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, &texture);
    if (FAILED(hr)) {
        std::cerr << "Failed to create DirectX11 texture!" << std::endl;
        return false;
    }

    // Create shader resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = generateMips ? -1 : 1;  // -1 = all mip levels

    hr = device->CreateShaderResourceView(texture.Get(), &srvDesc, &srv);
    if (FAILED(hr)) {
        std::cerr << "Failed to create shader resource view!" << std::endl;
        return false;
    }

    // Upload initial data to mip level 0
    if (data) {
        context->UpdateSubresource(
            texture.Get(),
            0,  // Mip level 0
            nullptr,
            data,
            width * channels,  // Row pitch
            0  // Depth pitch (unused for 2D)
        );
    }

    // Generate mipmaps
    if (generateMips && data) {
        context->GenerateMips(srv.Get());
    }

    // Create default sampler
    if (!CreateSampler(device)) {
        std::cerr << "Failed to create sampler state!" << std::endl;
        return false;
    }

    return true;
}

bool DirectX11Texture::CreateRenderTarget(
    ID3D11Device* device,
    int width,
    int height,
    DXGI_FORMAT format)
{
    this->width = width;
    this->height = height;
    this->format = format;
    this->isRenderTarget = true;

    // Create texture description for render target
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = format;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE; // Can use as texture too
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;

    // Create texture
    HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, &texture);
    if (FAILED(hr)) {
        std::cerr << "Failed to create render target texture!" << std::endl;
        return false;
    }

    // Create render target view
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = format;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;

    hr = device->CreateRenderTargetView(texture.Get(), &rtvDesc, &rtv);
    if (FAILED(hr)) {
        std::cerr << "Failed to create render target view!" << std::endl;
        return false;
    }

    // Create shader resource view (so we can sample from this RT later)
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    hr = device->CreateShaderResourceView(texture.Get(), &srvDesc, &srv);
    if (FAILED(hr)) {
        std::cerr << "Failed to create SRV for render target!" << std::endl;
        return false;
    }

    // Create sampler
    if (!CreateSampler(device)) {
        std::cerr << "Failed to create sampler state!" << std::endl;
        return false;
    }

    return true;
}

bool DirectX11Texture::CreateDepthStencil(
    ID3D11Device* device,
    int width,
    int height)
{
    this->width = width;
    this->height = height;
    this->format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    this->isRenderTarget = false;

    // Create depth stencil texture
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS; // Typeless for SRV compatibility
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;

    HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, &texture);
    if (FAILED(hr)) {
        std::cerr << "Failed to create depth stencil texture!" << std::endl;
        return false;
    }

    // Create shader resource view (for reading depth in shaders)
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    hr = device->CreateShaderResourceView(texture.Get(), &srvDesc, &srv);
    if (FAILED(hr)) {
        std::cerr << "Failed to create SRV for depth texture!" << std::endl;
        return false;
    }

    return true;
}

bool DirectX11Texture::CreateSampler(
    ID3D11Device* device,
    D3D11_FILTER filter,
    D3D11_TEXTURE_ADDRESS_MODE addressMode)
{
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = filter;
    samplerDesc.AddressU = addressMode;
    samplerDesc.AddressV = addressMode;
    samplerDesc.AddressW = addressMode;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 16;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.BorderColor[0] = 0.0f;
    samplerDesc.BorderColor[1] = 0.0f;
    samplerDesc.BorderColor[2] = 0.0f;
    samplerDesc.BorderColor[3] = 0.0f;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    HRESULT hr = device->CreateSamplerState(&samplerDesc, &sampler);
    if (FAILED(hr)) {
        return false;
    }

    return true;
}

void DirectX11Texture::Bind(ID3D11DeviceContext* context, UINT slot) {
    if (srv) {
        context->PSSetShaderResources(slot, 1, srv.GetAddressOf());
    }
    if (sampler) {
        context->PSSetSamplers(slot, 1, sampler.GetAddressOf());
    }
}

void DirectX11Texture::Unbind(ID3D11DeviceContext* context, UINT slot) {
    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->PSSetShaderResources(slot, 1, &nullSRV);
}

ID3D11Texture2D* DirectX11Texture::GetTexture() const {
    return texture.Get();
}

ID3D11ShaderResourceView* DirectX11Texture::GetSRV() const {
    return srv.Get();
}

ID3D11SamplerState* DirectX11Texture::GetSampler() const {
    return sampler.Get();
}

ID3D11RenderTargetView* DirectX11Texture::GetRTV() const {
    return rtv.Get();
}

int DirectX11Texture::GetWidth() const {
    return width;
}

int DirectX11Texture::GetHeight() const {
    return height;
}

DXGI_FORMAT DirectX11Texture::GetFormat() const {
    return format;
}

bool DirectX11Texture::IsRenderTarget() const {
    return isRenderTarget;
}