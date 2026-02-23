//
// Created by Claude on 04/02/2026.
//

#include "DebugLineRenderer.h"
#include "Renderer/RenderApi/IRendererApi.h"
#include "Renderer/Shaders/ShaderProgram.h"
#include "Renderer/Shaders/Shader.h"
#include "Renderer/Shaders/ShaderTypes.h"
#include "Resources/Mesh.h"
#include "Core/Math/Matrix4x4.h"
#include <iostream>

DebugLineRenderer::DebugLineRenderer(IRendererApi* renderer)
    : renderer(renderer)
    , mesh(nullptr)
    , meshDirty(false)
{
    mesh = new Mesh("debug_lines");
}

DebugLineRenderer::~DebugLineRenderer() {
    delete mesh;
}

void DebugLineRenderer::Clear() {
    vertices.clear();
    meshDirty = true;
}

void DebugLineRenderer::AddLine(const Math::Vector3& start, const Math::Vector3& end, const Math::Vector4& color) {
    Vertex v1;
    v1.position = start;
    v1.color = color;
    v1.normal = Math::Vector3(0, 1, 0);
    v1.texCoord = Math::Vector2(0, 0);
    v1.lightLevel = 15.0f;
    v1.textureIndex = 0;
    v1.uvRotation = 0;

    Vertex v2;
    v2.position = end;
    v2.color = color;
    v2.normal = Math::Vector3(0, 1, 0);
    v2.texCoord = Math::Vector2(0, 0);
    v2.lightLevel = 15.0f;
    v2.textureIndex = 0;
    v2.uvRotation = 0;

    vertices.push_back(v1);
    vertices.push_back(v2);
    meshDirty = true;
}

void DebugLineRenderer::AddBox(const Math::Vector3& min, const Math::Vector3& max, const Math::Vector4& color) {
    // 8 corners of the box
    Math::Vector3 corners[8] = {
        Math::Vector3(min.x, min.y, min.z),  // 0: bottom-back-left
        Math::Vector3(max.x, min.y, min.z),  // 1: bottom-back-right
        Math::Vector3(max.x, min.y, max.z),  // 2: bottom-front-right
        Math::Vector3(min.x, min.y, max.z),  // 3: bottom-front-left
        Math::Vector3(min.x, max.y, min.z),  // 4: top-back-left
        Math::Vector3(max.x, max.y, min.z),  // 5: top-back-right
        Math::Vector3(max.x, max.y, max.z),  // 6: top-front-right
        Math::Vector3(min.x, max.y, max.z),  // 7: top-front-left
    };

    // 12 edges of the box
    // Bottom face
    AddLine(corners[0], corners[1], color);
    AddLine(corners[1], corners[2], color);
    AddLine(corners[2], corners[3], color);
    AddLine(corners[3], corners[0], color);

    // Top face
    AddLine(corners[4], corners[5], color);
    AddLine(corners[5], corners[6], color);
    AddLine(corners[6], corners[7], color);
    AddLine(corners[7], corners[4], color);

    // Vertical edges
    AddLine(corners[0], corners[4], color);
    AddLine(corners[1], corners[5], color);
    AddLine(corners[2], corners[6], color);
    AddLine(corners[3], corners[7], color);
}

void DebugLineRenderer::Render(ShaderProgram* shader, void* perFrameBuffer, void* perObjectBuffer) {
    if (vertices.empty() || !shader || !renderer) {
        return;
    }

    // Update mesh data if dirty - need to recreate GPU mesh each frame since line count changes
    if (meshDirty) {
        // Delete old mesh and create new one
        delete mesh;
        mesh = new Mesh("debug_lines");

        // Use empty indices for line list (non-indexed rendering)
        std::vector<uint32_t> emptyIndices;
        mesh->SetData(vertices, emptyIndices);
        mesh->SetTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        meshDirty = false;
    }

    // Create GPU mesh if needed
    if (!mesh->GetGPUHandle()) {
        if (!renderer->CreateMesh(mesh)) {
            std::cerr << "DebugLineRenderer: Failed to create GPU mesh!" << std::endl;
            return;
        }
    }

    // Create input layout if needed
    Shader* vertexShader = shader->GetVertexShader();
    if (vertexShader) {
        renderer->CreateMeshInputLayout(mesh, vertexShader);
    }

    // Bind shader
    renderer->BindShader(shader);

    // Bind constant buffers
    renderer->BindConstantBuffer(0, perFrameBuffer, ShaderStage::Vertex | ShaderStage::Pixel);

    // Set identity world matrix
    PerObjectBuffer perObject;
    perObject.world = Math::Matrix4x4::Identity;
    perObject.worldInverseTranspose = Math::Matrix4x4::Identity;
    renderer->UpdateConstantBuffer(perObjectBuffer, &perObject, sizeof(PerObjectBuffer));
    renderer->BindConstantBuffer(1, perObjectBuffer, ShaderStage::Vertex);

    // Disable depth write for debug lines (so they show through geometry slightly)
    renderer->SetDepthMode(DepthMode::ReadOnly);

    // Draw using non-indexed rendering
    mesh->Bind(renderer);
    renderer->GetContext()->Draw(static_cast<UINT>(vertices.size()), 0);

    // Restore depth mode
    renderer->SetDepthMode(DepthMode::Less);

    renderer->UnbindShader();
}
