//
// Created by Marvin on 09/02/2026.
//

#ifndef VOXTAU_ENTITYMANAGER_H
#define VOXTAU_ENTITYMANAGER_H
#pragma once
#include <EngineApi.h>
#include <unordered_map>

#include "Entity.h"
#include "EntityID.h"

#include "Core/Network/Replication/Repfield.h"

class EntityManager
{
private:
    std::unordered_map<EntityID, std::unique_ptr<Entity>> _entities;
    std::vector<EntityID> _pendingDestroy;
    uint32_t _nextId = 1; // 0 is reserved for Invalid
    EntityID _playerID; // Cached for O(1) access

public:
    EntityManager();
    EntityManager(EntityID playerID) : _playerID(playerID) {};
    ~EntityManager();


    // Cannot copy EntityManager
    EntityManager(const EntityManager&) = delete;
    EntityManager& operator=(const EntityManager&) = delete;
    EntityManager(EntityManager&&) = default;
    EntityManager& operator=(EntityManager&&) = default;

    // Creation
    // Creates an entity of concrete type T, assigns an ID, calls OnInit()
    // Returns the EntityID (not a pointer) — use GetEntity() to resolve
    template<typename T, typename... Args>
    EntityID CreateEntity(Args&&... args);

    template<typename T, typename... Args>
    EntityID CreateEntityWithID(EntityID id, Args&&... args);

    // Destruction
    // Marks entity for deferred destruction (safe to call during Update)
    void DestroyEntity(EntityID id);
    // Actually removes pending entities — call at end of frame
    void DestroyPending();

    // Lookup
    [[nodiscard]] Entity* GetEntity(EntityID id) const;

    template<typename T>
    T* GetEntityAs(EntityID id) const;

    // Returns all entity IDs matching the given type
    [[nodiscard]] std::vector<EntityID> GetEntitiesByType(EntityType type) const;
    // Returns the first entity found of the given type (useful for Player)
    [[nodiscard]] EntityID FindFirstOfType(EntityType type) const;
    // Total active entity count
    [[nodiscard]] size_t GetEntityCount() const { return _entities.size(); }

    // Player
    [[nodiscard]] EntityID GetPlayerID() const { return _playerID; }
    [[nodiscard]] Entity* GetPlayer() const { return GetEntity(_playerID); }

    template<typename T>
    T* GetPlayerAs() const { return GetEntityAs<T>(_playerID); }

    // Frame Update
    void Update(float deltaTime);
    void LateUpdate(float deltaTime);

    // Spatial Queries

    // Flat loop fine for < 200 entities, swap to spatial hash later (Something like Quadtree or Octree.)
    [[nodiscard]] std::vector<EntityID> FindEntitiesInRadius(const Math::Vector3& center, float radius) const;
    [[nodiscard]] std::vector<EntityID> FindEntitiesInRadius(const Math::Vector3& center, float radius, EntityType filter) const;
    [[nodiscard]] EntityID FindNearestEntity(const Math::Vector3& center, EntityType filter) const;

    // Iteration
    // Calls a function for each active entity
    // Usage: manager.ForEach([](Entity* e) { ... });
    template<typename Func>
    void ForEach(Func&& func) const;

    template<typename T, typename Func>
    void ForEachOfType(EntityType type, Func&& func) const;
};

// Template Implementations
template<typename T, typename... Args>
EntityID EntityManager::CreateEntity(Args&&... args) {
    static_assert(std::is_base_of_v<Entity, T>, "T must derive from Entity");

    EntityID id((_nextId++));

    auto entity = std::make_unique<T>(std::forward<Args>(args)...);
    RepContext::activeFieldList = nullptr;

    entity->_id = id;
    entity->_entityManager = this;

    entity->OnInit();

    _entities[id] = std::move(entity);
    return id;
}

template<typename T, typename... Args>
EntityID EntityManager::CreateEntityWithID(EntityID id, Args&&... args) {
    static_assert(std::is_base_of_v<Entity, T>, "T must derive from Entity");

    if (id.Get() == 0 || _entities.contains(id))
    {
        return EntityID::Invalid;
    }

    auto entity = std::make_unique<T>(std::forward<Args>(args)...);
    RepContext::activeFieldList = nullptr;

    entity->_id = id;
    entity->_entityManager = this;

    entity->OnInit();

    _entities[id] = std::move(entity);

    // Keep counter ahead to avoid collisions if CreateEntity is also used
    if (id.Get() >= _nextId) {
        _nextId = id.Get() + 1;
    }

    return id;
}

template<typename T>
T* EntityManager::GetEntityAs(EntityID id) const {
    Entity* entity = GetEntity(id);
    if (!entity) return nullptr;
    return static_cast<T*>(entity);
}

template<typename Func>
void EntityManager::ForEach(Func&& func) const {
    for (const auto& [id, entity] : _entities) {
        if (entity && entity->IsActive() && !entity->IsPendingDestroy()) {
            func(entity.get());
        }
    }
}

template<typename T, typename Func>
void EntityManager::ForEachOfType(EntityType type, Func&& func) const {
    for (const auto& [id, entity] : _entities) {
        if (entity && entity->IsActive() && !entity->IsPendingDestroy() && entity->GetType() == type) {
            func(static_cast<T*>(entity.get()));
        }
    }
}

#endif //VOXTAU_ENTITYMANAGER_H