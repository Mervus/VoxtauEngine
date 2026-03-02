//
// VoxtauEngine Demo Scene
//

#include "DemoGameplayScene.h"

#include <iostream>

#include "Core/Entity/EntityManager.h"
#include "Core/Entity/Living/Player/PlayerEntity.h"
#include "Core/Entity/Living/Player/PlayerController.h"
#include "Core/Math/MathTypes.h"
#include "Core/Profiler/Profiler.h"
#include "Renderer/Debug/DebugLineRenderer.h"
#include "Renderer/Entity/EntityRenderer.h"
#include "Renderer/RenderApi/IRendererApi.h"
#include "Renderer/Shaders/ShaderCollection.h"
#include "Renderer/Sky/SkyRenderer.h"
#include "Renderer/Voxel/VoxelRenderer.h"

//  DemoTerrainGenerator 

DemoTerrainGenerator::DemoTerrainGenerator()
    : _noise(42)
{
}

DemoTerrainGenerator::DemoTerrainGenerator(uint32_t seed)
    : _noise(seed)
{
}

void DemoTerrainGenerator::GenerateChunk(Chunk* chunk) const
{
    if (!chunk) return;

    Math::Vector3 chunkPos = chunk->GetWorldPosition();

    int startX = static_cast<int>(chunkPos.x) * Chunk::CHUNK_SIZE;
    int startY = static_cast<int>(chunkPos.y) * Chunk::CHUNK_SIZE;
    int startZ = static_cast<int>(chunkPos.z) * Chunk::CHUNK_SIZE;

    for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
        for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
            int worldX = startX + x;
            int worldZ = startZ + z;

            // Perlin noise height
            double nx = worldX * SCALE;
            double nz = worldZ * SCALE;
            double noise = _noise.OctaveNoise01(nx, nz, OCTAVES, PERSISTENCE, LACUNARITY);
            int surfaceHeight = MIN_HEIGHT + static_cast<int>(noise * (MAX_HEIGHT - MIN_HEIGHT));

            for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
                int worldY = startY + y;

                if (worldY > surfaceHeight) continue;

                int depth = surfaceHeight - worldY;

                uint8_t blockType;
                if (depth == 0) {
                    blockType = BlockType::Grass;
                } else if (depth < 4) {
                    blockType = BlockType::Dirt;
                } else {
                    blockType = BlockType::Stone;
                }

                chunk->SetBlock(x, y, z, blockType);
            }
        }
    }
}

//  DemoGameplayScene 

DemoGameplayScene::DemoGameplayScene()
    : Scene("Demo")
{
}

DemoGameplayScene::~DemoGameplayScene()
{
    OnDestroy();
}

void DemoGameplayScene::OnInit()
{
    std::cout << "[Demo] Initializing..." << std::endl;

    // Sky
    _skyRenderer = new SkyRenderer(renderer, shaderCollection);
    _skyRenderer->Initialize();
    _skyRenderer->SetTimeOfDay(0.35f);
    _skyRenderer->SetDaySpeed(0.0f);

    // Render pipeline
    _renderPipeline = new RenderPipeline(renderer, shaderCollection);
    _renderPipeline->Initialize(
        renderer->GetViewportWidth(),
        renderer->GetViewportHeight()
    );

    // Voxel world
    _chunkManager = new ChunkManager();
    SetupBlockRegistry();

    _debugLineRenderer = new DebugLineRenderer(renderer);

    _voxelRenderer = new VoxelRenderer(renderer, shaderCollection, _blockRegistry);
    _voxelRenderer->Initialize(true);
    _voxelRenderer->SetChunkManager(_chunkManager);

    _renderPipeline->SetSkyRenderer(_skyRenderer);
    _renderPipeline->SetDebugRenderer(_debugLineRenderer);

    // Camera
    SetupCamera();
    _renderPipeline->SetCamera(mainCamera);

    // Entities
    _entityManager = new EntityManager();

    // Physics
    _voxelPhysics = new VoxelPhysics();
    _voxelPhysics->Initialize(_chunkManager);

    // Player
    SetupPlayer();

    // Generate terrain
    GenerateWorld();

    inputManager->SetMouseCaptured(true);

    std::cout << "[Demo] Ready." << std::endl;
}

