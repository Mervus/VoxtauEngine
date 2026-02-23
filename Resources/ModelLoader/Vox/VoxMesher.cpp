//
// Created by Marvin on 10/02/2026.
//

#include "VoxMesher.h"
#include "Renderer/Vertex.h"
#include <cstring>
#include <vector>

#include "Resources/Mesh.h"

bool VoxMesher::IsSolid(const uint8_t* grid, uint32_t sx, uint32_t sy, uint32_t sz,
                        int x, int y, int z) {
    if (x < 0 || y < 0 || z < 0) return false;
    if (x >= (int)sx || y >= (int)sy || z >= (int)sz) return false;
    return grid[x + y * sx + z * sx * sy] != 0;
}

void VoxMesher::AddFace(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
                         const Math::Vector3& p0, const Math::Vector3& p1,
                         const Math::Vector3& p2, const Math::Vector3& p3,
                         const Math::Vector3& normal, const VoxColor& color) {
    uint32_t baseIndex = static_cast<uint32_t>(vertices.size());

    float r = color.r / 255.0f;
    float g = color.g / 255.0f;
    float b = color.b / 255.0f;
    float a = color.a / 255.0f;

    auto makeVert = [&](const Math::Vector3& pos, float u, float v) {
        Vertex vert;
        vert.position = pos;
        vert.normal = normal;
        vert.texCoord = Math::Vector2(u, v);
        vert.color = Math::Vector4(r, g, b, a);
        return vert;
    };

    vertices.push_back(makeVert(p0, 0.0f, 0.0f));
    vertices.push_back(makeVert(p1, 1.0f, 0.0f));
    vertices.push_back(makeVert(p2, 1.0f, 1.0f));
    vertices.push_back(makeVert(p3, 0.0f, 1.0f));

    // Two triangles: 0-1-2, 0-2-3
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 3);
}

Mesh* VoxMesher::CreateMesh(const VoxModel& model, const std::string& name,
                             bool centerOrigin, float scale) {
    uint32_t sx = model.sizeX;
    uint32_t sy = model.sizeY;
    uint32_t sz = model.sizeZ;

    if (sx == 0 || sy == 0 || sz == 0) return nullptr;

    // Build a 3D grid from the sparse voxel list
    // Grid stores color index (0 = empty)
    size_t gridSize = sx * sy * sz;
    std::vector<uint8_t> grid(gridSize, 0);

    for (const auto& v : model.voxels) {
        if (v.x < sx && v.y < sy && v.z < sz) {
            grid[v.x + v.y * sx + v.z * sx * sy] = v.colorIndex;
        }
    }

    // Origin offset for centering
    // MagicaVoxel is Z-up, engine is Y-up: MV x→x, MV z→y(up), MV y→z(depth)
    Math::Vector3 offset(0.0f, 0.0f, 0.0f);
    if (centerOrigin) {
        offset = Math::Vector3(
            -(float)sx * 0.5f * scale,
            -(float)sz * 0.5f * scale,
            -(float)sy * 0.5f * scale
        );
    }

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Reserve rough estimate
    vertices.reserve(model.voxels.size() * 8);
    indices.reserve(model.voxels.size() * 12);

    // Generate faces for each solid voxel
    // Coordinate remap: MV x→engine x, MV z→engine y (up), MV y→engine z (depth)
    for (uint32_t z = 0; z < sz; z++) {
        for (uint32_t y = 0; y < sy; y++) {
            for (uint32_t x = 0; x < sx; x++) {
                uint8_t colorIdx = grid[x + y * sx + z * sx * sy];
                if (colorIdx == 0) continue;

                VoxColor color = model.GetColor(colorIdx);

                // Engine coordinates: MV x→x, MV z→y(up), MV y→z(depth)
                float ex = (float)x * scale + offset.x;
                float ey = (float)z * scale + offset.y;
                float ez = (float)y * scale + offset.z;
                float s = scale;

                // +X face (MV) → +X face (engine)
                if (!IsSolid(grid.data(), sx, sy, sz, x + 1, y, z)) {
                    AddFace(vertices, indices,
                        Math::Vector3(ex + s, ey,     ez),
                        Math::Vector3(ex + s, ey,     ez + s),
                        Math::Vector3(ex + s, ey + s, ez + s),
                        Math::Vector3(ex + s, ey + s, ez),
                        Math::Vector3(1, 0, 0), color);
                }

                // -X face (MV) → -X face (engine)
                if (!IsSolid(grid.data(), sx, sy, sz, x - 1, y, z)) {
                    AddFace(vertices, indices,
                        Math::Vector3(ex, ey,     ez + s),
                        Math::Vector3(ex, ey,     ez),
                        Math::Vector3(ex, ey + s, ez),
                        Math::Vector3(ex, ey + s, ez + s),
                        Math::Vector3(-1, 0, 0), color);
                }

                // +Y face (MV) → +Z face (engine)
                if (!IsSolid(grid.data(), sx, sy, sz, x, y + 1, z)) {
                    AddFace(vertices, indices,
                        Math::Vector3(ex,     ey,     ez + s),
                        Math::Vector3(ex,     ey + s, ez + s),
                        Math::Vector3(ex + s, ey + s, ez + s),
                        Math::Vector3(ex + s, ey,     ez + s),
                        Math::Vector3(0, 0, 1), color);
                }

                // -Y face (MV) → -Z face (engine)
                if (!IsSolid(grid.data(), sx, sy, sz, x, y - 1, z)) {
                    AddFace(vertices, indices,
                        Math::Vector3(ex + s, ey,     ez),
                        Math::Vector3(ex + s, ey + s, ez),
                        Math::Vector3(ex,     ey + s, ez),
                        Math::Vector3(ex,     ey,     ez),
                        Math::Vector3(0, 0, -1), color);
                }

                // +Z face (MV) → +Y face (engine)
                if (!IsSolid(grid.data(), sx, sy, sz, x, y, z + 1)) {
                    AddFace(vertices, indices,
                        Math::Vector3(ex,     ey + s, ez),
                        Math::Vector3(ex + s, ey + s, ez),
                        Math::Vector3(ex + s, ey + s, ez + s),
                        Math::Vector3(ex,     ey + s, ez + s),
                        Math::Vector3(0, 1, 0), color);
                }

                // -Z face (MV) → -Y face (engine)
                if (!IsSolid(grid.data(), sx, sy, sz, x, y, z - 1)) {
                    AddFace(vertices, indices,
                        Math::Vector3(ex,     ey, ez + s),
                        Math::Vector3(ex + s, ey, ez + s),
                        Math::Vector3(ex + s, ey, ez),
                        Math::Vector3(ex,     ey, ez),
                        Math::Vector3(0, -1, 0), color);
                }
            }
        }
    }

    if (vertices.empty()) return nullptr;

    Mesh* mesh = new Mesh(name);
    mesh->SetData(vertices, indices);
    return mesh;
}