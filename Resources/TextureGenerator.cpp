//
// Created by Marvin on 28/01/2026.
//

#include "TextureGenerator.h"

TextureData TextureGenerator::CreateSolidColor(int width, int height,
    uint8_t r, uint8_t g, uint8_t b, uint8_t a) {

    TextureData texData;

    std::vector<uint8_t> pixels(width * height * 4);

    for (int i = 0; i < width * height; i++) {
        pixels[i * 4 + 0] = r;
        pixels[i * 4 + 1] = g;
        pixels[i * 4 + 2] = b;
        pixels[i * 4 + 3] = a;
    }

    texData.Create(width, height, 4, pixels.data());

    return texData;
}

TextureData TextureGenerator::CreateCheckerboard(int width, int height) {
    TextureData texData;

    std::vector<uint8_t> pixels(width * height * 4);

    int tileSize = 8; // 8x8 pixel tiles

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = (y * width + x) * 4;

            // Checkerboard pattern
            bool isWhite = ((x / tileSize) + (y / tileSize)) % 2 == 0;

            uint8_t color = isWhite ? 255 : 128;

            pixels[index + 0] = color; // R
            pixels[index + 1] = color; // G
            pixels[index + 2] = color; // B
            pixels[index + 3] = 255;   // A
        }
    }

    texData.Create(width, height, 4, pixels.data());

    return texData;
}