void DemoGameplayScene::OnDestroy()
{
    if (_voxelPhysics) {
        _voxelPhysics->Shutdown();
        delete _voxelPhysics;
        _voxelPhysics = nullptr;
    }

    if (_renderPipeline) {
        _renderPipeline->Shutdown();
        delete _renderPipeline;
        _renderPipeline = nullptr;
    }

    delete _skyRenderer;       _skyRenderer = nullptr;
    delete mainCamera;         mainCamera = nullptr;
    delete _blockTextureArray; _blockTextureArray = nullptr;
    delete _blockRegistry;     _blockRegistry = nullptr;
    delete _chunkManager;      _chunkManager = nullptr;
    delete _terrainGen;        _terrainGen = nullptr;
    delete _debugLineRenderer; _debugLineRenderer = nullptr;
    delete _playerController;  _playerController = nullptr;
    delete _entityManager;     _entityManager = nullptr;
}

void DemoGameplayScene::Update(float deltaTime)
{
    PROFILE_FUNCTION();

    _playerController->Update(deltaTime);

    if (_voxelPhysics) {
        _voxelPhysics->Update(deltaTime);
    }

    _entityManager->Update(deltaTime);

    _playerController->LateUpdate(deltaTime);
    _entityManager->LateUpdate(deltaTime);

    Scene::Update(deltaTime);
}

void DemoGameplayScene::Render(float deltaTime)
{
    PROFILE_FUNCTION();

    if (!mainCamera || !_renderPipeline) return;

    _renderPipeline->SetCamera(mainCamera);
    _renderPipeline->SetDebugCameraMode(
        _playerController && _playerController->IsDebugCamera());

    _renderPipeline->GetEntityRenderer()->SubmitEntities(_entityManager);
    _renderPipeline->Execute(deltaTime, _totalTime);
}

void DemoGameplayScene::SetupBlockRegistry()
{
    _blockRegistry = new BlockRegistry();
    _blockRegistry->LoadFromFile("Assets/Data/blocks.json");

    _blockTextureArray = new TextureArray("BlockTextures");
}

void DemoGameplayScene::SetupCamera()
{
    mainCamera = new Camera();
    mainCamera->SetPerspective(Math::PI / 3.0f, 16.0f / 9.0f, 0.1f, 2500.0f);
}

void DemoGameplayScene::SetupPlayer()
{
    EntityID playerID = _entityManager->CreateEntity<PlayerEntity>("Player");
    PlayerEntity* player = _entityManager->GetEntityAs<PlayerEntity>(playerID);

    // No mesh — the player is an invisible physics body with a camera
    _playerController = new PlayerController(_entityManager, inputManager, mainCamera, nullptr);
    _playerController->SetPlayerID(playerID);

    Math::Vector3 spawnPos(0.0f, 24.0f, 0.0f);
    player->SetPosition(spawnPos);
    player->SetRespawnPosition(spawnPos);

    VoxelBodyID bodyId = _voxelPhysics->CreateBody(spawnPos, 0.6f, 1.8f);
    player->BindPhysics(_voxelPhysics, bodyId);
}

void DemoGameplayScene::GenerateWorld()
{
    _terrainGen = new DemoTerrainGenerator();

    _chunkManager->SetBlockRegistry(_blockRegistry);
    _chunkManager->SetChunkGenerator(_terrainGen);

    for (int y = 0; y < VERTICAL_CHUNKS; y++)
        for (int x = -RENDER_DISTANCE; x < RENDER_DISTANCE; ++x)
            for (int z = -RENDER_DISTANCE; z < RENDER_DISTANCE; ++z)
                _chunkManager->CreateChunk(x, y, z);
}
