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
{
}

Entity::Entity(EntityType type, Vector3 spawnPos, const std::string& name) : Entity(type, name)
{
    SetPosition(spawnPos);
}

Entity::~Entity() {
}

void Entity::SyncTransform() {
    _transform.SetPosition(position.Get());
    _transform.SetRotation(rotation.Get());
}

void Entity::SerializeScope(BitWriter& writer, Scope scope) const {
    for (const auto* field : _repFields) {
        if (field->scope == scope) {
            field->Write(writer);
        }
    }
}

void Entity::DeserializeScope(BitReader& reader, Scope scope) {
    for (auto* field : _repFields) {
        if (field->scope == scope) {
            field->Read(reader);
        }
    }
}