//
// Created by Marvin on 28/01/2026.
//

#include "MeshGenerator.h"

Mesh* MeshGenerator::CreateCube(const Math::Vector3& position) {
    Mesh* mesh = new Mesh("cube");

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    float size = 1.0f;
    float halfSize = size * 0.5f;

    // Define 8 corners of the cube (centered at position)
    Math::Vector3 corners[8] = {
        position + Math::Vector3(-halfSize, -halfSize, -halfSize), // 0: left-bottom-back
        position + Math::Vector3( halfSize, -halfSize, -halfSize), // 1: right-bottom-back
        position + Math::Vector3( halfSize,  halfSize, -halfSize), // 2: right-top-back
        position + Math::Vector3(-halfSize,  halfSize, -halfSize), // 3: left-top-back
        position + Math::Vector3(-halfSize, -halfSize,  halfSize), // 4: left-bottom-front
        position + Math::Vector3( halfSize, -halfSize,  halfSize), // 5: right-bottom-front
        position + Math::Vector3( halfSize,  halfSize,  halfSize), // 6: right-top-front
        position + Math::Vector3(-halfSize,  halfSize,  halfSize)  // 7: left-top-front
    };

    // Face normals
    Math::Vector3 normals[6] = {
        Math::Vector3( 0,  0,  1), // Front
        Math::Vector3( 0,  0, -1), // Back
        Math::Vector3(-1,  0,  0), // Left
        Math::Vector3( 1,  0,  0), // Right
        Math::Vector3( 0,  1,  0), // Top
        Math::Vector3( 0, -1,  0)  // Bottom
    };

    // Face corner indices
    int faceCorners[6][4] = {
        {4, 5, 6, 7}, // Front
        {1, 0, 3, 2}, // Back
        {0, 4, 7, 3}, // Left
        {5, 1, 2, 6}, // Right
        {7, 6, 2, 3}, // Top
        {4, 5, 1, 0}  // Bottom
    };

    // UV coordinates for each corner of a face
    Math::Vector2 uvs[4] = {
        Math::Vector2(0, 1), // Bottom-left
        Math::Vector2(1, 1), // Bottom-right
        Math::Vector2(1, 0), // Top-right
        Math::Vector2(0, 0)  // Top-left
    };

    // Generate vertices for each face
    for (int face = 0; face < 6; face++) {
        int baseIndex = vertices.size();

        for (int i = 0; i < 4; i++) {
            Vertex v;
            v.position = corners[faceCorners[face][i]];
            v.normal = normals[face];
            v.texCoord = uvs[i];
            v.color = Math::Vector4(1, 1, 1, 1); // White
            v.tangent = Math::Vector3(1, 0, 0);
            v.lightLevel = 15.0f; // Full brightness

            vertices.push_back(v);
        }

        // Generate indices (two triangles per face)
        indices.push_back(baseIndex + 0);
        indices.push_back(baseIndex + 1);
        indices.push_back(baseIndex + 2);

        indices.push_back(baseIndex + 0);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 3);
    }

    mesh->SetData(vertices, indices);
    return mesh;
}

Mesh* MeshGenerator::CreateSphere(float radius, int segments, int rings)
{
    Mesh* mesh = new Mesh("sphere");

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Generate vertices
    for (int ring = 0; ring <= rings; ++ring)
    {
        float phi = Math::PI * ring / rings;
        float y = radius * cosf(phi);
        float ringRadius = radius * sinf(phi);

        for (int seg = 0; seg <= segments; ++seg)
        {
            float theta = 2.0f * Math::PI * seg / segments;
            float x = ringRadius * cosf(theta);
            float z = ringRadius * sinf(theta);

            Vertex v;
            v.position = Math::Vector3(x, y, z);
            v.normal = Math::Vector3(x, y, z).Normalized(); // Point outward
            v.texCoord = Math::Vector2((float)seg / segments, (float)ring / rings);
            v.color = Math::Vector4(1, 1, 1, 1);
            v.tangent = Math::Vector3(1, 0, 0);
            v.lightLevel = 15.0f;

            vertices.push_back(v);
        }
    }

    // Generate indices
    for (int ring = 0; ring < rings; ++ring)
    {
        for (int seg = 0; seg < segments; ++seg)
        {
            int current = ring * (segments + 1) + seg;
            int next = current + segments + 1;

            // Two triangles per quad
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);

            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    mesh->SetData(vertices, indices);
    return mesh;
}

Mesh* MeshGenerator::CreateFullscreenQuad() {
    Mesh* mesh = new Mesh("fullscreen_quad");

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Fullscreen quad in NDC space (-1 to 1)
    // Positioned at z=0 (will be transformed to screen space by vertex shader)
    Vertex v0, v1, v2, v3;

    // Bottom-left
    v0.position = Math::Vector3(-1.0f, -1.0f, 0.0f);
    v0.texCoord = Math::Vector2(0.0f, 1.0f);
    v0.normal = Math::Vector3(0.0f, 0.0f, 1.0f);
    v0.color = Math::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    v0.tangent = Math::Vector3(1.0f, 0.0f, 0.0f);
    v0.lightLevel = 15.0f;

    // Bottom-right
    v1.position = Math::Vector3(1.0f, -1.0f, 0.0f);
    v1.texCoord = Math::Vector2(1.0f, 1.0f);
    v1.normal = Math::Vector3(0.0f, 0.0f, 1.0f);
    v1.color = Math::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    v1.tangent = Math::Vector3(1.0f, 0.0f, 0.0f);
    v1.lightLevel = 15.0f;

    // Top-right
    v2.position = Math::Vector3(1.0f, 1.0f, 0.0f);
    v2.texCoord = Math::Vector2(1.0f, 0.0f);
    v2.normal = Math::Vector3(0.0f, 0.0f, 1.0f);
    v2.color = Math::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    v2.tangent = Math::Vector3(1.0f, 0.0f, 0.0f);
    v2.lightLevel = 15.0f;

    // Top-left
    v3.position = Math::Vector3(-1.0f, 1.0f, 0.0f);
    v3.texCoord = Math::Vector2(0.0f, 0.0f);
    v3.normal = Math::Vector3(0.0f, 0.0f, 1.0f);
    v3.color = Math::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    v3.tangent = Math::Vector3(1.0f, 0.0f, 0.0f);
    v3.lightLevel = 15.0f;

    vertices.push_back(v0);
    vertices.push_back(v1);
    vertices.push_back(v2);
    vertices.push_back(v3);

    // Two triangles: 0-1-2 and 0-2-3
    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(2);

    indices.push_back(0);
    indices.push_back(2);
    indices.push_back(3);

    mesh->SetData(vertices, indices);
    return mesh;
}
