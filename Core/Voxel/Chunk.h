//
// Created by Marvin on 04/02/2026.
//

#ifndef VOXTAU_CHUNK_H
#define VOXTAU_CHUNK_H
#include <functional>
#include "Block.h"
#include "Renderer/RenderApi/IRendererApi.h"
#include "Renderer/Shaders/Shader.h"
#include "Resources/BlockRegistry.h"
#include "Resources/Mesh.h"
#include "Resources/VoxelMesh.h"

// Callback type for querying neighbor blocks during mesh generation
// Parameters: worldX, worldY, worldZ -> returns block type (0 = air/not loaded)
using NeighborQueryFunc = std::function<uint8_t(int, int, int)>;

class Chunk
{
public:
    static constexpr uint32_t CHUNK_SIZE = 32;
    static constexpr uint32_t VOXELS_PER_CHUNK = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;
    static constexpr uint32_t MAX_FACES_PER_CHUNK = 50000;

    Chunk(const Math::Vector3& worldPosition);
    ~Chunk();

    // Voxel access (flat array)
    void SetBlock(int x, int y, int z, uint8_t type);
    uint8_t GetBlock(int x, int y, int z) const;
    bool IsSolid(int x, int y, int z) const;

    // Mesh generation
    // neighborQuery: optional callback to check blocks in neighboring chunks (world coords)
    void GenerateMesh(const BlockRegistry* blockRegistry, NeighborQueryFunc neighborQuery = nullptr);
    void GenerateMeshLOD(const BlockRegistry* blockRegistry, int lodLevel);

    void Update(float dt);
    void Render(IRendererApi* pRenderer, Shader* vertexShader);

    bool IsEmpty() const { return _solidBlockCount == 0; }
    void IncreaseSolidBlockCount() { _solidBlockCount++; }
    void DecreaseSolidBlockCount() { _solidBlockCount--; }

    // State
    bool IsDirty() const { return isDirty; }

    Vector3 GetWorldPosition() { return worldPosition; }
    uint32_t GetSolidBlockCount() const { return _solidBlockCount; }

private: // The blocks data
    Math::Vector3 worldPosition;

    uint8_t _blocks[CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE];
    uint32_t _solidBlockCount = 0;


    VoxelMesh* mesh;
    bool isDirty;

    // Internal helpers
    int GetIndex(int x, int y, int z) const;
    bool IsInBounds(int x, int y, int z) const;

    void AddFace(
        int x, int y, int z,
        int face,
        uint8_t blockType,
        const BlockRegistry* blockRegistry,
        std::vector<VoxelVertex>& vertices,
        std::vector<uint32_t>& indices
    );

    // Check if neighbor at local coords is solid
    // Uses neighborQuery for out-of-bounds positions (cross-chunk)
    bool IsNeighborSolid(int localX, int localY, int localZ, int face, const NeighborQueryFunc& neighborQuery);
};


#endif //VOXTAU_CHUNK_H