//
// Created by Claude on 04/02/2026.
//

#ifndef VOXTAU_DEBUGLINERENDERER_H
#define VOXTAU_DEBUGLINERENDERER_H

#include <vector>
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"
#include "Renderer/Vertex.h"

class IRendererApi;
class ShaderProgram;
class Mesh;

class DebugLineRenderer {
private:
    IRendererApi* renderer;
    std::vector<Vertex> vertices;
    Mesh* mesh;
    bool meshDirty;

public:
    DebugLineRenderer(IRendererApi* renderer);
    ~DebugLineRenderer();

    // Clear all lines (call at frame start)
    void Clear();

    // Add a single line
    void AddLine(const Math::Vector3& start, const Math::Vector3& end, const Math::Vector4& color);

    // Add a wireframe box (12 edges)
    void AddBox(const Math::Vector3& min, const Math::Vector3& max, const Math::Vector4& color);

    // Render all lines
    void Render(ShaderProgram* shader, void* perFrameBuffer, void* perObjectBuffer);

    // Get vertex count (for debugging)
    size_t GetVertexCount() const { return vertices.size(); }
};

#endif //VOXTAU_DEBUGLINERENDERER_H
