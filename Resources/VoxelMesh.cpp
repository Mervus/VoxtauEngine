//
// Created by Marvin on 05/02/2026.
//

#define NOMINMAX
#include "VoxelMesh.h"
#include <limits>
#include <algorithm>

// Input layout for VoxelVertex (24 bytes)
// Matches the voxel vertex shader input:
//   float3 position : POSITION  (offset 0)
//   float2 texCoord : TEXCOORD  (offset 12)
//   uint   packed   : PACKED    (offset 20)
static const VertexLayoutElement s_VoxelVertexLayout[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0  },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    12 },
    { "PACKED",   0, DXGI_FORMAT_R32_UINT,         20 }
};
static const size_t s_VoxelVertexLayoutCount = sizeof(s_VoxelVertexLayout) / sizeof(s_VoxelVertexLayout[0]);

VoxelMesh::VoxelMesh(const std::string& name)
    : Mesh(name)
{
}

VoxelMesh::~VoxelMesh() {
}

void VoxelMesh::SetVoxelData(const std::vector<VoxelVertex>& verts, const std::vector<uint32_t>& inds) {
    voxelVertices = verts;
    voxelIndices = inds;
    isDirty = true;
    CalculateBounds();
}

const std::vector<VoxelVertex>& VoxelMesh::GetVoxelVertices() const {
    return voxelVertices;
}

const std::vector<uint32_t>& VoxelMesh::GetVoxelIndices() const {
    return voxelIndices;
}

// ===== Virtual overrides =====

const void* VoxelMesh::GetRawVertexData() const {
    return voxelVertices.data();
}

const uint32_t* VoxelMesh::GetRawIndexData() const {
    return voxelIndices.data();
}

size_t VoxelMesh::GetVertexStride() const {
    return sizeof(VoxelVertex);
}

size_t VoxelMesh::GetVertexCount() const {
    return voxelVertices.size();
}

size_t VoxelMesh::GetIndexCount() const {
    return voxelIndices.size();
}

const VertexLayoutElement* VoxelMesh::GetInputLayoutDesc(size_t& outCount) const {
    outCount = s_VoxelVertexLayoutCount;
    return s_VoxelVertexLayout;
}

void VoxelMesh::ReleaseCPUData() {
    voxelVertices.clear();
    voxelVertices.shrink_to_fit();
    voxelIndices.clear();
    voxelIndices.shrink_to_fit();
}

void VoxelMesh::CalculateBounds() {
    if (voxelVertices.empty()) {
        bounds = AABB();
        return;
    }

    Math::Vector3 min(std::numeric_limits<float>::max());
    Math::Vector3 max(std::numeric_limits<float>::lowest());

    for (const auto& vert : voxelVertices) {
        min.x = std::min(min.x, vert.position.x);
        min.y = std::min(min.y, vert.position.y);
        min.z = std::min(min.z, vert.position.z);

        max.x = std::max(max.x, vert.position.x);
        max.y = std::max(max.y, vert.position.y);
        max.z = std::max(max.z, vert.position.z);
    }

    bounds = AABB(min, max);
}