//
// Created by Marvin on 04/02/2026.
//

#ifndef VOXTAU_BLOCK_H
#define VOXTAU_BLOCK_H
#include "Voxel.h"
#include "EngineApi.h"
#include "VoxelType.h"

class Block : public Voxel
{
public:
    Block(VoxelType _block_type);
    Block();
    ~Block();
private:
    VoxelType _blockType;
};


#endif //VOXTAU_BLOCK_H