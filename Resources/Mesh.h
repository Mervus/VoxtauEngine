//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_MESH_H
#define DIRECTX11_MESH_H
#pragma once

#include <EngineApi.h>
#include <vector>
#include <string>
#include <cstdint>
#include <d3d11.h>
#include "../Renderer/Vertex.h"
#include "../Core/Math/Vector3.h"


using Math::Vector3;

struct AABB {
    Vector3 minBounds;
    Vector3 maxBounds;

    AABB() : minBounds(0, 0, 0), maxBounds(0, 0, 0) {}
    AABB(const Vector3& min, const Vector3& max) : minBounds(min), maxBounds(max) {}
};

// Describes a single element of an input layout (renderer-agnostic)
struct VertexLayoutElement {
    const char* semanticName;
    uint32_t semanticIndex;
    DXGI_FORMAT format;
    uint32_t offset;
};

class Mesh {
protected:
    std::string name;

    // CPU-side data (used by standard Vertex meshes)
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // OPAQUE GPU handle (points to DirectX11Mesh, DirectX12Mesh, etc.)
    void* gpuMeshHandle;

    // Metadata
    AABB bounds;
    bool isDirty;
    D3D11_PRIMITIVE_TOPOLOGY topology;

public:
    Mesh(const std::string& name = "");
    virtual ~Mesh();

    // Data management (standard Vertex)
    void SetData(const std::vector<Vertex>& verts, const std::vector<uint32_t>& inds);
    void SetVertices(const std::vector<Vertex>& verts);
    void SetIndices(const std::vector<uint32_t>& inds);

    [[nodiscard]] const std::vector<Vertex>& GetVertices() const;
    [[nodiscard]] const std::vector<uint32_t>& GetIndices() const;

    // ===== Virtual interface for renderer =====
    // These allow subclasses (VoxelMesh) to provide their own vertex format

    // Raw vertex data pointer for GPU upload
    [[nodiscard]] virtual const void* GetRawVertexData() const;

    // Raw index data pointer for GPU upload
    [[nodiscard]] virtual const uint32_t* GetRawIndexData() const;

    // Size of a single vertex in bytes
    [[nodiscard]] virtual size_t GetVertexStride() const;

    // Number of vertices
    [[nodiscard]] virtual size_t GetVertexCount() const;

    // Number of indices
    [[nodiscard]] virtual size_t GetIndexCount() const;

    // Input layout description for this vertex format
    // Returns array of elements, sets outCount to number of elements
    [[nodiscard]] virtual const VertexLayoutElement* GetInputLayoutDesc(size_t& outCount) const;

    // Release CPU-side data after GPU upload
    virtual void ReleaseCPUData();

    // GPU handle (used by renderer)
    [[nodiscard]] void* GetGPUHandle() const;
    void SetGPUHandle(void* handle);

    // Metadata
    const AABB& GetBounds() const;
    virtual void CalculateBounds();

    bool IsDirty() const;
    void MarkDirty();
    void ClearDirty();

    const std::string& GetName() const;

    // Topology
    void SetTopology(D3D11_PRIMITIVE_TOPOLOGY topo);
    D3D11_PRIMITIVE_TOPOLOGY GetTopology() const;

    // Bind mesh for rendering (requires renderer context)
    void Bind(class IRendererApi* renderer);
};



#endif //DIRECTX11_MESH_H
