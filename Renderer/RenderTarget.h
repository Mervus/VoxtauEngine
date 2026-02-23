//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_RENDERTARGET_H
#define DIRECTX11_RENDERTARGET_H

#pragma once

#include <string>
#include <Resources/Texture.h>

namespace Math {
    class Texture; // This refers to Engine/Resources/Texture
}

class Texture; // High-level texture from Resources/

class RenderTarget {
private:
    std::string name;
    int width;
    int height;

    // Color attachments
    Texture* colorTexture;
    void* colorRTV; // ID3D11RenderTargetView* for DX11

    // Depth attachment (optional)
    Texture* depthTexture;
    void* depthDSV; // ID3D11DepthStencilView* for DX11

    bool hasDepth;

public:
    RenderTarget(const std::string& name, int width, int height, bool withDepth = true);
    ~RenderTarget();

    // Getters
    const std::string& GetName() const;
    int GetWidth() const;
    int GetHeight() const;

    Texture* GetColorTexture() const;
    Texture* GetDepthTexture() const;

    void* GetColorRTV() const;
    void* GetDepthDSV() const;

    bool HasDepth() const;

    // Setters (used by renderer)
    void SetColorTexture(Texture* tex);
    void SetDepthTexture(Texture* tex);
    void SetColorRTV(void* rtv);
    void SetDepthDSV(void* dsv);

    // Resize
    void Resize(int newWidth, int newHeight);
};


#endif //DIRECTX11_RENDERTARGET_H