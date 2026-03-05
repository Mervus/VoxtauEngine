//
// Created by Marvin on 14/02/2026.
//

#ifndef VOXTAU_SKINNEDMESH_H
#define VOXTAU_SKINNEDMESH_H
#pragma once

#include <EngineApi.h>
#include "Mesh.h"
#include "Renderer/Vertex.h"
#include <vector>

class SkinnedMesh : public Mesh {
private:
    std::vector<SkinnedVertex> skinnedVertices;

public:
    SkinnedMesh(const std::string& name = "");
    ~SkinnedMesh() override;

    // Set skinned vertex data
    void SetSkinnedData(const std::vector<SkinnedVertex>& verts,
                        const std::vector<uint32_t>& inds);

    const std::vector<SkinnedVertex>& GetSkinnedVertices() const { return skinnedVertices; }

    // Virtual interface overrides (so the renderer sees SkinnedVertex data)
    [[nodiscard]] const void* GetRawVertexData() const override;
    [[nodiscard]] const uint32_t* GetRawIndexData() const override;
    [[nodiscard]] size_t GetVertexStride() const override;
    [[nodiscard]] size_t GetVertexCount() const override;
    [[nodiscard]] size_t GetIndexCount() const override;
    [[nodiscard]] const VertexLayoutElement* GetInputLayoutDesc(size_t& outCount) const override;
    void ReleaseCPUData() override;
    void CalculateBounds() override;
};

#endif //VOXTAU_SKINNEDMESH_H