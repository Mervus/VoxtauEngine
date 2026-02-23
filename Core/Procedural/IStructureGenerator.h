//
// Created by Marvin on 31/01/2026.
//

#ifndef VOXTAU_ISTRUCTUREGENERATOR_H
#define VOXTAU_ISTRUCTUREGENERATOR_H
#include <EngineApi.h>
#include "Core/Math/Vector3.h"

class Chunk;
class Scene;

class ENGINE_API IStructureGenerator
{
public:
    virtual ~IStructureGenerator() = default;

    // Generate structure at position
    virtual void Generate(Scene* scene, Chunk* chunk, const Math::Vector3& position) = 0;

    // Check if structure can be placed here
    virtual bool CanPlaceAt(Scene* scene, const Math::Vector3& position) const = 0;

    // Get structure bounds
    virtual Math::Vector3 GetSize() const = 0;
};

#endif //VOXTAU_ISTRUCTUREGENERATOR_H