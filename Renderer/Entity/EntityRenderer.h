//
// Created by Marvin on 14/02/2026.
//

#ifndef VOXTAU_ENTITYRENDERER_H
#define VOXTAU_ENTITYRENDERER_H
#pragma once
#include "Core/Math/MathTypes.h"
#include "Core/Math/Matrix4x4.h"
#include <vector>

class IRendererApi;
class EntityManager;
class Animator;
class Mesh;
class ShaderProgram;
class Texture;

// Submitted entity for rendering
struct EntityRenderCommand {
    Math::Matrix4x4 worldMatrix;
    Mesh* mesh = nullptr;
    ShaderProgram* shader = nullptr;
    Texture* texture = nullptr;
    Animator* animator = nullptr;
};

class EntityRenderer
{
private:
    IRendererApi* _renderer = nullptr;
    // Per-frame entity queue
    std::vector<EntityRenderCommand> _entityQueue;

    void* _boneBuffer = nullptr;  // Constant buffer for bone matrices (b2)
public:
    EntityRenderer(IRendererApi* renderer);
    void Initialize();
    void Shutdown();

    void Render(void* perObjectBuffer, bool debugCameraMode);
    // Submit an entity for rendering this frame
    void SubmitEntity(const EntityRenderCommand& cmd);

    // Submit all entities from an EntityManager
    void SubmitEntities(EntityManager* entityManager);
};


#endif //VOXTAU_ENTITYRENDERER_H