//
// Created by Marvin on 09/02/2026.
//

#ifndef VOXTAU_ENTITY_H
#define VOXTAU_ENTITY_H
#pragma once
#include <EngineApi.h>
#include "EntityID.h"
#include "EntityType.h"
#include "../Scene/Transform.h"
#include "Renderer/Shaders/ShaderProgram.h"
#include "Resources/Mesh.h"
#include "Resources/Texture.h"
#include "Resources/TextureArray.h"

#include "Core/Network/Replication/Repfield.h"

class Animator;
class EntityManager;

struct RenderData {
    Mesh* mesh = nullptr;
    ShaderProgram* shader = nullptr;
    Texture* texture = nullptr;
    TextureArray* textureArray = nullptr;
    Math::Matrix4x4 worldMatrix;
    Animator* animator = nullptr;

    bool IsValid() const { return mesh != nullptr; }
    bool IsSkinned() const { return animator != nullptr; }
};

class Entity
{
    friend class EntityManager;
protected:
    std::vector<IRepField*> _repFields;
    static thread_local Entity* _constructing;

public:
    Rep<Vector3, Scope::All>  position{{0,0,0}};
    Rep<Vector3, Scope::All>  velocity{{0,0,0}};
    Rep<Math::Quaternion, Scope::All> rotation{Math::Quaternion::Identity};

private:
    EntityID _id;
    EntityType _type;

    bool _active;
    bool _pendingDestroy;
    std::string _name; // Debug only, not replicated

protected:
    Transform _transform;
    RenderData _renderData;
    Animator* _animator = nullptr;
    EntityManager* _entityManager = nullptr; // TODO: does this make sense? "find the nearest enemy" inside entity

public:
    Entity(EntityType type, const std::string& name = "Entity");
    Entity(EntityType type, Vector3 spawnPos , const std::string& name = "Entity");
    virtual ~Entity();

    virtual void OnInit() {};
    /**
     * @param deltaTime
     */
    virtual void Update(float deltaTime) {};
    virtual void LateUpdate(float deltaTime) {};
    virtual void OnDestroy() {};

    // Rendering — returns stored render data with current world matrix
    virtual RenderData GetRenderData() const {
        RenderData rd = _renderData;
        rd.worldMatrix = _transform.GetWorldMatrix();
        rd.animator = _animator;
        return rd;
    }

    void SetMesh(Mesh* mesh) { _renderData.mesh = mesh; }
    void SetShader(ShaderProgram* shader) { _renderData.shader = shader; }
    void SetTexture(Texture* texture) { _renderData.texture = texture; }

    [[nodiscard]] EntityID GetID() const { return _id; }
    [[nodiscard]] EntityType GetType() const { return _type; }
    [[nodiscard]] const std::string& GetName() const { return _name; }
    void SetName(const std::string& name) { _name = name; }

    // Transform
    Transform* GetTransform() { return &_transform; }
    const Transform* GetTransform() const { return &_transform; }

    void SetPosition(const Math::Vector3& pos) { _transform.SetPosition(pos); }
    Math::Vector3 GetPosition() const { return _transform.GetPosition(); }

    //Quaternion(x, y, z, w) sets raw quaternion components, not Euler angles. Passing
    //y=90, w=0 creates a quaternion with length 90, which produces a wildly scaled/distorted matrix.
    void SetRotation(const Math::Quaternion& rot) { _transform.SetRotation(rot); }
    Math::Quaternion GetRotation() const { return _transform.GetRotation(); }

    void SetScale(const Math::Vector3& scale) { _transform.SetScale(scale); }
    Math::Vector3 GetScale() const { return _transform.GetScale(); }

    void SetAnimator(Animator* animator) { _animator = animator; }
    Animator* GetAnimator() { return _animator; }

    // State
    bool IsActive() const { return _active; }
    void SetActive(bool active) { _active = active; }
    bool IsPendingDestroy() const { return _pendingDestroy; }


};


#endif //VOXTAU_ENTITY_H