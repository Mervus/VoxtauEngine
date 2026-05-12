# VoxtauEngine

A custom C++20 voxel engine built on DirectX 11. Features GPU-accelerated voxel meshing via compute shaders, procedural terrain generation, custom AABB physics, skeletal animation, and a modular scene-based architecture.

<img width="2335" height="1371" alt="image" src="https://github.com/user-attachments/assets/047b15eb-d95e-4112-8981-313a5970a899" />
<img width="2473" height="1325" alt="image" src="https://github.com/user-attachments/assets/932c76a7-76c4-42e9-b843-01f1d971637f" />
<img width="1927" height="1193" alt="image" src="https://github.com/user-attachments/assets/57a9b8b2-65d4-439c-b500-3d57a000524a" />
<img width="2412" height="1382" alt="image" src="https://github.com/user-attachments/assets/1fe4d2cb-0978-48b4-8e93-d41bef7a0f42" />
<img width="2531" height="1355" alt="image" src="https://github.com/user-attachments/assets/2013d234-e6c1-48e9-b2e7-f2f118fe49f3" />
<img width="2314" height="1332" alt="image" src="https://github.com/user-attachments/assets/7852f6aa-0036-4546-81c4-3603f312d5fe" />
<img width="2316" height="1325" alt="image" src="https://github.com/user-attachments/assets/f15c09a3-6ffd-431a-8d0e-876001ce35f1" />
<img width="1953" height="1196" alt="image" src="https://github.com/user-attachments/assets/60eb34ad-1fd9-49ce-9567-99fe194e4764" />
<img width="1304" height="1034" alt="image" src="https://github.com/user-attachments/assets/cb9f6d8d-4eb1-4bec-8c37-f250af2fdf06" />


## Features

### Rendering
- **DirectX 11** renderer with compute shader pipeline
- **GPU voxel meshing** — compute shaders generate mesh faces directly on the GPU
- **GPU frustum culling** — per-chunk AABB culling with `DrawIndexedInstancedIndirect`
- **Per-vertex ambient occlusion** computed in the mesh generation shader
- **Texture arrays** for block face textures with per-face mapping
- **Procedural sky dome** with time-of-day simulation
- **Skinned mesh rendering** with skeletal animation (FBX, glTF, OBJ via Assimp)
- **Post-processing pipeline** with modular effect passes
- **Shader hot-reload** — edit HLSL without recompiling the application

### Voxel Engine
- **32x32x32 chunk system** with streaming and padded voxel storage (34x34x34 grids for neighbor-aware face culling)
- **JSON-driven block registry** — define block types, textures, and properties in `blocks.json`
- **Packed GPU format** — 2x uint32 per visible face (position, texture index, AO data)

### Procedural Generation
- **Perlin noise** with octave-based generation, persistence, and lacunarity
- **Modular structure placement** via `IStructureGenerator` interface

### Physics
- **Custom AABB voxel physics** with fixed 60 Hz timestep
- **Sweep collision** with per-axis resolution and auto-stepping for ledges
- **Ray-vs-voxel intersection** for targeting and attacks

### Entity System
- Entity hierarchy: `Entity` -> `LivingEntity` -> `PlayerEntity`
- ID-based entity manager with lifecycle management
- Skeleton-based animation with clips and blending
- Third-person player controller with camera follow

### Debug Tools (ImGui)
- Scene hierarchy inspector
- Performance window (FPS, frame time, memory)
- Per-function profiler with timing breakdown
- Compass and debug line renderer

## Project Structure

```
VoxtauEngine/
├── Core/
│   ├── Application              # Main loop, system ownership
│   ├── Entity/                  # Entity system, PlayerEntity, LivingEntity
│   ├── Input/                   # Keyboard/mouse via GLFW
│   ├── Math/                    # Vector, Matrix, Quaternion, Frustum, Noise
│   ├── Physics/                 # Custom AABB voxel physics
│   ├── Procedural/              # IStructureGenerator, StructurePlacer
│   ├── Profiler/                # Frame timing with named scopes
│   ├── Scene/                   # Scene, SceneManager, Camera, Transform
│   └── Voxel/                   # ChunkManager, Chunk, Block, VoxelType
├── Renderer/
│   ├── RenderApi/DirectX11/     # DX11 implementation, buffers, textures
│   ├── Shaders/                 # ShaderCollection, ShaderTypes, hot-reload
│   ├── Voxel/                   # GPU compute meshing + frustum culling
│   ├── Entity/                  # Skinned/unskinned model rendering
│   ├── Sky/                     # Procedural sky dome
│   ├── PostProcess/             # Post-processing effects
│   ├── Debug/                   # Debug line renderer
│   └── RenderPipeline           # Full render sequence orchestration
├── Resources/
│   ├── ResourceManager          # Asset loading and caching
│   ├── BlockRegistry            # JSON block definitions
│   ├── ModelLoader/             # FBX/glTF/OBJ via Assimp
│   └── Animation/               # Skeleton, AnimationClip, Animator
├── UI/Debug/                    # ImGui windows (hierarchy, profiler, etc.)
├── Demo/
│   └── DemoGameplayScene        # Ready-to-run demo scene (terrain, sky, physics, player)
└── Assets/
    ├── Shaders/                 # HLSL sources (compute, culling, sky, etc.)
    ├── Data/blocks.json         # Default block definitions
    └── Textures/Blocks/         # Placeholder block textures
```

## Prerequisites

- **Windows 10/11**
- **Visual Studio 2022** or later (Desktop development with C++ workload)
- **Windows SDK** 10.0.26100.0 or later
- **CMake 4.1+**
- **Git**

## Getting Started

There are two ways to use VoxtauEngine in your project: **FetchContent** (simplest) or **Git submodule** (for parallel engine development).

