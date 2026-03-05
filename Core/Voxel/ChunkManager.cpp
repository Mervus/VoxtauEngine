//
// Created by Marvin on 01/02/2026.
//

#include "ChunkManager.h"
#include <cmath>

#include "VoxelType.h"
#include "IChunkGenerator.h"

ChunkManager::ChunkManager() {
}

ChunkManager::~ChunkManager() {
    // Clean up all chunks
    for (auto& pair : chunks) {
        delete pair.second;
    }
    chunks.clear();
}

Math::Vector3 ChunkManager::WorldToChunkCoords(int x, int y, int z) const {
    int chunkX = (int)std::floor((float)x / Chunk::CHUNK_SIZE);
    int chunkY = (int)std::floor((float)y / Chunk::CHUNK_SIZE);
    int chunkZ = (int)std::floor((float)z / Chunk::CHUNK_SIZE);
    return Math::Vector3((float)chunkX, (float)chunkY, (float)chunkZ);
}

void ChunkManager::WorldToLocalCoords(int x, int y, int z, int& localX, int& localY, int& localZ) const {
    // Use floor division for correct negative handling
    int chunkX = (int)std::floor((float)x / Chunk::CHUNK_SIZE);
    int chunkY = (int)std::floor((float)y / Chunk::CHUNK_SIZE);
    int chunkZ = (int)std::floor((float)z / Chunk::CHUNK_SIZE);

    localX = x - chunkX * Chunk::CHUNK_SIZE;
    localY = y - chunkY * Chunk::CHUNK_SIZE;
    localZ = z - chunkZ * Chunk::CHUNK_SIZE;
}

void ChunkManager::AddChunk(const Math::Vector3& chunkPos, Chunk* chunk) {
    chunks[chunkPos] = chunk;
}

Chunk* ChunkManager::GetChunk(const Math::Vector3& chunkPos) {
    auto it = chunks.find(chunkPos);
    if (it != chunks.end()) {
        return it->second;
    }
    return nullptr;
}

Chunk* ChunkManager::GetChunk(int chunkX, int chunkY, int chunkZ) {
    return GetChunk(Math::Vector3((float)chunkX, (float)chunkY, (float)chunkZ));
}

void ChunkManager::RemoveChunk(const Math::Vector3& chunkPos) {
    auto it = chunks.find(chunkPos);
    if (it != chunks.end()) {
        delete it->second;
        chunks.erase(it);
    }
}

uint8_t ChunkManager::GetBlockAt(int worldX, int worldY, int worldZ) const {
    Math::Vector3 chunkPos = WorldToChunkCoords(worldX, worldY, worldZ);

    auto it = chunks.find(chunkPos);
    if (it == chunks.end()) {
        return 0;  // No chunk loaded = air
    }

    int localX, localY, localZ;
    WorldToLocalCoords(worldX, worldY, worldZ, localX, localY, localZ);

    return it->second->GetBlock(localX, localY, localZ);
}

void ChunkManager::SetBlockAt(int worldX, int worldY, int worldZ, uint8_t blockType) {
    Math::Vector3 chunkPos = WorldToChunkCoords(worldX, worldY, worldZ);

    auto it = chunks.find(chunkPos);
    if (it == chunks.end()) {
        if (blockType == 0) return; // No need to create a chunk just to place air

        auto* chunk = new Chunk(chunkPos);
        AddChunk(chunkPos, chunk);
        it = chunks.find(chunkPos);
    }

    int localX, localY, localZ;
    WorldToLocalCoords(worldX, worldY, worldZ, localX, localY, localZ);

    it->second->SetBlock(localX, localY, localZ, blockType);
}

bool ChunkManager::IsSolidAt(int worldX, int worldY, int worldZ) const {
    return GetBlockAt(worldX, worldY, worldZ) != 0;
}

Chunk* ChunkManager::CreateChunk(int chunkX, int chunkY, int chunkZ) {
    Math::Vector3 chunkPos((float)chunkX, (float)chunkY, (float)chunkZ);

    // Check if chunk already exists
    if (GetChunk(chunkPos) != nullptr) {
        return GetChunk(chunkPos);
    }

    // Create new chunk
    auto* chunk = new Chunk(chunkPos);

    // Generate terrain if generator is set
    if (chunkGenerator) {
        chunkGenerator->GenerateChunk(chunk);
    }

    if (!chunk->IsEmpty())
    {
        AddChunk(chunkPos, chunk);
        return chunk;
    }

    delete chunk;
    return nullptr;
}

void ChunkManager::GenerateChunkMesh(Chunk* chunk) {
    assert(chunk);
    assert(blockRegistry);
    if (!chunk || !blockRegistry) return;

    // Create neighbor query that captures this ChunkManager
    auto neighborQuery = CreateNeighborQuery();
    chunk->GenerateMesh(blockRegistry, neighborQuery);
}

void ChunkManager::GenerateChunkMesh(int chunkX, int chunkY, int chunkZ) {
    Chunk* chunk = GetChunk(chunkX, chunkY, chunkZ);
    if (chunk) {
        GenerateChunkMesh(chunk);
    }
}

void ChunkManager::RebuildChunkAndNeighbors(int chunkX, int chunkY, int chunkZ) {
    // Rebuild the target chunk
    GenerateChunkMesh(chunkX, chunkY, chunkZ);

    // Rebuild all 6 neighbors (their boundary faces may have changed)
    GenerateChunkMesh(chunkX - 1, chunkY, chunkZ);
    GenerateChunkMesh(chunkX + 1, chunkY, chunkZ);
    GenerateChunkMesh(chunkX, chunkY - 1, chunkZ);
    GenerateChunkMesh(chunkX, chunkY + 1, chunkZ);
    GenerateChunkMesh(chunkX, chunkY, chunkZ - 1);
    GenerateChunkMesh(chunkX, chunkY, chunkZ + 1);
}

NeighborQueryFunc ChunkManager::CreateNeighborQuery() const {
    return [this](int worldX, int worldY, int worldZ) -> uint8_t {
        return this->GetBlockAt(worldX, worldY, worldZ);
    };
}


