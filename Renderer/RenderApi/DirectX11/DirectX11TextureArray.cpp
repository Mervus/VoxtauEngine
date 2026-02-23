//
// Created by Claude on 01/02/2026.
//

#include "DirectX11TextureArray.h"
#include "Resources/TextureData.h"
#include <iostream>
#include <cmath>

DirectX11TextureArray::DirectX11TextureArray()
    : width(0)
    , height(0)
    , arraySize(0)
    , format(DXGI_FORMAT_UNKNOWN)
{
}

DirectX11TextureArray::~DirectX11TextureArray() {
    // ComPtr automatically releases
}

bool DirectX11TextureArray::Create(
    ID3D11Device* device,
    ID3D11DeviceContext* context,
    const std::vector<TextureData*>& textures,
    bool generateMips)
{
    if (textures.empty()) {
        std::cerr << "No textures provided for texture array!" << std::endl;
        return false;
    }

    // Get dimensions from first texture
    this->width = textures[0]->GetWidth();
    this->height = textures[0]->GetHeight();
    this->arraySize = static_cast<int>(textures.size());

    // Validate all textures have same dimensions
    for (size_t i = 1; i < textures.size(); i++) {
        if (textures[i]->GetWidth() != width || textures[i]->GetHeight() != height) {
            std::cerr << "Texture " << i << " has different dimensions! Expected "
                      << width << "x" << height << ", got "
                      << textures[i]->GetWidth() << "x" << textures[i]->GetHeight() << std::endl;
            return false;
        }
    }

    // Determine format - assume RGBA8 for now
    this->format = DXGI_FORMAT_R8G8B8A8_UNORM;

    // Create texture array description
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.ArraySize = arraySize;
    texDesc.Format = format;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.CPUAccessFlags = 0;

    if (generateMips) {
        texDesc.MipLevels = 0;  // Auto-calculate mip levels
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
    } else {
        texDesc.MipLevels = 1;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        texDesc.MiscFlags = 0;
    }

    // Create the texture array without initial data
    HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, &textureArray);
    if (FAILED(hr)) {
        std::cerr << "Failed to create texture array!" << std::endl;
        return false;
    }

    // Create shader resource view for the texture array
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Texture2DArray.MostDetailedMip = 0;
    srvDesc.Texture2DArray.MipLevels = generateMips ? -1 : 1;
    srvDesc.Texture2DArray.FirstArraySlice = 0;
    srvDesc.Texture2DArray.ArraySize = arraySize;

    hr = device->CreateShaderResourceView(textureArray.Get(), &srvDesc, &srv);
    if (FAILED(hr)) {
        std::cerr << "Failed to create SRV for texture array!" << std::endl;
        return false;
    }

    // Calculate actual mip levels for subresource calculation
    // For auto mip generation, compute from texture dimensions
    UINT mipLevels = 1;
    if (generateMips) {
        UINT maxDim = (width > height) ? width : height;
        mipLevels = static_cast<UINT>(std::floor(std::log2(static_cast<double>(maxDim)))) + 1;
    }

    // Upload each texture to its corresponding array slice
    for (int i = 0; i < arraySize; i++) {
        int channels = textures[i]->GetChannels();
        const uint8_t* pixels = textures[i]->GetPixels();

        // Calculate subresource index: mip 0 of slice i
        UINT subresource = D3D11CalcSubresource(0, i, mipLevels);

        // If texture is not RGBA, we need to convert
        if (channels == 4) {
            context->UpdateSubresource(
                textureArray.Get(),
                subresource,
                nullptr,
                pixels,
                width * 4,  // Row pitch
                0           // Depth pitch (unused for 2D)
            );
        } else {
            // Convert to RGBA
            std::vector<uint8_t> rgba(width * height * 4);
            for (int p = 0; p < width * height; p++) {
                if (channels == 1) {
                    rgba[p * 4 + 0] = pixels[p];
                    rgba[p * 4 + 1] = pixels[p];
                    rgba[p * 4 + 2] = pixels[p];
                    rgba[p * 4 + 3] = 255;
                } else if (channels == 3) {
                    rgba[p * 4 + 0] = pixels[p * 3 + 0];
                    rgba[p * 4 + 1] = pixels[p * 3 + 1];
                    rgba[p * 4 + 2] = pixels[p * 3 + 2];
                    rgba[p * 4 + 3] = 255;
                }
            }
            context->UpdateSubresource(
                textureArray.Get(),
                subresource,
                nullptr,
                rgba.data(),
                width * 4,
                0
            );
        }
    }

    // Generate mipmaps
    if (generateMips) {
        context->GenerateMips(srv.Get());
    }

    return true;
}

