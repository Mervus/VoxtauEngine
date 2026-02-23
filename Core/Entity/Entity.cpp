//
// Created by Marvin on 09/02/2026.
//

#include "Entity.h"

Entity::Entity(EntityType type, const std::string& name)
    : _id()
    , _type(type)
    , _active(true)
    , _pendingDestroy(false)
    , _name(name)
    , _entityManager(nullptr)
{
}

Entity::~Entity() {
}