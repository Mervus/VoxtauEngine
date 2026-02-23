//
// Created by Marvin on 14/02/2026.
//

#define NOMINMAX
#include "SkinnedMesh.h"
#include <limits>
#include <algorithm>

// Input layout for SkinnedVertex — must match the skinned vertex shader
static const VertexLayoutElement s_SkinnedVertexLayout[] = {
    { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0  },  // float3 position
    { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    12 },  // float3 normal
    { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       24 },  // float2 texCoord
    { "COLOR",       0, DXGI_FORMAT_R32G32B32A32_FLOAT, 32 },  // float4 color
    { "TANGENT",     0, DXGI_FORMAT_R32G32B32_FLOAT,    48 },  // float3 tangent
    { "BONEINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,  60 },  // uint4  boneIndices
    { "BONEWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 76 },  // float4 boneWeights
};
static const size_t s_SkinnedVertexLayoutCount =
    sizeof(s_SkinnedVertexLayout) / sizeof(s_SkinnedVertexLayout[0]);

SkinnedMesh::SkinnedMesh(const std::string& name)
    : Mesh(name)
{
}

SkinnedMesh::~SkinnedMesh() = default;

void SkinnedMesh::SetSkinnedData(const std::vector<SkinnedVertex>& verts,
                                  const std::vector<uint32_t>& inds) {
    skinnedVertices = verts;
    indices = inds;
    isDirty = true;
    CalculateBounds();
}

const void* SkinnedMesh::GetRawVertexData() const {
    return skinnedVertices.data();
}

const uint32_t* SkinnedMesh::GetRawIndexData() const {
    return indices.data();
}

size_t SkinnedMesh::GetVertexStride() const {
    return sizeof(SkinnedVertex);
}

size_t SkinnedMesh::GetVertexCount() const {
    return skinnedVertices.size();
}

size_t SkinnedMesh::GetIndexCount() const {
    return indices.size();
}

const VertexLayoutElement* SkinnedMesh::GetInputLayoutDesc(size_t& outCount) const {
    outCount = s_SkinnedVertexLayoutCount;
    return s_SkinnedVertexLayout;
}

void SkinnedMesh::ReleaseCPUData() {
    skinnedVertices.clear();
    skinnedVertices.shrink_to_fit();
    indices.clear();
    indices.shrink_to_fit();
}

void SkinnedMesh::CalculateBounds() {
    if (skinnedVertices.empty()) {
        bounds = AABB();
        return;
    }

    Vector3 minB(std::numeric_limits<float>::max(),
                 std::numeric_limits<float>::max(),
                 std::numeric_limits<float>::max());
    Vector3 maxB(std::numeric_limits<float>::lowest(),
                 std::numeric_limits<float>::lowest(),
                 std::numeric_limits<float>::lowest());

    for (const auto& v : skinnedVertices) {
        minB.x = (std::min)(minB.x, v.position.x);
        minB.y = (std::min)(minB.y, v.position.y);
        minB.z = (std::min)(minB.z, v.position.z);
        maxB.x = (std::max)(maxB.x, v.position.x);
        maxB.y = (std::max)(maxB.y, v.position.y);
        maxB.z = (std::max)(maxB.z, v.position.z);
    }

    bounds = AABB(minB, maxB);
}