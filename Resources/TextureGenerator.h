//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_TEXTUREGENERATOR_H
#define DIRECTX11_TEXTUREGENERATOR_H

#pragma once
#include "TextureData.h"
#include <EngineApi.h>

class ENGINE_API TextureGenerator {
public:
    // Create a simple colored texture
    static TextureData CreateSolidColor(int width, int height,
        uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

    // Create a checkerboard pattern (useful for testing)
    static TextureData CreateCheckerboard(int width, int height);
};


#endif //DIRECTX11_TEXTUREGENERATOR_H