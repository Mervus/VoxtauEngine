//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_TEXTUREDATA_H
#define DIRECTX11_TEXTUREDATA_H

#pragma once
#include <vector>
#include <string>
#include <cstdint>

#include "Texture.h"
#include <EngineApi.h>

class TextureData {
private:
    std::vector<uint8_t> pixels;
    int width;
    int height;
    int channels;
    TextureFormat format;

public:
    TextureData();
    ~TextureData();

    // Load from file (using stb_image or similar)
    bool LoadFromFile(const std::string& filepath);

    // Load from memory buffer (for embedded textures in FBX etc.)
    bool LoadFromMemory(const uint8_t* data, size_t size);

    // Manual creation
    void Create(int width, int height, int channels, const uint8_t* data = nullptr);

    // Getters
    const uint8_t* GetPixels() const;
    uint8_t* GetPixels();
    int GetWidth() const;
    int GetHeight() const;
    int GetChannels() const;
    TextureFormat GetFormat() const;
    size_t GetDataSize() const;

    // Utilities
    void FlipVertically();
    void ConvertToRGBA();
};


#endif //DIRECTX11_TEXTUREDATA_H