//
// Created by Marvin on 28/01/2026.
//

#define NOMINMAX
#include "Mesh.h"
#include "Renderer/RenderApi/IRendererApi.h"
#include "Renderer/RenderApi/DirectX11/DirectX11Mesh.h"
#include <limits>
#include <algorithm>

// Static layout description for standard Vertex
static const VertexLayoutElement s_StandardVertexLayout[] = {
    { "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0  },
    { "NORMAL",     0, DXGI_FORMAT_R32G32B32_FLOAT,    12 },
    { "TEXCOORD",   0, DXGI_FORMAT_R32G32_FLOAT,       24 },
    { "COLOR",      0, DXGI_FORMAT_R32G32B32A32_FLOAT, 32 },
    { "TANGENT",    0, DXGI_FORMAT_R32G32B32_FLOAT,    48 },
    { "LIGHT",      0, DXGI_FORMAT_R32_FLOAT,          60 },
    { "TEXINDEX",   0, DXGI_FORMAT_R32_UINT,           64 },
    { "UVROTATION", 0, DXGI_FORMAT_R32_UINT,           68 }
};
static const size_t s_StandardVertexLayoutCount = sizeof(s_StandardVertexLayout) / sizeof(s_StandardVertexLayout[0]);


Mesh::Mesh(const std::string& name)
    : name(name)
    , gpuMeshHandle(nullptr)
    , isDirty(false)
    , topology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
{
}

Mesh::~Mesh() {
    // GPU handle is managed by renderer
    // High-level mesh doesn't delete it directly
}

// DATA MANAGEMENT
void Mesh::SetData(const std::vector<Vertex>& verts, const std::vector<uint32_t>& inds) {
    vertices = verts;
    indices = inds;
    isDirty = true;
    CalculateBounds();
}

void Mesh::SetVertices(const std::vector<Vertex>& verts) {
    vertices = verts;
    isDirty = true;
    CalculateBounds();
}

void Mesh::SetIndices(const std::vector<uint32_t>& inds) {
    indices = inds;
    isDirty = true;
}

// GETTERS

const std::vector<Vertex>& Mesh::GetVertices() const {
    return vertices;
}

const std::vector<uint32_t>& Mesh::GetIndices() const {
    return indices;
}

// VIRTUAL INTERFACE (base class implementations for standard Vertex)

const void* Mesh::GetRawVertexData() const {
    return vertices.data();
}

const uint32_t* Mesh::GetRawIndexData() const {
    return indices.data();
}

size_t Mesh::GetVertexStride() const {
    return sizeof(Vertex);
}

size_t Mesh::GetVertexCount() const {
    return vertices.size();
}

size_t Mesh::GetIndexCount() const {
    return indices.size();
}

const VertexLayoutElement* Mesh::GetInputLayoutDesc(size_t& outCount) const {
    outCount = s_StandardVertexLayoutCount;
    return s_StandardVertexLayout;
}

void Mesh::ReleaseCPUData() {
    vertices.clear();
    vertices.shrink_to_fit();
    indices.clear();
    indices.shrink_to_fit();
}

// GPU HANDLE

void* Mesh::GetGPUHandle() const {
    return gpuMeshHandle;
}

void Mesh::SetGPUHandle(void* handle) {
    gpuMeshHandle = handle;
}

// METADATA

const AABB& Mesh::GetBounds() const {
    return bounds;
}

void Mesh::CalculateBounds() {
    if (vertices.empty()) {
        bounds = AABB();
        return;
    }

    Math::Vector3 min(std::numeric_limits<float>::max());
    Math::Vector3 max(std::numeric_limits<float>::lowest());

    for (const auto& vertex : vertices) {
        min.x = std::min(min.x, vertex.position.x);
        min.y = std::min(min.y, vertex.position.y);
        min.z = std::min(min.z, vertex.position.z);

        max.x = std::max(max.x, vertex.position.x);
        max.y = std::max(max.y, vertex.position.y);
        max.z = std::max(max.z, vertex.position.z);
    }

    bounds = AABB(min, max);
}

bool Mesh::IsDirty() const {
    return isDirty;
}

void Mesh::MarkDirty() {
    isDirty = true;
}

void Mesh::ClearDirty() {
    isDirty = false;
}

const std::string& Mesh::GetName() const {
    return name;
}

void Mesh::SetTopology(D3D11_PRIMITIVE_TOPOLOGY topo) {
    topology = topo;
}

D3D11_PRIMITIVE_TOPOLOGY Mesh::GetTopology() const {
    return topology;
}

void Mesh::Bind(IRendererApi* renderer) {
    if (!gpuMeshHandle || !renderer) return;

    DirectX11Mesh* d3dMesh = static_cast<DirectX11Mesh*>(gpuMeshHandle);
    d3dMesh->Bind(renderer->GetContext());
}