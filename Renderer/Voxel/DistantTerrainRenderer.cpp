//
// Created by Marvin on 12/02/2026.
//

#include "DistantTerrainRenderer.h"
#include "World/Generators/TerrainGenerator.h"
#include "Renderer/RenderApi/IRendererApi.h"
#include "Renderer/Shaders/ShaderCollection.h"
#include "Renderer/Shaders/ShaderProgram.h"
#include "Renderer/Shaders/ShaderTypes.h"
#include "Renderer/Vertex.h"
#include "Resources/Mesh.h"
#include <vector>
#include <cmath>
#include <iostream>

DistantTerrainRenderer::DistantTerrainRenderer(IRendererApi* renderer, ShaderCollection* shaderCollection)
    : _renderer(renderer)
    , _shaderCollection(shaderCollection)
{
}

DistantTerrainRenderer::~DistantTerrainRenderer()
{
    delete _mesh;
}

Math::Vector3 DistantTerrainRenderer::HeightToColor(float height, float maxHeight)
{
    float t = std::max(0.0f, std::min(1.0f, height / maxHeight));

    if (t < 0.08f) {
        float s = t / 0.08f;
        return Math::Vector3(0.55f - s * 0.30f, 0.50f - s * 0.05f, 0.30f - s * 0.15f);
    } else if (t < 0.20f) {
        float s = (t - 0.08f) / 0.12f;
        return Math::Vector3(0.25f - s * 0.03f, 0.45f + s * 0.12f, 0.15f - s * 0.02f);
    } else if (t < 0.40f) {
        float s = (t - 0.20f) / 0.20f;
        return Math::Vector3(0.22f + s * 0.10f, 0.57f - s * 0.02f, 0.13f + s * 0.05f);
    } else if (t < 0.60f) {
        // Dry grass -> rock transition — desaturated olive
        float s = (t - 0.40f) / 0.20f;
        return Math::Vector3(0.32f + s * 0.10f, 0.55f - s * 0.15f, 0.18f + s * 0.15f);
    } else if (t < 0.80f) {
        // Grey stone — cool neutral, no red
        float s = (t - 0.60f) / 0.20f;
        return Math::Vector3(0.42f + s * 0.05f, 0.40f + s * 0.03f, 0.38f + s * 0.05f);
    } else {
        // Snow/peak — cold grey-white
        float s = (t - 0.80f) / 0.20f;
        return Math::Vector3(0.47f + s * 0.35f, 0.43f + s * 0.38f, 0.43f + s * 0.40f);
    }
}

