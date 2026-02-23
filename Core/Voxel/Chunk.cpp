//
// Created by Marvin on 04/02/2026.
//

#include "Chunk.h"

#include "Renderer/Vertex.h"
#include "Renderer/Debug/DebugLineRenderer.h"
#include "Resources/Mesh.h"
#include "Core/Math/Vector2.h"
#include "Core/Math/Vector4.h"

Chunk::Chunk(const Math::Vector3& worldPosition)
    : worldPosition(worldPosition)
    , mesh(nullptr)
    , isDirty(true)
{
    // Initialize all blocks to air
    memset(_blocks, 0, sizeof(_blocks));
}

Chunk::~Chunk() {
    // Octree nodes are in nodePool (std::vector), cleaned up automatically
    delete mesh;
}

int Chunk::GetIndex(int x, int y, int z) const {
    return x + z * CHUNK_SIZE + y * CHUNK_SIZE * CHUNK_SIZE;
}

bool Chunk::IsInBounds(int x, int y, int z) const {
    return x >= 0 && x < CHUNK_SIZE &&
           y >= 0 && y < CHUNK_SIZE &&
           z >= 0 && z < CHUNK_SIZE;
}

void Chunk::SetBlock(int x, int y, int z, uint8_t type) {
    if (!IsInBounds(x, y, z)) return;

    uint32_t idx = GetIndex(x, y, z);
    uint8_t old = _blocks[idx];

    if (old == type) return; // No change

    // Update solid count
    if (old == 0 && type != 0) _solidBlockCount++;
    else if (old != 0 && type == 0) _solidBlockCount--;

    _blocks[idx] = type;
    isDirty = true;
}

uint8_t Chunk::GetBlock(int x, int y, int z) const {
    if (!IsInBounds(x, y, z)) return 0;  // Air

    return _blocks[GetIndex(x, y, z)];
}

bool Chunk::IsSolid(int x, int y, int z) const {
    if (!IsInBounds(x, y, z)) return false;

    return _blocks[GetIndex(x, y, z)] != 0;
}

void Chunk::GenerateMesh(const BlockRegistry* blockRegistry, NeighborQueryFunc neighborQuery) {
    // Create mesh if needed
    if (!mesh) {
        mesh = new VoxelMesh("chunk_mesh");
    }

    std::vector<VoxelVertex> vertices;
    std::vector<uint32_t> indices;

    // Reserve approximate space
    vertices.reserve(1024);
    indices.reserve(2048);

    mesh->SetVoxelData(vertices, indices);
    isDirty = false;
}

bool Chunk::IsNeighborSolid(int localX, int localY, int localZ, int face, const NeighborQueryFunc& neighborQuery) {
    // Neighbor offsets for each face
    static const int offsets[6][3] = {
        {0, 0, 1},   // Front (+Z)
        {0, 0, -1},  // Back (-Z)
        {-1, 0, 0},  // Left (-X)
        {1, 0, 0},   // Right (+X)
        {0, 1, 0},   // Top (+Y)
        {0, -1, 0}   // Bottom (-Y)
    };

    int nx = localX + offsets[face][0];
    int ny = localY + offsets[face][1];
    int nz = localZ + offsets[face][2];

    // Check if neighbor is within this chunk
    if (IsInBounds(nx, ny, nz)) {
        return IsSolid(nx, ny, nz);
    }

    // Neighbor is in adjacent chunk - use query function if available
    if (neighborQuery) {
        // Convert to world coordinates
        int worldX = static_cast<int>(worldPosition.x) * CHUNK_SIZE + nx;
        int worldY = static_cast<int>(worldPosition.y) * CHUNK_SIZE + ny;
        int worldZ = static_cast<int>(worldPosition.z) * CHUNK_SIZE + nz;

        return neighborQuery(worldX, worldY, worldZ) != 0;
    }

    // No neighbor query = assume not solid (draw the face)
    return false;
}

void Chunk::AddFace(
    int x, int y, int z,
    int face,
    uint8_t blockType,
    const BlockRegistry* blockRegistry,
    std::vector<VoxelVertex>& vertices,
    std::vector<uint32_t>& indices)
{
    // Vertex offsets for each face (4 vertices per face, CW winding when viewed from outside)
    static const float faceVertices[6][4][3] = {
        // Front (+Z) - viewed from +Z looking at -Z
        {{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}},
        // Back (-Z) - viewed from -Z looking at +Z (reversed for CW)
    {{1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0}},
        // Left (-X) - viewed from -X looking at +X (reversed for CW)
        {{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}},
        // Right (+X)
        {{1, 0, 1}, {1, 0, 0}, {1, 1, 0}, {1, 1, 1}},
        // Top (+Y)
        {{0, 1, 1}, {1, 1, 1}, {1, 1, 0}, {0, 1, 0}},
        // Bottom (-Y)
        {{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}}
    };

    // UV coordinates
    static const float faceUVs[4][2] = {
        {0, 1}, {1, 1}, {1, 0}, {0, 0}
    };

    // Get texture index
    uint32_t texIndex = 0;
    if (blockRegistry) {
        texIndex = blockRegistry->GetTextureIndex(blockType, face);
    }

    uint32_t baseIndex = static_cast<uint32_t>(vertices.size());

    // Add 4 vertices
    for (int v = 0; v < 4; v++) {
        VoxelVertex vert;
        vert.Set(
            Vector3(
                worldPosition.x * CHUNK_SIZE + x + faceVertices[face][v][0],
                worldPosition.y * CHUNK_SIZE + y + faceVertices[face][v][1],
                worldPosition.z * CHUNK_SIZE + z + faceVertices[face][v][2]
            ),
            Math::Vector2(faceUVs[v][0], faceUVs[v][1]),
            static_cast<uint8_t>(face),   // faceIndex (0-5)
            texIndex,                      // textureIndex
            15,                            // lightLevel (full brightness)
            0                              // uvRotation
        );
        vertices.push_back(vert);
    }

    // Add 2 triangles (6 indices)
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 3);
}

void Chunk::GenerateMeshLOD(const BlockRegistry* blockRegistry, int lodLevel)
{
}

void Chunk::Update(float dt)
{
}

void Chunk::Render(IRendererApi* pRenderer, Shader* vertexShader)
{
    if (!mesh) return;
    if (mesh->GetVertexCount() == 0 && mesh->GetGPUHandle() == nullptr) return;

    // Create GPU mesh if needed
    if (!mesh->GetGPUHandle()) {
        bool success = pRenderer->CreateMesh(mesh);
        if (!success) {
            printf("ERROR: Failed to create GPU mesh!\n");
            return;
        }

        mesh->ReleaseCPUData();
    }

    // Create input layout if needed (requires vertex shader)
    if (vertexShader) {
        pRenderer->CreateMeshInputLayout(mesh, vertexShader);
    }

    // Draw the mesh
    pRenderer->Draw(mesh);
}