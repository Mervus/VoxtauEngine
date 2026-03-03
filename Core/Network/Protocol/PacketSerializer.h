//
// Created by Marvin on 27/02/2026.
//

#ifndef VOXTAU_PACKETSERIALIZER_H
#define VOXTAU_PACKETSERIALIZER_H
#pragma once

#include <vector>
#include <cstdint>
#include <cstring>
#include "PacketTypes.h"
#include "ServerSnapshot.h"
#include "PlayerInputState.h"
#include "Core/Entity/EntityID.h"
#include "Core/Math/Vector3.h"
#include "Core/Network/Replication/EntityReplicator.h"
#include "Core/Voxel/Chunk.h"

// Simple binary serializer. Writes/reads raw bytes.
// No endian handling — both client and server are the same platform.
// Upgrade to a proper bitstream later if bandwidth matters.

class PacketSerializer {
public:

    //  Write helpers 
    static void WriteU8(std::vector<uint8_t>& buf, uint8_t v) {
        buf.push_back(v);
    }

    static void WriteU16(std::vector<uint8_t>& buf, uint16_t v) {
        buf.resize(buf.size() + 2);
        std::memcpy(buf.data() + buf.size() - 2, &v, 2);
    }

    static void WriteU32(std::vector<uint8_t>& buf, uint32_t v) {
        buf.resize(buf.size() + 4);
        std::memcpy(buf.data() + buf.size() - 4, &v, 4);
    }

    static void WriteFloat(std::vector<uint8_t>& buf, float v) {
        buf.resize(buf.size() + 4);
        std::memcpy(buf.data() + buf.size() - 4, &v, 4);
    }

    static void WriteVec3(std::vector<uint8_t>& buf, const Math::Vector3& v) {
        WriteFloat(buf, v.x);
        WriteFloat(buf, v.y);
        WriteFloat(buf, v.z);
    }

    static void WriteBool(std::vector<uint8_t>& buf, bool v) {
        buf.push_back(v ? 1 : 0);
    }

    //  Read helpers 

    static uint8_t ReadU8(const uint8_t* data, size_t& offset) {
        return data[offset++];
    }

    static uint16_t ReadU16(const uint8_t* data, size_t& offset) {
        uint16_t v;
        std::memcpy(&v, data + offset, 2);
        offset += 2;
        return v;
    }

    static uint32_t ReadU32(const uint8_t* data, size_t& offset) {
        uint32_t v;
        std::memcpy(&v, data + offset, 4);
        offset += 4;
        return v;
    }

    static float ReadFloat(const uint8_t* data, size_t& offset) {
        float v;
        std::memcpy(&v, data + offset, 4);
        offset += 4;
        return v;
    }

    static Math::Vector3 ReadVec3(const uint8_t* data, size_t& offset) {
        float x = ReadFloat(data, offset);
        float y = ReadFloat(data, offset);
        float z = ReadFloat(data, offset);
        return Math::Vector3(x, y, z);
    }

    static bool ReadBool(const uint8_t* data, size_t& offset) {
        return data[offset++] != 0;
    }

    //  ServerSnapshot 

    static std::vector<uint8_t> SerializeSnapshot(const ServerSnapshot& snapshot) {
        std::vector<uint8_t> buf;
        buf.reserve(256);

        WriteU8(buf, static_cast<uint8_t>(PacketType::Snapshot));

        // Header
        WriteU32(buf, snapshot.serverTick);
        WriteU32(buf, snapshot.lastProcessedInput);
        WriteU32(buf, snapshot.worldStateVersion);

        // Local player state
        WriteVec3(buf, snapshot.playerPosition);
        WriteVec3(buf, snapshot.playerVelocity);
        WriteBool(buf, snapshot.playerOnGround);

        // Entity count
        WriteU16(buf, static_cast<uint16_t>(snapshot.entities.size()));

        // Each entity — only write dirty fields
        for (const auto& e : snapshot.entities) {
            WriteU32(buf, e.id.Get());
            WriteU8(buf, static_cast<uint8_t>(e.type));
            WriteU32(buf, e.dirtyFlags);

            if (e.dirtyFlags & DirtyFlag::Position)      WriteVec3(buf, e.position);
            if (e.dirtyFlags & DirtyFlag::Velocity)      WriteVec3(buf, e.velocity);
            if (e.dirtyFlags & DirtyFlag::Yaw)           WriteFloat(buf, e.yaw);
            if (e.dirtyFlags & DirtyFlag::Health)        WriteFloat(buf, e.health);
            if (e.dirtyFlags & DirtyFlag::MaxHealth)     WriteFloat(buf, e.maxHealth);
            if (e.dirtyFlags & DirtyFlag::IsDead)        WriteBool(buf, e.isDead);
            if (e.dirtyFlags & DirtyFlag::MovementState) WriteU8(buf, e.movementState);
            if (e.dirtyFlags & DirtyFlag::AnimationId)   WriteU8(buf, e.animationId);
        }

        return buf;
    }