### Option A: FetchContent (recommended for most users)

Add the engine to your game's `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 4.1)
project(MyGame)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(VoxtauEngine
    GIT_REPOSITORY https://github.com/Mervus/VoxtauEngine.git
    GIT_TAG master   # or pin to a specific commit/tag
)
FetchContent_MakeAvailable(VoxtauEngine)

add_executable(MyGame main.cpp)
target_link_libraries(MyGame PRIVATE Engine)
# Ensure game Assets directory exists
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Assets)
# Copy your game assets to the output directory
add_custom_target(copy_game_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Assets ${CMAKE_BINARY_DIR}/bin/Assets
)
add_dependencies(MyGame copy_game_assets)

set_target_properties(MyGame PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
```

CMake will download VoxtauEngine and all its dependencies automatically during configure. No manual cloning required.

### Option B: Git submodule (for parallel engine development)

Use this when you want to edit the engine alongside your game — changes to the engine are immediately reflected without re-fetching.

**1. Add the submodule to your game project:**

```bash
cd MyGame
git submodule add https://github.com/Mervus/VoxtauEngine.git VoxtauEngine
git commit -m "Add VoxtauEngine submodule"
```

**2. Set up your `CMakeLists.txt` to support both workflows:**

```cmake
cmake_minimum_required(VERSION 4.1)
project(MyGame)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use local submodule if present, otherwise fetch from GitHub
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VoxtauEngine/CMakeLists.txt")
    add_subdirectory(VoxtauEngine)
else()
    include(FetchContent)
    FetchContent_Declare(VoxtauEngine
        GIT_REPOSITORY https://github.com/Mervus/VoxtauEngine.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(VoxtauEngine)
endif()

add_executable(MyGame main.cpp)
target_link_libraries(MyGame PRIVATE Engine)
# Ensure game Assets directory exists
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Assets)
# Copy your game assets to the output directory
add_custom_target(copy_game_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Assets ${CMAKE_BINARY_DIR}/bin/Assets
)
add_dependencies(MyGame copy_game_assets)

set_target_properties(MyGame PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
```

This way contributors who clone your game with `--recurse-submodules` get the local engine, while others who don't init the submodule still get it via FetchContent automatically.

**3. Clone and build:**

```bash
git clone --recurse-submodules https://github.com/you/MyGame.git
cd MyGame
cmake -B build -S .
cmake --build build --config Release
```

**4. Develop the engine in parallel:**

The engine lives in `VoxtauEngine/` as a regular Git repo. You can edit, commit, and push engine changes independently:

```bash
cd VoxtauEngine
# make changes...
git add -A && git commit -m "Fix chunk streaming bug"
git push origin master

# Back in your game repo, update the submodule reference
cd ..
git add VoxtauEngine
git commit -m "Update engine submodule"
```

## Demo Scene

The engine ships with `DemoGameplayScene` — a self-contained scene that wires up all core systems so you can see the engine running immediately. It includes:

- Perlin noise terrain with grass, dirt, and stone layers
- Procedural sky frozen at mid-morning
- Player with WASD movement, mouse look, and third-person camera
- AABB physics with gravity and ground collision
- 16x6x16 chunk world (~1500 chunks)

### Quick start with the demo

```cpp
#include "Core/Application.h"
#include "Core/Scene/SceneManager.h"
#include "Demo/DemoGameplayScene.h"

int main() {
    Application app("MyGame");
    app.Initialize();

    auto* demo = new DemoGameplayScene();
    app.GetSceneManager()->AddScene("Demo", demo);
    app.GetSceneManager()->LoadScene("Demo");

    app.Run();
    return 0;
}
```

### Writing your own scene

Subclass `Scene` and override the lifecycle methods. Core systems (`renderer`, `shaderCollection`, `resourceManager`, `inputManager`) are injected automatically when the scene is added to the SceneManager.

```cpp
#include "Core/Scene/Scene.h"

class MyScene : public Scene {
public:
    MyScene() : Scene("MyScene") {}

    void OnInit() override {
        // renderer, shaderCollection, resourceManager, inputManager
        // are available here — injected via SetSystems()
    }

    void Update(float dt) override {
        // game logic
    }

    void Render(float dt) override {
        // use renderer-> to draw
    }
};
```

Use `DemoGameplayScene` as a reference for how to set up the full voxel rendering pipeline, physics, and player controller.

## Dependencies

All fetched automatically via CMake `FetchContent` — no manual installation needed.

| Library | Version | Purpose |
|---------|---------|---------|
| [GLFW](https://github.com/glfw/glfw) | latest | Windowing and input |
| [Assimp](https://github.com/assimp/assimp) | 6.0.4 | Model loading (FBX, glTF, OBJ) |
| [Dear ImGui](https://github.com/ocornut/imgui) | 1.92.5 | Debug UI |
| [nlohmann/json](https://github.com/nlohmann/json) | 3.11.3 | JSON parsing |
| [stb](https://github.com/nothings/stb) | latest | Image loading |

## Architecture

The engine uses **dependency injection** — the `Application` class owns all core systems (`IRenderer`, `ShaderCollection`, `InputManager`, `ResourceManager`). The `SceneManager` injects these into `Scene` subclasses via `SetSystems()`, so scenes focus on game logic without managing system lifetimes.

Rendering follows a **component pattern**: renderers like `VoxelRenderer`, `EntityRenderer`, and `SkyRenderer` each own their GPU resources and expose a `Render()` method, orchestrated by `RenderPipeline`.

The voxel pipeline runs entirely on the GPU: voxel data is uploaded as packed buffers, compute shaders generate visible faces, a culling pass filters chunks by frustum, and `DrawIndexedInstancedIndirect` issues draw calls with zero CPU readback.

## License

MIT
