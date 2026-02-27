//
// Created by Marvin on 27/02/2026.
//

#ifndef VOXTAU_IWORLDGENERATOR_H
#define VOXTAU_IWORLDGENERATOR_H
#pragma once
#include "Core/Voxel/IChunkGenerator.h"

class ChunkManager;

class IWorldGenerator {
public:
    virtual ~IWorldGenerator() = default;
    virtual void Generate(ChunkManager* chunkManager) = 0;
    virtual void DecorateChunk(ChunkManager* chunkManager, int chunkX, int chunkY, int chunkZ) = 0;
    virtual IChunkGenerator* GetTerrainGenerator() = 0;
};

#endif //VOXTAU_IWORLDGENERATOR_H