    static bool DeserializeSnapshot(const std::vector<uint8_t>& data, ServerSnapshot& out) {
        if (data.size() < 2) return false;

        size_t offset = 1; // skip packet type byte

        out.serverTick          = ReadU32(data.data(), offset);
        out.lastProcessedInput  = ReadU32(data.data(), offset);
        out.worldStateVersion   = ReadU32(data.data(), offset);

        out.playerPosition  = ReadVec3(data.data(), offset);
        out.playerVelocity  = ReadVec3(data.data(), offset);
        out.playerOnGround  = ReadBool(data.data(), offset);

        uint16_t entityCount = ReadU16(data.data(), offset);
        out.entities.resize(entityCount);

        for (uint16_t i = 0; i < entityCount; i++) {
            auto& e = out.entities[i];

            e.id         = EntityID(ReadU32(data.data(), offset));
            e.type       = static_cast<EntityType>(ReadU8(data.data(), offset));
            e.dirtyFlags = ReadU32(data.data(), offset);

            if (e.dirtyFlags & DirtyFlag::Position)      e.position      = ReadVec3(data.data(), offset);
            if (e.dirtyFlags & DirtyFlag::Velocity)      e.velocity      = ReadVec3(data.data(), offset);
            if (e.dirtyFlags & DirtyFlag::Yaw)           e.yaw           = ReadFloat(data.data(), offset);
            if (e.dirtyFlags & DirtyFlag::Health)        e.health        = ReadFloat(data.data(), offset);
            if (e.dirtyFlags & DirtyFlag::MaxHealth)     e.maxHealth     = ReadFloat(data.data(), offset);
            if (e.dirtyFlags & DirtyFlag::IsDead)        e.isDead        = ReadBool(data.data(), offset);
            if (e.dirtyFlags & DirtyFlag::MovementState) e.movementState = ReadU8(data.data(), offset);
            if (e.dirtyFlags & DirtyFlag::AnimationId)   e.animationId   = ReadU8(data.data(), offset);
        }

        return true;
    }

    //  PlayerInputState 

    static std::vector<uint8_t> SerializeInput(const PlayerInputState& input) {
        std::vector<uint8_t> buf;
        buf.reserve(32);

        WriteU8(buf, static_cast<uint8_t>(PacketType::ClientInput));
        WriteU32(buf, input.tick);
        WriteFloat(buf, input.moveX);
        WriteFloat(buf, input.moveY);
        WriteFloat(buf, input.moveZ);
        WriteFloat(buf, input.yaw);
        WriteFloat(buf, input.pitch);
        WriteU16(buf, input.buttons);

        return buf;
    }

    // Serialize multiple inputs for redundancy (handles packet loss)
    static std::vector<uint8_t> SerializeInputs(const PlayerInputState* inputs, size_t count) {
        std::vector<uint8_t> buf;
        buf.reserve(8 + count * 26);

        WriteU8(buf, static_cast<uint8_t>(PacketType::ClientInput));
        WriteU8(buf, static_cast<uint8_t>(count));

        for (size_t i = 0; i < count; i++) {
            WriteU32(buf, inputs[i].tick);
            WriteFloat(buf, inputs[i].moveX);
            WriteFloat(buf, inputs[i].moveY);
            WriteFloat(buf, inputs[i].moveZ);
            WriteFloat(buf, inputs[i].yaw);
            WriteFloat(buf, inputs[i].pitch);
            WriteU16(buf, inputs[i].buttons);
        }

        return buf;
    }

    static bool DeserializeInput(const std::vector<uint8_t>& data, PlayerInputState& out) {
        if (data.size() < 1 + 26) return false;

        size_t offset = 1; // skip packet type
        out.tick    = ReadU32(data.data(), offset);
        out.moveX   = ReadFloat(data.data(), offset);
        out.moveY   = ReadFloat(data.data(), offset);
        out.moveZ   = ReadFloat(data.data(), offset);
        out.yaw     = ReadFloat(data.data(), offset);
        out.pitch   = ReadFloat(data.data(), offset);
        out.buttons = ReadU16(data.data(), offset);

        return true;
    }

    // Deserialize multiple redundant inputs
    static bool DeserializeInputs(const std::vector<uint8_t>& data,
                                   std::vector<PlayerInputState>& out) {
        if (data.size() < 2) return false;

        size_t offset = 1; // skip packet type
        uint8_t count = ReadU8(data.data(), offset);
        out.resize(count);

        for (uint8_t i = 0; i < count; i++) {
            if (offset + 26 > data.size()) return false;
            out[i].tick    = ReadU32(data.data(), offset);
            out[i].moveX   = ReadFloat(data.data(), offset);
            out[i].moveY   = ReadFloat(data.data(), offset);
            out[i].moveZ   = ReadFloat(data.data(), offset);
            out[i].yaw     = ReadFloat(data.data(), offset);
            out[i].pitch   = ReadFloat(data.data(), offset);
            out[i].buttons = ReadU16(data.data(), offset);
        }

        return true;
    }

