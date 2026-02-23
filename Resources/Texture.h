//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_TEXTURE_H
#define DIRECTX11_TEXTURE_H
#pragma once

#include <EngineApi.h>
#include <string>

#include "Renderer/RenderStates.h"

enum class TextureFormat {
    R8, RG8, RGB8, RGBA8, R16F, RGBA16F, RGBA32F, Depth24Stencil8, Unknown
};

class ENGINE_API Texture {
protected:
    std::string name;
    std::string filepath;

    int width;
    int height;
    int channels;
    TextureFormat format;

    // OPAQUE handle to platform-specific texture
    void* gpuTextureHandle; // Points to DirectX11Texture, etc.

    TextureFilter filter;
    TextureWrap wrap;

public:
    Texture(const std::string& name = "");
    virtual ~Texture();

    // Getters/Setters
    const void GetName(std::string& n) const;
    const std::string& GetName() const;
    int GetWidth() const;
    int GetHeight() const;
    TextureFormat GetFormat() const;
    void SetName(const std::string& n);

    void* GetGPUHandle() const;
    void SetGPUHandle(void* handle);

    TextureFilter GetFilter() const;
    TextureWrap GetWrap() const;
    void SetFilter(TextureFilter f);
    void SetWrap(TextureWrap w);

    void SetFilepath(const std::string& path);
    // Internal use
    void SetDimensions(int w, int h, int ch);
    void SetFormat(TextureFormat fmt);
};


#endif //DIRECTX11_TEXTURE_H