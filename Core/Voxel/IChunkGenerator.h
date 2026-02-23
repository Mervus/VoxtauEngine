//
// Created by Marvin on 05/02/2026.
//

#ifndef VOXTAU_ICHUNKGENERATOR_H
#define VOXTAU_ICHUNKGENERATOR_H

#include <cstdint>

class Chunk;

// Interface for chunk terrain generation
// Implement this in Game layer to provide terrain data to ChunkManager
class IChunkGenerator {
public:
    virtual ~IChunkGenerator() = default;

    // Generate terrain data into the given chunk
    // Chunk position is in chunk coordinates (not world coordinates)
    virtual void GenerateChunk(Chunk* chunk) const = 0;
};

#endif //VOXTAU_ICHUNKGENERATOR_H
