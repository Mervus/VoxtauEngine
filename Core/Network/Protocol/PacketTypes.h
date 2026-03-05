//
// Created by Marvin on 27/02/2026.
//

#ifndef VOXTAU_PACKETTYPES_H
#define VOXTAU_PACKETTYPES_H
#pragma once

#include <cstdint>

// First byte of every packet identifies its type.
// Server and client switch on this to route to the correct handler.
enum class PacketType : uint8_t {
    // Client → Server
    ClientInput         = 0x01,  // PlayerInputState (sent every tick, unreliable)
    BlockChangeRequest  = 0x02,  // Position + block type (reliable)
    ChatMessage         = 0x03,  // Text (reliable)
    ClientReady         = 0x04,  // Client finished loading, ready to receive snapshots

    // Server → Client
    Snapshot            = 0x10,  // ServerSnapshot (sent at network rate, unreliable)
    EntitySpawn         = 0x11,  // Full entity state for newly visible entity (reliable)
    EntityDespawn       = 0x12,  // EntityID leaving area of interest (reliable)
    BlockChangeConfirm  = 0x13,  // Server accepted/rejected block change (reliable)
    BlockChangeNotify   = 0x14,  // Another player changed a block near you (reliable)
    ChunkData           = 0x15,  // Chunk voxel data for streaming (reliable ordered)
    ChatBroadcast       = 0x16,  // Chat from another player (reliable)
    PlayerSpawn         = 0x17,

    // Bidirectional
    Ping                = 0xF0,  // RTT measurement
    Pong                = 0xF1,
    Disconnect          = 0xFF,  // Graceful disconnect
};
#endif //VOXTAU_PACKETTYPES_H