//
// Created by Marvin on 01/02/2026.
//

#ifndef VOXTAU_CHUNKMANAGER_H
#define VOXTAU_CHUNKMANAGER_H
#include <unordered_map>

#include "Chunk.h"
#include "Core/Math/Vector3.h"

class BlockRegistry;
class IChunkGenerator;

struct Vector3Hash {
    std::size_t operator()(const Math::Vector3& v) const {
        return std::hash<int>()(int(v.x)) ^
               std::hash<int>()(int(v.y)) << 1 ^
               std::hash<int>()(int(v.z)) << 2;
    }
};

struct Vector3Equal {
    bool operator()(const Math::Vector3& a, const Math::Vector3& b) const {
        return int(a.x) == int(b.x) &&
               int(a.y) == int(b.y) &&
               int(a.z) == int(b.z);
    }
};

class ChunkManager {
    static constexpr int MAX_LOD_LEVELS = 3;  // LOD 0 = full, LOD 1 = 2:1, LOD 2 = 4:1

private:
    std::unordered_map<Math::Vector3, Chunk*, Vector3Hash, Vector3Equal> chunks;
    const BlockRegistry* blockRegistry = nullptr;
    const IChunkGenerator* chunkGenerator = nullptr;

    // Convert world coords to chunk coords
    [[nodiscard]] Math::Vector3 WorldToChunkCoords(int x, int y, int z) const;

    // Convert world coords to local chunk coords (0-31)
    void WorldToLocalCoords(int x, int y, int z, int& localX, int& localY, int& localZ) const;

public:
    ChunkManager();
    ~ChunkManager();

    // Configuration
    void SetBlockRegistry(const BlockRegistry* registry) { blockRegistry = registry; }
    void SetChunkGenerator(const IChunkGenerator* generator) { chunkGenerator = generator; }

    // Chunk management
    void AddChunk(const Math::Vector3& chunkPos, Chunk* chunk);
    Chunk* GetChunk(const Math::Vector3& chunkPos);
    Chunk* GetChunk(int chunkX, int chunkY, int chunkZ);
    void RemoveChunk(const Math::Vector3& chunkPos);

    // Block access across chunk boundaries (world coordinates)
    uint8_t GetBlockAt(int worldX, int worldY, int worldZ) const;
    void SetBlockAt(int worldX, int worldY, int worldZ, uint8_t blockType);
    bool IsSolidAt(int worldX, int worldY, int worldZ) const;

    // Chunk generation and meshing
    Chunk* CreateChunk(int chunkX, int chunkY, int chunkZ);
    void GenerateChunkMesh(Chunk* chunk);
    void GenerateChunkMesh(int chunkX, int chunkY, int chunkZ);

    void OnInit();
    void Update();

    // Rebuild meshes for chunk and its neighbors (call after modifying blocks)
    void RebuildChunkAndNeighbors(int chunkX, int chunkY, int chunkZ);

    // Create neighbor query function for a specific chunk
    NeighborQueryFunc CreateNeighborQuery() const;

    // Get all chunks (for rendering)
    const std::unordered_map<Math::Vector3, Chunk*, Vector3Hash, Vector3Equal>& GetChunks() const {
        return chunks;
    }
};


#endif //VOXTAU_CHUNKMANAGER_H