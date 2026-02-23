//
// Created by Claude on 01/02/2026.
//

#ifndef DIRECTX11_DIRECTX11TEXTUREARRAY_H
#define DIRECTX11_DIRECTX11TEXTUREARRAY_H
#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <vector>
#include <string>

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

class TextureData;

class DirectX11TextureArray {
private:
    ComPtr<ID3D11Texture2D> textureArray;
    ComPtr<ID3D11ShaderResourceView> srv;

    int width;
    int height;
    int arraySize;
    DXGI_FORMAT format;

public:
    DirectX11TextureArray();
    ~DirectX11TextureArray();

    // Create texture array from multiple TextureData objects
    // All textures must have the same dimensions
    bool Create(
        ID3D11Device* device,
        ID3D11DeviceContext* context,
        const std::vector<TextureData*>& textures,
        bool generateMips = true
    );

    // Create texture array with specific dimensions (allocates empty array)
    bool CreateEmpty(
        ID3D11Device* device,
        int width,
        int height,
        int arraySize,
        DXGI_FORMAT format,
        bool generateMips = true
    );

    // Upload data to a specific slice
    bool UploadSlice(
        ID3D11DeviceContext* context,
        int sliceIndex,
        const void* data,
        int channels
    );

    // Bind texture array to shader
    void Bind(ID3D11DeviceContext* context, UINT slot);
    void Unbind(ID3D11DeviceContext* context, UINT slot);

    // Getters
    [[nodiscard]] ID3D11Texture2D* GetTexture() const;
    [[nodiscard]] ID3D11ShaderResourceView* GetSRV() const;
    [[nodiscard]] int GetWidth() const;
    [[nodiscard]] int GetHeight() const;
    [[nodiscard]] int GetArraySize() const;
    [[nodiscard]] DXGI_FORMAT GetFormat() const;
};

#endif //DIRECTX11_DIRECTX11TEXTUREARRAY_H
