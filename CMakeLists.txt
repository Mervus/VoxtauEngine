cmake_minimum_required(VERSION 4.1)
project(VoxtauEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(nlohmann_json GIT_REPOSITORY https://github.com/nlohmann/json.git GIT_TAG v3.11.3)
FetchContent_Declare(enet GIT_REPOSITORY https://github.com/lsalzman/enet.git GIT_TAG v1.3.18)
FetchContent_MakeAvailable(nlohmann_json)
FetchContent_MakeAvailable(enet)
target_include_directories(enet PUBLIC ${enet_SOURCE_DIR}/include)

# EngineCore (cross-platform, no GPU)
# This MUST come first since Engine depends on it

add_library(EngineCore STATIC
        # Entity system
        Core/Entity/Entity.cpp
        Core/Entity/EntityManager.cpp
        Core/Entity/Living/LivingEntity.cpp
        Core/Entity/Living/Player/PlayerEntity.cpp

        # Math
        Core/Math/Vector2.cpp
        Core/Math/Vector3.cpp
        Core/Math/Vector4.cpp
        Core/Math/Matrix4x4.cpp
        Core/Math/Quaternion.cpp
        Core/Math/Frustum.cpp
        Core/Math/Noise/PerlinNoise.cpp

        # Physics
        Core/Physics/Voxtau/VoxelPhysics.cpp

        # Voxel (data only, no rendering)
        Core/Voxel/Voxel.cpp
        Core/Voxel/Block.cpp
        Core/Voxel/Chunk.cpp
        Core/Voxel/ChunkManager.cpp
        Core/Voxel/VoxelType.cpp

        # Procedural generation
        Core/Procedural/StructurePlacer.cpp

        # Scene (transform only)
        Core/Scene/Transform.cpp

        # Profiler
        Core/Profiler/Profiler.cpp

        # Resources (data only)
        Resources/BlockRegistry.cpp
        Core/Network/Transport/INetworkTransport.h
        Core/Network/NetContext.cpp
        Core/Network/NetContext.h
        Core/Network/Server/ServerInstance.cpp
        Core/Network/Server/ServerInstance.h
        Core/Network/Client/ClientSession.cpp
        Core/Network/Client/ClientSession.h
        Core/Network/Transport/LocalTransport.cpp
        Core/Network/Transport/LocalTransport.h
        Core/Network/Protocol/InputButton.h
        Core/Network/Protocol/ServerSnapshot.h
        Core/Network/Server/ClientProxy.h
        Core/Network/Protocol/PlayerInputState.h
        Core/Network/Protocol/PacketTypes.h
        Core/Network/Replication/EntityReplicator.cpp
        Core/Network/Replication/EntityReplicator.h
        Core/Network/Client/EntityInterpolator.cpp
        Core/Network/Client/EntityInterpolator.h
        Core/World/IWorldGenerator.h
        Core/Network/Transport/ENetTransport.cpp
        Core/Network/Transport/ENetTransport.h
)

target_include_directories(EngineCore
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:>
)

target_compile_definitions(EngineCore
        PUBLIC
        NOMINMAX
        ENGINE_EXPORTS
)

target_link_libraries(EngineCore
        PUBLIC
        nlohmann_json::nlohmann_json
        enet
)

if(WIN32)
    target_link_libraries(EngineCore PRIVATE ws2_32 winmm)
endif()

target_compile_features(EngineCore PUBLIC cxx_std_20)

set_target_properties(EngineCore PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

#  Engine (Windows client — DX11 + GLFW + ImGui) 
# Only built when NOT doing a server-only build

if(NOT VOXTAU_SERVER_ONLY)
    # Client-only dependencies
    FetchContent_Declare(glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG master)
    FetchContent_Declare(stb
            GIT_REPOSITORY https://github.com/nothings/stb.git
            GIT_TAG master)
    FetchContent_Declare(imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG v1.92.5)
    FetchContent_Declare(assimp
            GIT_REPOSITORY https://github.com/assimp/assimp.git
            GIT_TAG v6.0.4)

    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(glfw assimp stb imgui)
    include_directories(${stb_SOURCE_DIR})

    # ImGui static library (DX11 + GLFW backends)
    add_library(imgui STATIC
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
    )
    target_include_directories(imgui PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
    )
    target_link_libraries(imgui PRIVATE glfw)

    # Engine library — create it THEN link to it
    add_library(Engine STATIC
            # Application + Input
            Core/Application.cpp
            Core/Input/InputManager.cpp

            # Scene system (client version with renderer refs)
            Core/Scene/Scene.cpp
            Core/Scene/Camera.cpp
            Core/Scene/SceneManager.cpp

            # Player controller (needs InputManager)
            Core/Entity/Living/Player/PlayerController.cpp

            # Renderer
            Renderer/RenderApi/DirectX11/DirectX11Renderer.cpp
            Renderer/RenderApi/DirectX11/DirectX11Mesh.cpp
            Renderer/RenderApi/DirectX11/DirectX11Texture.cpp
            Renderer/RenderApi/DirectX11/DirectX11TextureArray.cpp
            Renderer/Shaders/Shader.cpp
            Renderer/Shaders/ShaderCollection.cpp
            Renderer/Shaders/ShaderProgram.cpp
            Renderer/Voxel/VoxelRenderer.cpp
            Renderer/Sky/SkyRenderer.cpp
            Renderer/Debug/DebugLineRenderer.cpp
            Renderer/Entity/EntityRenderer.cpp
            Renderer/PostProcess/PostProcessPipeline.cpp
            Renderer/RenderPipeline.cpp
            Renderer/RenderTarget.cpp

            # Resources (rendering-dependent)
            Resources/ResourceManager.cpp
            Resources/Mesh.cpp
            Resources/MeshGenerator.cpp
            Resources/Texture.cpp
            Resources/TextureData.cpp
            Resources/TextureArray.cpp
            Resources/TextureGenerator.cpp
            Resources/VoxelMesh.cpp
            Resources/SkinnedMesh.cpp
            Resources/ModelLoader/ModelLoader.cpp
            Resources/ModelLoader/Vox/VoxLoader.cpp
            Resources/ModelLoader/Vox/VoxMesher.cpp
            Resources/Animation/Skeleton.cpp
            Resources/Animation/AnimationClip.cpp
            Resources/Animation/Animator.cpp

            # UI
            UI/Debug/ImGuiManager.cpp
            UI/Debug/SceneHierarchyWindow.cpp
            UI/Debug/PerformanceWindow.cpp
            UI/Debug/ProfilerWindow.cpp
            UI/Debug/CompassWindow.cpp

            # Demo
            Demo/DemoGameplayScene.cpp
    )

    target_include_directories(Engine
            PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<INSTALL_INTERFACE:>
    )

    target_compile_definitions(Engine
            PUBLIC
            ENGINE_EXPORTS
            UNICODE
            _UNICODE
            VOXTAU_CLIENT=1
    )

    target_link_libraries(Engine
            PUBLIC
            EngineCore
            glfw
            PRIVATE
            d3d11
            dxgi
            d3dcompiler
            imgui
            assimp
    )

    target_compile_features(Engine PUBLIC cxx_std_20)

    file(GLOB SHADER_FILES "Renderer/Shaders/*.hlsl")
    target_sources(Engine PRIVATE ${SHADER_FILES})

    add_custom_target(copy_assets
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_LIST_DIR}/Assets ${CMAKE_BINARY_DIR}/bin/Assets
    )
    add_dependencies(Engine copy_assets)

    set_target_properties(Engine PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    )
endif()