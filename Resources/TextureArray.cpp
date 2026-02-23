//
// Created by Claude on 01/02/2026.
//

#include "TextureArray.h"

TextureArray::TextureArray(const std::string& name)
    : name(name)
    , width(0)
    , height(0)
    , arraySize(0)
    , gpuTextureArrayHandle(nullptr)
{
}

TextureArray::~TextureArray() {
    // Note: GPU handle should be cleaned up by renderer
    // We don't delete it here as it may be managed elsewhere
}

const std::string& TextureArray::GetName() const {
    return name;
}

int TextureArray::GetWidth() const {
    return width;
}

int TextureArray::GetHeight() const {
    return height;
}

int TextureArray::GetArraySize() const {
    return arraySize;
}

const std::vector<std::string>& TextureArray::GetTextureFilepaths() const {
    return textureFilepaths;
}

void TextureArray::SetName(const std::string& n) {
    name = n;
}

void TextureArray::SetDimensions(int w, int h, int size) {
    width = w;
    height = h;
    arraySize = size;
}

void TextureArray::SetTextureFilepaths(const std::vector<std::string>& paths) {
    textureFilepaths = paths;
}

void* TextureArray::GetGPUHandle() const {
    return gpuTextureArrayHandle;
}

void TextureArray::SetGPUHandle(void* handle) {
    gpuTextureArrayHandle = handle;
}
