//
// Created by Marvin on 09/03/2026.
//
// Self-registering replicated field system.
// Fields declare their type and network scope inline the engine
// discovers them automatically and handles serialization, delta
// compression, and scope-filtered replication.
//
// Usage (in game entity classes):
//
//   class PlayerEntity: public LivingEntity {
//       Rep<float, Scope::Owner>    mana{50.0f};
//       Rep<uint16_t, Scope::All>   weaponId{0};
//       Rep<float, Scope::Server>   cheatScore{0.0f};
//   };
//

#ifndef VOXTAU_REPFIELD_H
#define VOXTAU_REPFIELD_H
#pragma once

#include <vector>
#include <cstdint>
#include <cstring>
#include <type_traits>

struct IRepField;
// Forward declarations engine provides these
class BitWriter;
class BitReader;

/**
 * Replication scopes
 */
enum class Scope : uint8_t {
    /** Sent to every client in area of interest */
    All,
    /** Sent to the client that owns the entity */
    Owner,
    /** Sent to the server, but not to the clients */
    Server
};

/**
 *   Entity's constructor sets the active field list before
 *   member construction begins. Each Rep<> field pushes itself
 *   into this list during its own construction.
 *
 *   This avoids coupling Rep<> to the Entity class directly.
 */
namespace RepContext {
    inline thread_local std::vector<IRepField*>* activeFieldList = nullptr;
}

/**
    IRepField  type-erased field interface

    The engine iterates these without knowing concrete types.
    Stored as pointers into the owning entity's member layout.
 */
struct IRepField {
    Scope scope;

    virtual ~IRepField() = default;

    // Serialization
    virtual void Write(BitWriter& writer) const = 0;
    virtual void Read(BitReader& reader) = 0;

    // Raw access for baseline storage and delta comparison
    [[nodiscard]] virtual const void* Data() const = 0;
    [[nodiscard]] virtual size_t      DataSize() const = 0;

    // Compare this field's current value against a baseline byte buffer
    // at the given offset. Returns true if the values are identical.
    bool EqualsBaseline(const uint8_t* baseline, size_t offset) const {
        return std::memcmp(Data(), baseline + offset, DataSize()) == 0;
    }

    // Copy this field's current value into a buffer at the given offset.
    void CopyToBaseline(uint8_t* baseline, size_t offset) const {
        std::memcpy(baseline + offset, Data(), DataSize());
    }

    // Restore this field's value from a baseline buffer at the given offset.
    void CopyFromBaseline(const uint8_t* baseline, size_t offset) {
        std::memcpy(const_cast<void*>(Data()), baseline + offset, DataSize());
    }
};

/**
    Rep<T, S> the replicated field wrapper

    Holds a value of type T, tagged with scope S.
    Self-registers into the owning entity's field list
    during construction.

    Supports implicit conversion and assignment so game
    code can treat it like a plain variable:

      _health = _health - damage;
      float h = _health;
      if (_isDead) { ... }
*/
template<typename T, Scope S>
class Rep final : public IRepField {
    static_assert(std::is_trivially_copyable_v<T>,
        "Rep<T> requires a trivially copyable type for safe serialization");

    T _value;

public:
    //  Construction 
    explicit Rep(T defaultValue = T{}) : _value(defaultValue) {
        this->scope = S;

        // Self-register into the entity currently being constructed
        if (RepContext::activeFieldList) {
            RepContext::activeFieldList->push_back(this);
        }
    }

    // Prevent copying/moving each Rep is tied to its
    // owning entity's memory layout and field list position.
    Rep(const Rep&)            = delete;
    Rep& operator=(const Rep&) = delete;
    Rep(Rep&&)                 = delete;
    Rep& operator=(Rep&&)      = delete;

    ~Rep() override = default;

    //  Value access 

    // Implicit read use the field like a plain variable
    operator const T&() const { return _value; }

    const T& Get() const { return _value; }

    // Assignment from raw value
    Rep& operator=(const T& v) {
        _value = v;
        return *this;
    }

    // Pointer-style access for structs (e.g., position->x)
    const T* operator->() const { return &_value; }

    //  Comparison operators
    bool operator==(const T& other) const { return _value == other; }
    bool operator!=(const T& other) const { return _value != other; }
    bool operator<(const T& other)  const { return _value < other; }
    bool operator>(const T& other)  const { return _value > other; }
    bool operator<=(const T& other) const { return _value <= other; }
    bool operator>=(const T& other) const { return _value >= other; }

    //  Arithmetic operators 
    // These return T, not Rep so expressions like
    //   _health = _health - damage
    // work naturally without extra temporaries.
    T operator+(const T& rhs) const { return _value + rhs; }
    T operator-(const T& rhs) const { return _value - rhs; }
    T operator*(const T& rhs) const { return _value * rhs; }
    T operator/(const T& rhs) const { return _value / rhs; }

    Rep& operator+=(const T& rhs) { _value += rhs; return *this; }
    Rep& operator-=(const T& rhs) { _value -= rhs; return *this; }
    Rep& operator*=(const T& rhs) { _value *= rhs; return *this; }
    Rep& operator/=(const T& rhs) { _value /= rhs; return *this; }

    //  IRepField interface
    void Write(BitWriter& writer) const override;
    void Read(BitReader& reader) override;

    [[nodiscard]] const void* Data() const override { return &_value; }
    [[nodiscard]] size_t DataSize() const override  { return sizeof(T); }
};

// 
//  Serialization implemented in RepField.inl or
//  specialized per type after BitWriter/BitReader
//  are defined.
// 
//
//  Default: raw memcpy-style write/read.
//  Specialize for types that need custom encoding
//  (e.g., compressed quaternions, variable-length strings).

// 
//  Scope query helpers for engine-side iteration
// 

// Calculate total serialized size for all fields of a given scope
inline size_t GetScopeDataSize(const std::vector<IRepField*>& fields, Scope scope) {
    size_t total = 0;
    for (const auto* f : fields) {
        if (f->scope == scope) {
            total += f->DataSize();
        }
    }
    return total;
}

// Serialize all fields of a given scope into a byte buffer.
// Returns number of bytes written.
inline size_t SerializeScopeToBuffer(const std::vector<IRepField*>& fields,
                                     Scope scope,
                                     uint8_t* buffer,
                                     size_t bufferSize) {
    size_t offset = 0;
    for (const auto* f : fields) {
        if (f->scope == scope) {
            if (offset + f->DataSize() > bufferSize) break;
            f->CopyToBaseline(buffer, offset);
            offset += f->DataSize();
        }
    }
    return offset;
}

// Compare all fields of a given scope against a baseline buffer.
// Returns true if ALL fields match (nothing dirty).
inline bool CompareScopeBaseline(const std::vector<IRepField*>& fields,
                                  Scope scope,
                                  const uint8_t* baseline) {
    size_t offset = 0;
    for (const auto* f : fields) {
        if (f->scope == scope) {
            if (!f->EqualsBaseline(baseline, offset)) {
                return false;
            }
            offset += f->DataSize();
        }
    }
    return true;
}

#include "RepField.inl"

#endif //VOXTAU_REPFIELD_H