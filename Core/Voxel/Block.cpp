//
// Created by Marvin on 04/02/2026.
//

#include "Block.h"

Block::Block(BlockType _block_type) : _blockType(_block_type)
{

}

Block::Block() : _blockType(BlockType::Air)
{

}

Block::~Block()
{
}