void DistantTerrainRenderer::Build(const TerrainGenerator* terrain,
                                    const DistantTerrainSettings& settings)
{
    _settings = settings;

    if (!terrain) {
        std::cerr << "DistantTerrainRenderer: No terrain generator!" << std::endl;
        return;
    }

    _shader = _shaderCollection->LoadShader("distant_terrain",
        "Assets/Shaders/Terrain/distant_terrain.vert.hlsl",
        "Assets/Shaders/Terrain/distant_terrain.pixel.hlsl");

    if (!_shader) {
        std::cerr << "DistantTerrainRenderer: Failed to load shader!" << std::endl;
        return;
    }

    const TerrainSettings& ts = terrain->GetSettings();
    float worldRadius = ts.worldRadius;
    float maxHeight = ts.maxMountainHeight + ts.maxHeight;
    int cell = _settings.cellSize;

    int gridRadius = static_cast<int>(std::ceil(worldRadius / cell)) + 1;
    int gridW = gridRadius * 2 + 1;
    int gridH = gridW;

    std::cout << "DistantTerrainRenderer: Building blocky heightmap "
              << gridW << "x" << gridH << " (cell=" << cell << ")" << std::endl;

    // Step 1: Sample heights — snap to integers for blocky look
    std::vector<float> heights(gridW * gridH, -1.0f);

    for (int gz = 0; gz < gridH; gz++) {
        for (int gx = 0; gx < gridW; gx++) {
            int worldX = (gx - gridRadius) * cell;
            int worldZ = (gz - gridRadius) * cell;
            int h = terrain->GetHeightAt(worldX, worldZ);
            heights[gz * gridW + gx] = static_cast<float>(h); // already integer from GetHeightAt
        }
    }

    auto getH = [&](int gx, int gz) -> float {
        if (gx < 0 || gx >= gridW || gz < 0 || gz >= gridH) return -1.0f;
        return heights[gz * gridW + gx];
    };

    // Step 2: Build UNSHARED vertices — 4 per quad for flat shading
    // Each quad gets its own vertices so ddx/ddy gives a flat per-triangle normal.
    // Color is based on the average height of the quad (uniform per face).
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    size_t estimatedQuads = static_cast<size_t>(3.14159 * gridRadius * gridRadius);
    vertices.reserve(estimatedQuads * 4);
    indices.reserve(estimatedQuads * 6);

    // Compute the world-space bounds of loaded chunks to skip
    // Chunks go from -renderDist to +renderDist in chunk coords,
    // each chunk is 32 blocks wide
    float chunkExcludeMin = static_cast<float>(-_settings.chunkRenderDistance) * 32.0f;
    float chunkExcludeMax = static_cast<float>(_settings.chunkRenderDistance) * 32.0f;

    for (int gz = 0; gz < gridH - 1; gz++) {
        for (int gx = 0; gx < gridW - 1; gx++) {
            float h00 = getH(gx,     gz);
            float h10 = getH(gx + 1, gz);
            float h01 = getH(gx,     gz + 1);
            float h11 = getH(gx + 1, gz + 1);

            // Skip fully outside world
            if (h00 < 0 && h10 < 0 && h01 < 0 && h11 < 0) continue;

            float x0 = static_cast<float>((gx - gridRadius) * cell);
            float x1 = static_cast<float>((gx + 1 - gridRadius) * cell);
            float z0 = static_cast<float>((gz - gridRadius) * cell);
            float z1 = static_cast<float>((gz + 1 - gridRadius) * cell);

            // Skip quads fully inside the loaded chunk area
            if (x0 >= chunkExcludeMin && x1 <= chunkExcludeMax &&
                z0 >= chunkExcludeMin && z1 <= chunkExcludeMax) {
                continue;
            }

            // Skirt for edge cells
            float skirtY = -_settings.skirtDepth;
            float y00 = (h00 >= 0) ? h00 : skirtY;
            float y10 = (h10 >= 0) ? h10 : skirtY;
            float y01 = (h01 >= 0) ? h01 : skirtY;
            float y11 = (h11 >= 0) ? h11 : skirtY;

            // Use average height for uniform face color
            float avgH = (std::max(0.0f, y00) + std::max(0.0f, y10) +
                          std::max(0.0f, y01) + std::max(0.0f, y11)) * 0.25f;
            auto col = HeightToColor(avgH, maxHeight);
            Math::Vector4 faceColor(col.x, col.y, col.z, 1.0f);

            // Dummy normal — pixel shader computes flat normal via ddx/ddy
            Math::Vector3 dummyNormal(0, 1, 0);

            uint32_t baseIdx = static_cast<uint32_t>(vertices.size());

            Vertex v0, v1, v2, v3;

            v0.position = Math::Vector3(x0, y00, z0);
            v0.normal = dummyNormal;
            v0.color = faceColor;
            v0.texCoord = Math::Vector2(0, 0);
            v0.lightLevel = 15.0f;
            v0.textureIndex = 0;
            v0.uvRotation = 0;

            v1.position = Math::Vector3(x1, y10, z0);
            v1.normal = dummyNormal;
            v1.color = faceColor;
            v1.texCoord = Math::Vector2(1, 0);
            v1.lightLevel = 15.0f;
            v1.textureIndex = 0;
            v1.uvRotation = 0;

            v2.position = Math::Vector3(x1, y11, z1);
            v2.normal = dummyNormal;
            v2.color = faceColor;
            v2.texCoord = Math::Vector2(1, 1);
            v2.lightLevel = 15.0f;
            v2.textureIndex = 0;
            v2.uvRotation = 0;

            v3.position = Math::Vector3(x0, y01, z1);
            v3.normal = dummyNormal;
            v3.color = faceColor;
            v3.texCoord = Math::Vector2(0, 1);
            v3.lightLevel = 15.0f;
            v3.textureIndex = 0;
            v3.uvRotation = 0;

            vertices.push_back(v0);
            vertices.push_back(v1);
            vertices.push_back(v2);
            vertices.push_back(v3);

            indices.push_back(baseIdx + 0);
            indices.push_back(baseIdx + 1);
            indices.push_back(baseIdx + 2);

            indices.push_back(baseIdx + 0);
            indices.push_back(baseIdx + 2);
            indices.push_back(baseIdx + 3);
        }
    }

    if (indices.empty()) {
        std::cerr << "DistantTerrainRenderer: No terrain triangles!" << std::endl;
        return;
    }

    // Step 3: Upload
    _mesh = new Mesh("DistantTerrain");
    _mesh->SetData(vertices, indices);

    if (!_renderer->CreateMesh(_mesh)) {
        std::cerr << "DistantTerrainRenderer: Failed to create GPU mesh!" << std::endl;
        delete _mesh;
        _mesh = nullptr;
        return;
    }

    _renderer->CreateMeshInputLayout(_mesh, _shader->GetVertexShader());
    _mesh->ReleaseCPUData();

    std::cout << "DistantTerrainRenderer: "
              << vertices.size() << " verts, "
              << indices.size() / 3 << " tris" << std::endl;
}

void DistantTerrainRenderer::Rebuild(const TerrainGenerator* terrain)
{
    delete _mesh;
    _mesh = nullptr;
    Build(terrain, _settings);
}

void DistantTerrainRenderer::Render(void* perFrameBuffer, void* perObjectBuffer)
{
    if (!_mesh || !_shader) return;

    _renderer->BindShader(_shader);
    _renderer->SetRasterizerMode(RasterizerMode::NoCull);
    // Identity world matrix — vertices are already in world space
    PerObjectBuffer perObject;
    perObject.world = Math::Matrix4x4::Identity;
    perObject.worldInverseTranspose = Math::Matrix4x4::Identity;
    _renderer->UpdateConstantBuffer(perObjectBuffer, &perObject, sizeof(PerObjectBuffer));
    _renderer->BindConstantBuffer(1, perObjectBuffer, ShaderStage::Vertex);

    _renderer->Draw(_mesh);
}