//
// Created by Marvin on 10/02/2026.
//

#ifndef VOXTAU_LIVINGENTITY_H
#define VOXTAU_LIVINGENTITY_H
#pragma once
#include <EngineApi.h>
#include "Core/Entity/Entity.h"

class ENGINE_API LivingEntity : public Entity {
private:
    float _currentHealth;
    float _currentMana;
    bool _isDead;

protected:
    float _maxHealth;
    float _maxMana;
    float _moveSpeed;
    int _level;

    // Called when health reaches zero — override for death behavior
    virtual void OnDeath(EntityID killedBy) {}

public:
    LivingEntity(EntityType type, const std::string& name = "LivingEntity");
    virtual ~LivingEntity();

    void Update(float deltaTime) override;

    // Health
    // Virtual so game layer can compute from RPG stats + modifiers
    virtual float GetMaxHealth() const { return _maxHealth; }
    virtual float GetMoveSpeed() const { return _moveSpeed; }

    float GetCurrentHealth() const { return _currentHealth; }
    bool IsDead() const { return _isDead; }

    void SetMaxHealth(float value) { _maxHealth = value; }
    void SetMoveSpeed(float value) { _moveSpeed = value; }
    void SetCurrentHealth(float health);

    // Damage/heal — virtual so game layer can apply custom formulas
    virtual float TakeDamage(float amount, EntityID source);
    virtual void Heal(float amount);

    // Level
    int GetLevel() const { return _level; }
    void SetLevel(int level) { _level = level; }

    // Movement
    virtual void Move(const Math::Vector3& direction, float deltaTime);

    // Replication
    // virtual void SerializeNetState(ByteBuffer& buffer) override;
    // virtual void DeserializeNetState(ByteBuffer& buffer) override;
};


#endif //VOXTAU_LIVINGENTITY_H