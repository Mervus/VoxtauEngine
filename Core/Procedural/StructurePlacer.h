//
// Created by Marvin on 31/01/2026.
//

#ifndef VOXTAU_STRUCTUREPLACER_H
#define VOXTAU_STRUCTUREPLACER_H
#pragma once
#include <EngineApi.h>
#include <vector>
#include "Core/Math/Vector3.h"

class IStructureGenerator;
class Chunk;

class StructurePlacer
{
public:
    // Place structures in chunk based on density
    static void PlaceStructures(
        Chunk* chunk,
        const std::vector<IStructureGenerator*>& generators,
        float density,
        int seed
    );

    // Check if area is clear for structure
    static bool IsAreaClear(
        Chunk* chunk,
        const Math::Vector3& position,
        const Math::Vector3& size
    );
};


#endif //VOXTAU_STRUCTUREPLACER_H