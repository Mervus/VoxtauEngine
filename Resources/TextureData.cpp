//
// Created by Marvin on 28/01/2026.
//

#include "TextureData.h"

// Include stb_image for loading
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

TextureData::TextureData()
    : width(0)
    , height(0)
    , channels(0)
    , format(TextureFormat::Unknown)
{
}

TextureData::~TextureData() {
}

bool TextureData::LoadFromFile(const std::string& filepath) {
    int w, h, c;
    unsigned char* data = stbi_load(filepath.c_str(), &w, &h, &c, 0);

    if (!data) {
        // Failed to load
        return false;
    }

    width = w;
    height = h;
    channels = c;

    // Determine format
    switch (channels) {
        case 1: format = TextureFormat::R8; break;
        case 2: format = TextureFormat::RG8; break;
        case 3: format = TextureFormat::RGB8; break;
        case 4: format = TextureFormat::RGBA8; break;
        default: format = TextureFormat::Unknown; break;
    }

    // Copy pixel data
    size_t dataSize = w * h * c;
    pixels.resize(dataSize);
    std::memcpy(pixels.data(), data, dataSize);

    // Free stb_image data
    stbi_image_free(data);

    return true;
}

bool TextureData::LoadFromMemory(const uint8_t* data, size_t size) {
    int w, h, c;
    unsigned char* decoded = stbi_load_from_memory(
        reinterpret_cast<const stbi_uc*>(data),
        static_cast<int>(size), &w, &h, &c, 0);

    if (!decoded) {
        return false;
    }

    width = w;
    height = h;
    channels = c;

    switch (channels) {
        case 1: format = TextureFormat::R8; break;
        case 2: format = TextureFormat::RG8; break;
        case 3: format = TextureFormat::RGB8; break;
        case 4: format = TextureFormat::RGBA8; break;
        default: format = TextureFormat::Unknown; break;
    }

    size_t dataSize = w * h * c;
    pixels.resize(dataSize);
    std::memcpy(pixels.data(), decoded, dataSize);

    stbi_image_free(decoded);
    return true;
}

void TextureData::Create(int w, int h, int c, const uint8_t* data) {
    width = w;
    height = h;
    channels = c;

    size_t dataSize = w * h * c;
    pixels.resize(dataSize);

    if (data) {
        std::memcpy(pixels.data(), data, dataSize);
    } else {
        std::memset(pixels.data(), 0, dataSize);
    }

    // Determine format
    switch (channels) {
        case 1: format = TextureFormat::R8; break;
        case 2: format = TextureFormat::RG8; break;
        case 3: format = TextureFormat::RGB8; break;
        case 4: format = TextureFormat::RGBA8; break;
        default: format = TextureFormat::Unknown; break;
    }
}

const uint8_t* TextureData::GetPixels() const {
    return pixels.data();
}

uint8_t* TextureData::GetPixels() {
    return pixels.data();
}

int TextureData::GetWidth() const {
    return width;
}

int TextureData::GetHeight() const {
    return height;
}

int TextureData::GetChannels() const {
    return channels;
}

TextureFormat TextureData::GetFormat() const {
    return format;
}

size_t TextureData::GetDataSize() const {
    return pixels.size();
}

void TextureData::FlipVertically() {
    int rowSize = width * channels;
    std::vector<uint8_t> rowBuffer(rowSize);

    for (int y = 0; y < height / 2; y++) {
        uint8_t* row1 = pixels.data() + y * rowSize;
        uint8_t* row2 = pixels.data() + (height - 1 - y) * rowSize;

        // Swap rows
        std::memcpy(rowBuffer.data(), row1, rowSize);
        std::memcpy(row1, row2, rowSize);
        std::memcpy(row2, rowBuffer.data(), rowSize);
    }
}

void TextureData::ConvertToRGBA() {
    if (channels == 4) return; // Already RGBA

    std::vector<uint8_t> newPixels(width * height * 4);

    for (int i = 0; i < width * height; i++) {
        if (channels == 3) {
            // RGB -> RGBA
            newPixels[i * 4 + 0] = pixels[i * 3 + 0];
            newPixels[i * 4 + 1] = pixels[i * 3 + 1];
            newPixels[i * 4 + 2] = pixels[i * 3 + 2];
            newPixels[i * 4 + 3] = 255;
        } else if (channels == 1) {
            // Grayscale -> RGBA
            newPixels[i * 4 + 0] = pixels[i];
            newPixels[i * 4 + 1] = pixels[i];
            newPixels[i * 4 + 2] = pixels[i];
            newPixels[i * 4 + 3] = 255;
        }
    }

    pixels = std::move(newPixels);
    channels = 4;
    format = TextureFormat::RGBA8;
}