//
// Created by Marvin on 09/02/2026.
//

#include "EntityManager.h"
#include <limits>

EntityManager::EntityManager() {
}

EntityManager::~EntityManager() {
    // Destroy all entities
    for (auto& [id, entity] : _entities) {
        if (entity) {
            entity->OnDestroy();
        }
    }
    _entities.clear();
}

// Destruction
void EntityManager::DestroyEntity(EntityID id) {
    auto it = _entities.find(id);
    if (it != _entities.end() && it->second && !it->second->IsPendingDestroy()) {
        it->second->_pendingDestroy = true;
        _pendingDestroy.push_back(id);
    }
}

void EntityManager::DestroyPending() {
    for (EntityID id : _pendingDestroy) {
        auto it = _entities.find(id);
        if (it != _entities.end()) {
            it->second->OnDestroy();
            _entities.erase(it);
        }
    }
    _pendingDestroy.clear();
}

// Lookup
Entity* EntityManager::GetEntity(EntityID id) const {
    auto it = _entities.find(id);
    if (it != _entities.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<EntityID> EntityManager::GetEntitiesByType(EntityType type) const {
    std::vector<EntityID> result;
    for (const auto& [id, entity] : _entities) {
        if (entity && entity->GetType() == type && !entity->IsPendingDestroy()) {
            result.push_back(id);
        }
    }
    return result;
}

EntityID EntityManager::FindFirstOfType(EntityType type) const {
    for (const auto& [id, entity] : _entities) {
        if (entity && entity->GetType() == type && !entity->IsPendingDestroy()) {
            return id;
        }
    }
    return EntityID::Invalid;
}

// Frame Update
void EntityManager::Update(float deltaTime) {
    for (auto& [id, entity] : _entities) {
        if (entity && entity->IsActive() && !entity->IsPendingDestroy()) {
            entity->Update(deltaTime);
        }
    }
}

void EntityManager::LateUpdate(float deltaTime) {
    for (auto& [id, entity] : _entities) {
        if (entity && entity->IsActive() && !entity->IsPendingDestroy()) {
            entity->LateUpdate(deltaTime);
        }
    }
}

// Spatial Queries
std::vector<EntityID> EntityManager::FindEntitiesInRadius(const Math::Vector3& center, float radius) const {
    std::vector<EntityID> result;
    float radiusSq = radius * radius;

    for (const auto& [id, entity] : _entities) {
        if (!entity || !entity->IsActive() || entity->IsPendingDestroy()) continue;

        Math::Vector3 diff = entity->GetPosition() - center;
        float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

        if (distSq <= radiusSq) {
            result.push_back(id);
        }
    }
    return result;
}

std::vector<EntityID> EntityManager::FindEntitiesInRadius(const Math::Vector3& center, float radius, EntityType filter) const {
    std::vector<EntityID> result;
    float radiusSq = radius * radius;

    for (const auto& [id, entity] : _entities) {
        if (!entity || !entity->IsActive() || entity->IsPendingDestroy()) continue;
        if (entity->GetType() != filter) continue;

        Math::Vector3 diff = entity->GetPosition() - center;
        float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

        if (distSq <= radiusSq) {
            result.push_back(id);
        }
    }
    return result;
}

EntityID EntityManager::FindNearestEntity(const Math::Vector3& center, EntityType filter) const {
    EntityID nearest = EntityID::Invalid;
    float nearestDistSq = std::numeric_limits<float>::max();

    for (const auto& [id, entity] : _entities) {
        if (!entity || !entity->IsActive() || entity->IsPendingDestroy()) continue;
        if (entity->GetType() != filter) continue;

        Math::Vector3 diff = entity->GetPosition() - center;
        float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

        if (distSq < nearestDistSq) {
            nearestDistSq = distSq;
            nearest = id;
        }
    }
    return nearest;
}
