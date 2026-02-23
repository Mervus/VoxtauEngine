//
// Created by Marvin on 28/01/2026.
//

#include "RenderTarget.h"
#include <Unknwn.h> // For IUnknown::Release()


RenderTarget::RenderTarget(const std::string& name, int width, int height, bool withDepth)
    : name(name)
    , width(width)
    , height(height)
    , colorTexture(nullptr)
    , colorRTV(nullptr)
    , depthTexture(nullptr)
    , depthDSV(nullptr)
    , hasDepth(withDepth)
{
}

RenderTarget::~RenderTarget() {
    // Release depth stencil view if we own it
    if (depthDSV) {
        // Cast to ID3D11DepthStencilView and release
        // Note: This assumes DirectX11, for a more portable solution
        // this would be handled by the renderer
        static_cast<IUnknown*>(depthDSV)->Release();
        depthDSV = nullptr;
    }

    // Color texture handles its own RTV release through DirectX11Texture
    delete colorTexture;
    delete depthTexture;
}

const std::string& RenderTarget::GetName() const {
    return name;
}

int RenderTarget::GetWidth() const {
    return width;
}

int RenderTarget::GetHeight() const {
    return height;
}

Texture* RenderTarget::GetColorTexture() const {
    return colorTexture;
}

Texture* RenderTarget::GetDepthTexture() const {
    return depthTexture;
}

void* RenderTarget::GetColorRTV() const {
    return colorRTV;
}

void* RenderTarget::GetDepthDSV() const {
    return depthDSV;
}

bool RenderTarget::HasDepth() const {
    return hasDepth;
}

void RenderTarget::SetColorTexture(Texture* tex) {
    colorTexture = tex;
}

void RenderTarget::SetDepthTexture(Texture* tex) {
    depthTexture = tex;
}

void RenderTarget::SetColorRTV(void* rtv) {
    colorRTV = rtv;
}

void RenderTarget::SetDepthDSV(void* dsv) {
    depthDSV = dsv;
}

void RenderTarget::Resize(int newWidth, int newHeight) {
    width = newWidth;
    height = newHeight;
    // Renderer needs to recreate GPU resources
}