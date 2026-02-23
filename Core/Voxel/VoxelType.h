//
// Created by Marvin on 30/01/2026.
//

#ifndef VOXTAU_VOXELTYPE_H
#define VOXTAU_VOXELTYPE_H
#pragma once
#include <cstdint>
#include <EngineApi.h>

typedef uint8_t VoxelType;

enum class BlockType : uint8_t {
    Air = 0,
    Stone = 1,
    Dirt = 2,
    Grass = 3,
    Wood = 4,
    Leaves = 5
};

#endif //VOXTAU_VOXELTYPE_H