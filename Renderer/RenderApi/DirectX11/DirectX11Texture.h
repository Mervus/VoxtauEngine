//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_DIRECTX11TEXTURE_H
#define DIRECTX11_DIRECTX11TEXTURE_H
#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <string>

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

class DirectX11Texture {
private:
    ComPtr<ID3D11Texture2D> texture;
    ComPtr<ID3D11ShaderResourceView> srv;
    ComPtr<ID3D11SamplerState> sampler;
    ComPtr<ID3D11RenderTargetView> rtv; // For render targets

    int width;
    int height;
    DXGI_FORMAT format;
    bool isRenderTarget;

public:
    DirectX11Texture();
    ~DirectX11Texture();

    bool Create(
        ID3D11Device* device,
        ID3D11DeviceContext* context,
        int width,
        int height,
        DXGI_FORMAT format,
        const void* data,
        int channels,
        bool generateMips = true
    );

    // Create render target texture
    bool CreateRenderTarget(
        ID3D11Device* device,
        int width,
        int height,
        DXGI_FORMAT format
    );

    // Create depth stencil texture
    bool CreateDepthStencil(
        ID3D11Device* device,
        int width,
        int height
    );

    // Create sampler state
    bool CreateSampler(
        ID3D11Device* device,
        D3D11_FILTER filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
        D3D11_TEXTURE_ADDRESS_MODE addressMode = D3D11_TEXTURE_ADDRESS_WRAP
    );

    // Bind texture to shader
    void Bind(ID3D11DeviceContext* context, UINT slot);
    void Unbind(ID3D11DeviceContext* context, UINT slot);

    // Getters
    [[nodiscard]] ID3D11Texture2D* GetTexture() const;
    [[nodiscard]] ID3D11ShaderResourceView* GetSRV() const;
    [[nodiscard]] ID3D11SamplerState* GetSampler() const;
    [[nodiscard]] ID3D11RenderTargetView* GetRTV() const;

    int GetWidth() const;
    int GetHeight() const;
    DXGI_FORMAT GetFormat() const;
    bool IsRenderTarget() const;
};


#endif //DIRECTX11_DIRECTX11TEXTURE_H