//
// Created by Marvin on 12/02/2026.
//

#ifndef VOXTAU_DISTANTTERRAINRENDERER_H
#define VOXTAU_DISTANTTERRAINRENDERER_H
#pragma once

#include "Core/Math/Vector3.h"

class IRendererApi;
class ShaderCollection;
class TerrainGenerator;
class Mesh;
class ShaderProgram;

struct DistantTerrainSettings {
    int cellSize = 2;           // World-space blocks per heightmap sample
    float skirtDepth = 20.0f;   // How far the edge skirt drops (hides gaps)
    int chunkRenderDistance = 10; // Must match the render distance used for chunk creation
    // Quads inside this range are skipped
};

class DistantTerrainRenderer {
public:
    DistantTerrainRenderer(IRendererApi* renderer, ShaderCollection* shaderCollection);
    ~DistantTerrainRenderer();

    // Build the heightmap mesh from the terrain generator
    // Call once after terrain is configured
    void Build(const TerrainGenerator* terrain, const DistantTerrainSettings& settings = {});

    // Render the distant terrain
    // Call after voxel chunk rendering (depth test will handle overlap)
    void Render(void* perFrameBuffer, void* perObjectBuffer);

    // Rebuild if terrain settings change
    void Rebuild(const TerrainGenerator* terrain);

    bool IsBuilt() const { return _mesh != nullptr; }

private:
    IRendererApi* _renderer;
    ShaderCollection* _shaderCollection;
    ShaderProgram* _shader = nullptr;

    Mesh* _mesh = nullptr;
    DistantTerrainSettings _settings;

    // Height-to-color mapping for visual variety
    static Math::Vector3 HeightToColor(float height, float maxHeight);
};


#endif //VOXTAU_DISTANTTERRAINRENDERER_H