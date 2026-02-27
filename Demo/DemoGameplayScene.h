//
// VoxtauEngine Demo Scene
// A self-contained example scene demonstrating the engine's core systems:
// voxel terrain, sky rendering, physics, and player movement.
//

#ifndef VOXTAUENGINE_DEMOGAMEPLAYSCENE_H
#define VOXTAUENGINE_DEMOGAMEPLAYSCENE_H
#pragma once
#include <EngineApi.h>
#include <Core/Scene/Scene.h>
#include "Core/Physics/Voxtau/VoxelPhysics.h"
#include "Core/Voxel/ChunkManager.h"
#include "Core/Voxel/IChunkGenerator.h"
#include "Core/Math/Noise/PerlinNoise.h"
#include "Resources/BlockRegistry.h"
#include "Resources/TextureArray.h"
#include "Renderer/RenderPipeline.h"

class PlayerController;
class DebugLineRenderer;
class VoxelRenderer;
class SkyRenderer;

// Simple Perlin noise terrain generator for the demo
class DemoTerrainGenerator : public IChunkGenerator {
public:
    DemoTerrainGenerator();
    explicit DemoTerrainGenerator(uint32_t seed);

    void GenerateChunk(Chunk* chunk) const override;

private:
    PerlinNoise _noise;

    static constexpr int MIN_HEIGHT = 4;
    static constexpr int MAX_HEIGHT = 16;
    static constexpr double SCALE = 0.02;
    static constexpr int OCTAVES = 4;
    static constexpr double PERSISTENCE = 0.5;
    static constexpr double LACUNARITY = 2.0;
};

// Demo scene that wires up all engine systems with a simple voxel world
class DemoGameplayScene : public Scene {
public:
    DemoGameplayScene();
    ~DemoGameplayScene() override;

    void OnInit() override;
    void OnDestroy() override;
    void Update(float deltaTime) override;
    void Render(float deltaTime) override;

    ChunkManager* GetChunkManager() const override { return _chunkManager; }
    BlockRegistry* GetBlockRegistry() const { return _blockRegistry; }

private:
    // World
    ChunkManager* _chunkManager           = nullptr;
    BlockRegistry* _blockRegistry         = nullptr;
    TextureArray* _blockTextureArray      = nullptr;
    DemoTerrainGenerator* _terrainGen     = nullptr;

    // Rendering
    VoxelRenderer* _voxelRenderer         = nullptr;
    SkyRenderer* _skyRenderer             = nullptr;
    DebugLineRenderer* _debugLineRenderer = nullptr;
    RenderPipeline* _renderPipeline       = nullptr;

    // Gameplay
    EntityManager* _entityManager         = nullptr;
    PlayerController* _playerController   = nullptr;
    VoxelPhysics* _voxelPhysics           = nullptr;


    // World size (in chunks)
    static constexpr int RENDER_DISTANCE = 8;
    static constexpr int VERTICAL_CHUNKS = 6;

    void SetupBlockRegistry();
    void SetupCamera();
    void SetupPlayer();
    void GenerateWorld();
};

#endif //VOXTAUENGINE_DEMOGAMEPLAYSCENE_H
