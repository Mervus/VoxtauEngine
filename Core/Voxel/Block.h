//
// Created by Marvin on 04/02/2026.
//

#ifndef VOXTAU_BLOCK_H
#define VOXTAU_BLOCK_H
#include "Voxel.h"
#include "EngineApi.h"
#include "VoxelType.h"

class ENGINE_API Block : public Voxel
{
public:
    Block(BlockType _block_type);
    Block();
    ~Block();
private:
    BlockType _blockType;
};


#endif //VOXTAU_BLOCK_H