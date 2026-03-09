//
// Created by Marvin on 09/03/2026.
//

#ifndef VOXTAU_BITSTREAM_H
#define VOXTAU_BITSTREAM_H
#pragma once

#include <vector>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <type_traits>
#include <span>

#include "Core/Math/Vector3.h"
#include "Core/Entity/EntityID.h"

// BitWriter serialize into a growable buffer
class BitWriter {
public:
    explicit BitWriter(size_t reserveBytes = 256) {
        _buffer.reserve(reserveBytes);
    }

    // Typed writes
    // Explicit overloads for common types. These exist, so we can
    // add compression later (e.g., varint, half-float, quantized
    // angles) per type without changing calling code.

    void Write(bool v) {
        _buffer.push_back(v ? 1 : 0);
    }

    void Write(uint8_t v) {
        _buffer.push_back(v);
    }

    void Write(int8_t v) {
        _buffer.push_back(static_cast<uint8_t>(v));
    }

    void Write(uint16_t v) {
        Append(&v, sizeof(v));
    }

    void Write(int16_t v) {
        Append(&v, sizeof(v));
    }

    void Write(uint32_t v) {
        Append(&v, sizeof(v));
    }

    void Write(int32_t v) {
        Append(&v, sizeof(v));
    }

    void Write(uint64_t v) {
        Append(&v, sizeof(v));
    }

    void Write(int64_t v) {
        Append(&v, sizeof(v));
    }

    void Write(float v) {
        Append(&v, sizeof(v));
    }

    void Write(double v) {
        Append(&v, sizeof(v));
    }

    // Engine types
    void Write(const Math::Vector3& v) {
        Write(v.x);
        Write(v.y);
        Write(v.z);
    }

    void Write(const EntityID& v) {
        Write(v.Get());
    }

    /**
    Generic write
    Fallback for any trivially_copyable type not covered above.
    This is what makes Rep<SomeCustomStruct, Scope::All> work
    without adding explicit overloads.

    SFINAE ensures this doesn't compete with the typed overloads.
    */
    template<typename T>
    std::enable_if_t<
        std::is_trivially_copyable_v<T> &&
        !std::is_arithmetic_v<T> &&
        !std::is_same_v<T, bool> &&
        !std::is_same_v<T, Math::Vector3> &&
        !std::is_same_v<T, EntityID>
    >
    Write(const T& v) {
        Append(&v, sizeof(T));
    }

    // Raw bytes
    void WriteBytes(const void* data, size_t size) {
        Append(data, size);
    }

    // Buffer access
    [[nodiscard]] const uint8_t*             Data()    const { return _buffer.data(); }
    [[nodiscard]] size_t                     Size()    const { return _buffer.size(); }
    [[nodiscard]] const std::vector<uint8_t>& Buffer()  const { return _buffer; }
    [[nodiscard]] std::vector<uint8_t>        Release()       { return std::move(_buffer); }

    [[nodiscard]] bool Empty() const { return _buffer.empty(); }

    void Clear() { _buffer.clear(); }

    // Allow building a packet header then appending the payload
    void Reserve(size_t bytes) { _buffer.reserve(_buffer.size() + bytes); }

private:
    std::vector<uint8_t> _buffer;

    void Append(const void* data, size_t size) {
        size_t offset = _buffer.size();
        _buffer.resize(offset + size);
        std::memcpy(_buffer.data() + offset, data, size);
    }
};


/**
 * Deserialize from a fixed buffer
 */
class BitReader {
public:
    // Construct from raw pointer + size (non-owning)
    BitReader(const uint8_t* data, size_t size)
        : _data(data), _size(size), _offset(0) {}

    // Construct from a vector (non-owning  vector must outlive reader)
    explicit BitReader(const std::vector<uint8_t>& buffer)
        : _data(buffer.data()), _size(buffer.size()), _offset(0) {}

    // Construct from span
    explicit BitReader(std::span<const uint8_t> buffer)
        : _data(buffer.data()), _size(buffer.size()), _offset(0) {}

    //  Typed reads 
    void Read(bool& v) {
        assert(CanRead(1));
        v = _data[_offset++] != 0;
    }

    void Read(uint8_t& v) {
        assert(CanRead(1));
        v = _data[_offset++];
    }

    void Read(int8_t& v) {
        assert(CanRead(1));
        v = static_cast<int8_t>(_data[_offset++]);
    }

    void Read(uint16_t& v) {
        Consume(&v, sizeof(v));
    }

    void Read(int16_t& v) {
        Consume(&v, sizeof(v));
    }

    void Read(uint32_t& v) {
        Consume(&v, sizeof(v));
    }

    void Read(int32_t& v) {
        Consume(&v, sizeof(v));
    }

    void Read(uint64_t& v) {
        Consume(&v, sizeof(v));
    }

    void Read(int64_t& v) {
        Consume(&v, sizeof(v));
    }

    void Read(float& v) {
        Consume(&v, sizeof(v));
    }

    void Read(double& v) {
        Consume(&v, sizeof(v));
    }

    // Engine types
    void Read(Math::Vector3& v) {
        Read(v.x);
        Read(v.y);
        Read(v.z);
    }

    void Read(EntityID& v) {
        uint32_t raw;
        Read(raw);
        v = EntityID(raw);
    }

    //  Generic read 
    template<typename T>
    std::enable_if_t<
        std::is_trivially_copyable_v<T> &&
        !std::is_arithmetic_v<T> &&
        !std::is_same_v<T, bool> &&
        !std::is_same_v<T, Math::Vector3> &&
        !std::is_same_v<T, EntityID>
    >
    Read(T& v) {
        Consume(&v, sizeof(T));
    }

    // Convenience: read and return
    // Useful for inline expressions:
    // uint32_t tick = reader.Read<uint32_t>();

    template<typename T>
    T Read() {
        T v{};
        Read(v);
        return v;
    }

    //  Raw bytes 
    void ReadBytes(void* dest, size_t size) {
        Consume(dest, size);
    }

    // Skip past bytes without reading them
    void Skip(size_t bytes) {
        assert(CanRead(bytes));
        _offset += bytes;
    }

    // State queries
    [[nodiscard]] size_t Offset()        const { return _offset; }
    [[nodiscard]] size_t Remaining()     const { return _size - _offset; }
    [[nodiscard]] size_t TotalSize()     const { return _size; }
    [[nodiscard]] bool   IsEnd()         const { return _offset >= _size; }
    [[nodiscard]] bool   HasError()      const { return _error; }

    // Check if we can safely read N more bytes
    [[nodiscard]] bool CanRead(size_t bytes) const {
        return (_offset + bytes) <= _size;
    }

    // Peek at current position without advancing
    [[nodiscard]] const uint8_t* Peek() const {
        return _data + _offset;
    }

    // Reset to beginning
    void Reset() { _offset = 0; _error = false; }

    // Seek to absolute position
    void Seek(size_t offset) {
        assert(offset <= _size);
        _offset = offset;
    }

private:
    const uint8_t* _data;
    size_t         _size;
    size_t         _offset;
    bool           _error = false;

    void Consume(void* dest, size_t size) {
        if (!CanRead(size)) {
            _error = true;
            std::memset(dest, 0, size);
            return;
        }
        std::memcpy(dest, _data + _offset, size);
        _offset += size;
    }
};

#endif //VOXTAU_BITSTREAM_H