//
// Created by Marvin on 31/01/2026.
//

#include "StructurePlacer.h"

#include <cstdlib>

#include "IStructureGenerator.h"
#include "Core/Voxel/Chunk.h"
#include "Core/Voxel/VoxelType.h"

void StructurePlacer::PlaceStructures(Chunk* chunk, const std::vector<IStructureGenerator*>& generators,
    float density, int seed)
{
}

bool StructurePlacer::IsAreaClear(Chunk* chunk, const Math::Vector3& position, const Math::Vector3& size)
{
    return true;
}
