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
#include "Core/Math/Quaternion.h"

#include "Core/Entity/EntityID.h"

// BitWriter serialize into a growable buffer
class BitWriter {
public:
    explicit BitWriter(size_t reserveBytes = 256)
    {
        _buffer.reserve(reserveBytes);
    }

    // Typed writes
    // Explicit overloads for common types. These exist, so we can
    // add compression later (e.g., varint, half-float, quantized
    // angles) per type without changing calling code.

    void Write(bool v)
    {
        _buffer.push_back(v ? 1 : 0);
    }

    void Write(uint8_t v)
    {
        _buffer.push_back(v);
    }

    void Write(int8_t v)
    {
        _buffer.push_back(static_cast<uint8_t>(v));
    }

    void Write(uint16_t v)
    {
        Append(&v, sizeof(v));
    }

    void Write(int16_t v)
    {
        Append(&v, sizeof(v));
    }

    void Write(uint32_t v)
    {
        Append(&v, sizeof(v));
    }

    void Write(int32_t v)
    {
        Append(&v, sizeof(v));
    }

    void Write(uint64_t v)
    {
        Append(&v, sizeof(v));
    }

    void Write(int64_t v)
    {
        Append(&v, sizeof(v));
    }

    void Write(float v)
    {
        Append(&v, sizeof(v));
    }

    void Write(double v)
    {
        Append(&v, sizeof(v));
    }

    // Engine types
    void Write(const Math::Vector3& v)
    {
        Write(v.x);
        Write(v.y);
        Write(v.z);
    }

    void Write(const EntityID& v)
    {
        Write(v.Get());
    }

    void Write(const Math::Quaternion& v)
    {
        Write(v.x);
        Write(v.y);
        Write(v.z);
        Write(v.w);
    }

    /**
    * Compressed quaternion 7 bytes, smallest-three encoding.
    * Drops the largest component (reconstructable since |q|=1),
    * quantizes the remaining three to uint16 each.
    * Layout: [1 byte: 2-bit index + 6 unused] [3x uint16: components]
    */
    void WriteCompressedQuat(const Math::Quaternion& q) {
        // Ensure positive w hemisphere (q and -q represent the same rotation)
        Math::Quaternion v = q;
        if (v.w < 0.0f) { v.x = -v.x; v.y = -v.y; v.z = -v.z; v.w = -v.w; }

        // Find largest component
        float components[4] = { v.x, v.y, v.z, v.w };
        uint8_t largestIndex = 0;
        float largestAbs = std::abs(components[0]);
        for (uint8_t i = 1; i < 4; i++) {
            float a = std::abs(components[i]);
            if (a > largestAbs) { largestAbs = a; largestIndex = i; }
        }

        // Collect the 3 smallest components
        // Each is in [-1/sqrt(2), 1/sqrt(2)] ≈ [-0.7071, 0.7071]
        float smallest[3];
        int si = 0;
        for (uint8_t i = 0; i < 4; i++) {
            if (i != largestIndex) smallest[si++] = components[i];
        }

        // Quantize to uint16: map [-0.7071, 0.7071] -> [0, 65535]
        constexpr float RANGE = 0.70710678118f; // 1/sqrt(2)
        auto Quantize = [](float val, float range) -> uint16_t {
            float normalized = (val + range) / (2.0f * range); // [0, 1]
            if (normalized < 0.0f) normalized = 0.0f;
            if (normalized > 1.0f) normalized = 1.0f;
            return static_cast<uint16_t>(normalized * 65535.0f + 0.5f);
        };

        Write(largestIndex);                    // 1 byte
        Write(Quantize(smallest[0], RANGE));    // 2 bytes
        Write(Quantize(smallest[1], RANGE));    // 2 bytes
        Write(Quantize(smallest[2], RANGE));    // 2 bytes
    }

    /**
    * Generic write
    * Fallback for any trivially_copyable type not covered above.
    * This is what makes Rep<SomeCustomStruct, Scope::All> work
    * without adding explicit overloads.
    *
    * SFINAE ensures this doesn't compete with the typed overloads.
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

    // Construct from a vector (non-owning vector must outlive reader)
    explicit BitReader(const std::vector<uint8_t>& buffer)
        : _data(buffer.data()), _size(buffer.size()), _offset(0) {}

    // Construct from span
    explicit BitReader(std::span<const uint8_t> buffer)
        : _data(buffer.data()), _size(buffer.size()), _offset(0) {}

    //  Typed reads
    void Read(bool& v) {
        uint8_t raw = 0;
        Consume(&raw, 1);
        v = raw != 0;
    }

    void Read(uint8_t& v) {
        Consume(&v, 1);
    }

    void Read(int8_t& v) {
        Consume(&v, 1);
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

    void Read(Math::Quaternion& v) {
        Read(v.x);
        Read(v.y);
        Read(v.z);
        Read(v.w);
    }
    void ReadCompressedQuat(Math::Quaternion& q) {
        uint8_t largestIndex;
        uint16_t a, b, c;
        Read(largestIndex);
        if (largestIndex > 3) {
            _error = true;
            q = Math::Quaternion{0, 0, 0, 1};
            return;
        }
        Read(a);
        Read(b);
        Read(c);

        // Dequantize: [0, 65535] -> [-0.7071, 0.7071]
        constexpr float RANGE = 0.70710678118f;
        auto Dequantize = [](uint16_t val, float range) -> float {
            float normalized = static_cast<float>(val) / 65535.0f; // [0, 1]
            return normalized * 2.0f * range - range;
        };

        float smallest[3] = {
            Dequantize(a, RANGE),
            Dequantize(b, RANGE),
            Dequantize(c, RANGE)
        };

        // Reconstruct the dropped component:
        // |q| = 1, so dropped = sqrt(1 - a² - b² - c²)
        float sumSq = smallest[0] * smallest[0]
                     + smallest[1] * smallest[1]
                     + smallest[2] * smallest[2];
        float dropped = (sumSq < 1.0f) ? std::sqrt(1.0f - sumSq) : 0.0f;

        // Rebuild the 4 components
        float components[4];
        int si = 0;
        for (int i = 0; i < 4; i++) {
            if (i == largestIndex) {
                components[i] = dropped;
            } else {
                components[i] = smallest[si++];
            }
        }

        q.x = components[0];
        q.y = components[1];
        q.z = components[2];
        q.w = components[3];
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
        if (!CanRead(bytes)) {
            _error = true;
            return;
        }
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