//
// Created by Marvin on 30/01/2026.
//

#ifndef VOXTAU_VOXELTYPE_H
#define VOXTAU_VOXELTYPE_H
#pragma once
#include <cstdint>
#include <EngineApi.h>

typedef uint8_t VoxelType;

// Extensible block type constants
namespace BlockType {
    constexpr VoxelType Air    = 0;
    constexpr VoxelType Stone  = 1;
    constexpr VoxelType Dirt   = 2;
    constexpr VoxelType Grass  = 3;
    constexpr VoxelType Wood   = 4;
    constexpr VoxelType Leaves = 5;
}

#endif //VOXTAU_VOXELTYPE_H