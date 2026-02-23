//
// Created by Marvin on 09/02/2026.
//

#ifndef VOXTAU_ENTITYID_H
#define VOXTAU_ENTITYID_H
#include <EngineApi.h>

struct ENGINE_API EntityID {
    static constexpr uint32_t INVALID_VALUE = 0;
    static const EntityID Invalid;

    uint32_t value;

    // Default constructs to Invalid
    EntityID() : value(INVALID_VALUE) {}

    // Explicit to prevent accidental conversion from random ints
    explicit EntityID(uint32_t id) : value(id) {}

    [[nodiscard]] bool IsValid() const { return value != INVALID_VALUE; }

    // Comparison
    bool operator==(const EntityID& other) const { return value == other.value; }
    bool operator!=(const EntityID& other) const { return value != other.value; }
    bool operator<(const EntityID& other) const { return value < other.value; }

    // Raw access for serialization/debug
    [[nodiscard]] uint32_t Get() const { return value; }
};

// Define the static Invalid constant
inline const EntityID EntityID::Invalid = EntityID{};

// Hash specialization so EntityID works as an unordered_map key
template<>
struct std::hash<EntityID> {
    size_t operator()(const EntityID& id) const noexcept {
        return std::hash<uint32_t>{}(id.value);
    }
};

#endif //VOXTAU_ENTITYID_H