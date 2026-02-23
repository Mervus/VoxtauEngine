//
// Created by Marvin on 05/02/2026.
//

#ifndef VOXTAU_VOXELMESH_H
#define VOXTAU_VOXELMESH_H
#pragma once

#include "Mesh.h"
#include "../Renderer/Vertex.h"
#include <vector>

class ENGINE_API VoxelMesh : public Mesh {
private:
    std::vector<VoxelVertex> voxelVertices;
    std::vector<uint32_t> voxelIndices;

public:
    VoxelMesh(const std::string& name = "");
    ~VoxelMesh() override;

    // Set voxel vertex data
    void SetVoxelData(const std::vector<VoxelVertex>& verts, const std::vector<uint32_t>& inds);

    // Direct access to voxel vertices (for building mesh)
    [[nodiscard]] const std::vector<VoxelVertex>& GetVoxelVertices() const;
    [[nodiscard]] const std::vector<uint32_t>& GetVoxelIndices() const;

    // Overrides from Mesh
    [[nodiscard]] const void* GetRawVertexData() const override;
    [[nodiscard]] const uint32_t* GetRawIndexData() const override;
    [[nodiscard]] size_t GetVertexStride() const override;
    [[nodiscard]] size_t GetVertexCount() const override;
    [[nodiscard]] size_t GetIndexCount() const override;
    [[nodiscard]] const VertexLayoutElement* GetInputLayoutDesc(size_t& outCount) const override;
    void ReleaseCPUData() override;
    void CalculateBounds() override;
};



#endif //VOXTAU_VOXELMESH_H