//
// Created by Marvin on 10/02/2026.
//

#ifndef VOXTAU_VOXMODEL_H
#define VOXTAU_VOXMODEL_H
#pragma once
#include <cstdint>
#include <vector>
#include <EngineApi.h>

struct VoxColor {
    uint8_t r, g, b, a;
};

struct VoxVoxel {
    uint8_t x, y, z;
    uint8_t colorIndex; // Index into the palette (1-255, 0 = empty)
};

struct VoxModel {
    uint32_t sizeX = 0;
    uint32_t sizeY = 0;
    uint32_t sizeZ = 0;

    std::vector<VoxVoxel> voxels;
    VoxColor palette[256]; // 256 colors, index 0 is unused

    // Get voxel color (returns transparent black if index is 0/invalid)
    VoxColor GetColor(uint8_t index) const {
        if (index == 0) return {0, 0, 0, 0};
        return palette[index];
    }
};

#endif //VOXTAU_VOXMODEL_H