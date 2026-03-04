//
// Created by Marvin on 10/02/2026.
//

#include "LivingEntity.h"
#include <algorithm>

LivingEntity::LivingEntity(EntityType type, const std::string& name)
    : Entity(type, name)
    , _currentHealth(100.0f)
    , _currentMana(50.0f)
    , _isDead(false)
    , _maxHealth(100.0f)
    , _maxMana(50.0f)
    , _moveSpeed(2.5f)
    , _velocity(0,0,0)
{
}

LivingEntity::~LivingEntity() {
}

void LivingEntity::Update(float deltaTime) {
    if (_isDead) return;
    float speed = std::sqrt(_velocity.x * _velocity.x + _velocity.z * _velocity.z);
    if (speed > 0.1f) {
        float moveYaw = std::atan2(_velocity.x, _velocity.z);
        SetRotation(Math::Quaternion::FromEulerAngles(Math::Vector3(moveYaw, 0, 0)));
    }
}

// Health & Mana
void LivingEntity::SetCurrentHealth(float health) {
    _currentHealth = std::clamp(health, 0.0f, GetMaxHealth());
}

float LivingEntity::TakeDamage(float amount, EntityID source) {
    if (_isDead || amount <= 0.0f) return 0.0f;

    _currentHealth -= amount;

    if (_currentHealth <= 0.0f) {
        _currentHealth = 0.0f;
        _isDead = true;
        OnDeath(source);
    }

    return amount;
}

void LivingEntity::Heal(float amount) {
    if (_isDead || amount <= 0.0f) return;
    _currentHealth = std::min(_currentHealth + amount, GetMaxHealth());
}

// Movement
void LivingEntity::Move(const Math::Vector3& direction, float deltaTime) {
    float speed = GetMoveSpeed();
    Math::Vector3 newPos = GetPosition() + direction * speed * deltaTime;
    SetPosition(newPos);
}