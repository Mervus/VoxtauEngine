//
// Created by Marvin on 10/02/2026.
//

#include "LivingEntity.h"
#include <algorithm>

LivingEntity::LivingEntity(EntityType type, const std::string& name)
    : Entity(type, name)
    , _currentHealth(100.0f)
    , _isDead(false)
    , _maxHealth(100.0f)
    , _maxMana(50.0f)
    , _moveSpeed(2.5f)
    , _velocity(Vector3::Zero)
{
}

LivingEntity::~LivingEntity() {
}

void LivingEntity::Update(float deltaTime) {
    if (_isDead) return;

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