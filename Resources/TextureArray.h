//
// Created by Mervus on 01/02/2026.
//

#ifndef VOXTAU_TEXTUREARRAY_H
#define VOXTAU_TEXTUREARRAY_H
#pragma once

#include <EngineApi.h>
#include <string>
#include <vector>

#include "Renderer/RenderStates.h"

class TextureArray {
protected:
    std::string name;
    std::vector<std::string> textureFilepaths;

    int width;
    int height;
    int arraySize;

    // Opaque handle to platform-specific texture array
    void* gpuTextureArrayHandle;  // Points to DirectX11TextureArray, etc.

public:
    TextureArray(const std::string& name = "");
    virtual ~TextureArray();

    // Getters
    [[nodiscard]] const std::string& GetName() const;
    [[nodiscard]] int GetWidth() const;
    [[nodiscard]] int GetHeight() const;
    [[nodiscard]] int GetArraySize() const;
    [[nodiscard]] const std::vector<std::string>& GetTextureFilepaths() const;

    // Setters
    void SetName(const std::string& n);
    void SetDimensions(int w, int h, int size);
    void SetTextureFilepaths(const std::vector<std::string>& paths);

    // GPU Handle
    [[nodiscard]] void* GetGPUHandle() const;
    void SetGPUHandle(void* handle);
};

#endif //VOXTAU_TEXTUREARRAY_H