bool DirectX11TextureArray::CreateEmpty(
    ID3D11Device* device,
    int width,
    int height,
    int arraySize,
    DXGI_FORMAT format,
    bool generateMips)
{
    this->width = width;
    this->height = height;
    this->arraySize = arraySize;
    this->format = format;

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.ArraySize = arraySize;
    texDesc.Format = format;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.CPUAccessFlags = 0;

    if (generateMips) {
        texDesc.MipLevels = 0;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
    } else {
        texDesc.MipLevels = 1;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        texDesc.MiscFlags = 0;
    }

    HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, &textureArray);
    if (FAILED(hr)) {
        std::cerr << "Failed to create empty texture array!" << std::endl;
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Texture2DArray.MostDetailedMip = 0;
    srvDesc.Texture2DArray.MipLevels = generateMips ? -1 : 1;
    srvDesc.Texture2DArray.FirstArraySlice = 0;
    srvDesc.Texture2DArray.ArraySize = arraySize;

    hr = device->CreateShaderResourceView(textureArray.Get(), &srvDesc, &srv);
    if (FAILED(hr)) {
        std::cerr << "Failed to create SRV for empty texture array!" << std::endl;
        return false;
    }

    return true;
}

bool DirectX11TextureArray::UploadSlice(
    ID3D11DeviceContext* context,
    int sliceIndex,
    const void* data,
    int channels)
{
    if (sliceIndex < 0 || sliceIndex >= arraySize) {
        std::cerr << "Invalid slice index: " << sliceIndex << std::endl;
        return false;
    }

    UINT subresource = D3D11CalcSubresource(0, sliceIndex, 1);

    if (channels == 4) {
        context->UpdateSubresource(
            textureArray.Get(),
            subresource,
            nullptr,
            data,
            width * 4,
            0
        );
    } else {
        // Convert to RGBA
        const uint8_t* pixels = static_cast<const uint8_t*>(data);
        std::vector<uint8_t> rgba(width * height * 4);
        for (int p = 0; p < width * height; p++) {
            if (channels == 1) {
                rgba[p * 4 + 0] = pixels[p];
                rgba[p * 4 + 1] = pixels[p];
                rgba[p * 4 + 2] = pixels[p];
                rgba[p * 4 + 3] = 255;
            } else if (channels == 3) {
                rgba[p * 4 + 0] = pixels[p * 3 + 0];
                rgba[p * 4 + 1] = pixels[p * 3 + 1];
                rgba[p * 4 + 2] = pixels[p * 3 + 2];
                rgba[p * 4 + 3] = 255;
            }
        }
        context->UpdateSubresource(
            textureArray.Get(),
            subresource,
            nullptr,
            rgba.data(),
            width * 4,
            0
        );
    }

    return true;
}

void DirectX11TextureArray::Bind(ID3D11DeviceContext* context, UINT slot) {
    if (srv) {
        context->PSSetShaderResources(slot, 1, srv.GetAddressOf());
    }
}

void DirectX11TextureArray::Unbind(ID3D11DeviceContext* context, UINT slot) {
    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->PSSetShaderResources(slot, 1, &nullSRV);
}

ID3D11Texture2D* DirectX11TextureArray::GetTexture() const {
    return textureArray.Get();
}

ID3D11ShaderResourceView* DirectX11TextureArray::GetSRV() const {
    return srv.Get();
}

int DirectX11TextureArray::GetWidth() const {
    return width;
}

int DirectX11TextureArray::GetHeight() const {
    return height;
}

int DirectX11TextureArray::GetArraySize() const {
    return arraySize;
}

DXGI_FORMAT DirectX11TextureArray::GetFormat() const {
    return format;
}