    //  Chunk Data 
    //
    // Format:
    //   [PacketType::ChunkData] 1 byte
    //   [chunkX]  4 bytes (int32)
    //   [chunkY]  4 bytes (int32)
    //   [chunkZ]  4 bytes (int32)
    //   [isEmpty] 1 byte (if true, no block data follows)
    //   [blocks]  32*32*32 = 32768 bytes (raw block types)
    //
    // TODO: RLE or palette compression. A typical chunk with mostly air
    // compresses extremely well. For now, raw is simple and correct.

    static std::vector<uint8_t> SerializeChunk(const Chunk* chunk) {
        std::vector<uint8_t> buf;

        Math::Vector3 pos = const_cast<Chunk*>(chunk)->GetWorldPosition();
        int cx = static_cast<int>(pos.x);
        int cy = static_cast<int>(pos.y);
        int cz = static_cast<int>(pos.z);

        WriteU8(buf, static_cast<uint8_t>(PacketType::ChunkData));

        // Chunk coordinates (not world position — chunk grid coords)
        int32_t coords[3] = { cx, cy, cz };
        buf.resize(buf.size() + 12);
        std::memcpy(buf.data() + buf.size() - 12, coords, 12);

        bool empty = chunk->IsEmpty();
        WriteBool(buf, empty);

        if (!empty) {
            // Write raw block data
            constexpr size_t BLOCK_COUNT = Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE;
            size_t startPos = buf.size();
            buf.resize(buf.size() + BLOCK_COUNT);

            // Read each block individually (blocks array is private)
            for (uint32_t y = 0; y < Chunk::CHUNK_SIZE; y++) {
                for (uint32_t z = 0; z < Chunk::CHUNK_SIZE; z++) {
                    for (uint32_t x = 0; x < Chunk::CHUNK_SIZE; x++) {
                        buf[startPos + x + z * Chunk::CHUNK_SIZE + y * Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE]
                            = chunk->GetBlock(x, y, z);
                    }
                }
            }
        }

        return buf;
    }

    struct ChunkPacketData {
        int chunkX, chunkY, chunkZ;
        bool isEmpty;
        uint8_t blocks[Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE];
    };

    static bool DeserializeChunk(const std::vector<uint8_t>& data, ChunkPacketData& out) {
        if (data.size() < 14) return false; // type + 3 ints + bool

        size_t offset = 1; // skip packet type

        int32_t coords[3];
        std::memcpy(coords, data.data() + offset, 12);
        offset += 12;

        out.chunkX = coords[0];
        out.chunkY = coords[1];
        out.chunkZ = coords[2];

        out.isEmpty = ReadBool(data.data(), offset);

        if (!out.isEmpty) {
            constexpr size_t BLOCK_COUNT = Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE;
            if (data.size() < offset + BLOCK_COUNT) return false;

            std::memcpy(out.blocks, data.data() + offset, BLOCK_COUNT);
        } else {
            std::memset(out.blocks, 0, sizeof(out.blocks));
        }

        return true;
    }

    // Player Spawn
    static std::vector<uint8_t> SerializePlayerSpawn(EntityID playerId, const Math::Vector3& spawnPos) {
        std::vector<uint8_t> buf;
        buf.reserve(20);

        WriteU8(buf, static_cast<uint8_t>(PacketType::PlayerSpawn));
        WriteU32(buf, playerId.Get());
        WriteVec3(buf, spawnPos);

        return buf;
    }

    struct PlayerSpawnData {
        EntityID playerId;
        Math::Vector3 spawnPos;
    };

    static bool DeserializePlayerSpawn(const std::vector<uint8_t>& data, PlayerSpawnData& out) {
        if (data.size() < 1 + 4 + 12) return false;

        size_t offset = 1;
        out.playerId = EntityID(ReadU32(data.data(), offset));
        out.spawnPos = ReadVec3(data.data(), offset);

        return true;
    }


    static std::vector<uint8_t>  SerializeEntitySpawn(EntityID entityId, EntityType type, const Math::Vector3& spawnPos) {
        std::vector<uint8_t> buf;

        WriteU8(buf, static_cast<uint8_t>(PacketType::EntitySpawn));
        WriteU32(buf, entityId.Get());
        WriteU8(buf, type);
        WriteVec3(buf, spawnPos);

        return buf;
    }

    struct EntitySpawnData
    {
        EntityID id;
        EntityType type = EntityType::None;
        Math::Vector3 position;
    };

    static bool DeserializeEntitySpawn(const std::vector<uint8_t>& data, EntitySpawnData& out) {
        if (data.size() < 1 + 4 + 1 + 12) return false;

        size_t offset = 1;
        out.id = EntityID(ReadU32(data.data(), offset));
        out.type     = static_cast<EntityType>(ReadU8(data.data(), offset));
        out.position = ReadVec3(data.data(), offset);

        return true;
    }
};

#endif //VOXTAU_PACKETSERIALIZER_H