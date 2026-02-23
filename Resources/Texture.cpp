//
// Created by Marvin on 28/01/2026.
//

#include "Texture.h"


Texture::Texture(const std::string& name)
    : name(name)
    , filepath("")
    , width(0)
    , height(0)
    , channels(0)
    , format(TextureFormat::Unknown)
    , gpuTextureHandle(nullptr)
    , filter(TextureFilter::Linear)
    , wrap(TextureWrap::Repeat)
{
}

Texture::~Texture() {
    // GPU handle released by renderer
}

const std::string& Texture::GetName() const {
    return name;
}


int Texture::GetWidth() const {
    return width;
}

int Texture::GetHeight() const {
    return height;
}


TextureFormat Texture::GetFormat() const {
    return format;
}


void Texture::SetName(const std::string& n) {
    name = n;
}

void Texture::SetFilepath(const std::string& path) {
    filepath = path;
}

void Texture::SetDimensions(int w, int h, int ch) {
    width = w;
    height = h;
    channels = ch;
}

void Texture::SetFormat(TextureFormat fmt) {
    format = fmt;
}

void* Texture::GetGPUHandle() const {
    return gpuTextureHandle;
}

void Texture::SetGPUHandle(void* handle) {
    gpuTextureHandle = handle;
}

TextureFilter Texture::GetFilter() const {
    return filter;
}

TextureWrap Texture::GetWrap() const {
    return wrap;
}

void Texture::SetFilter(TextureFilter f) {
    filter = f;
}

void Texture::SetWrap(TextureWrap w) {
    wrap = w;
}