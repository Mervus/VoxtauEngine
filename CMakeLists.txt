add_library(Engine STATIC
    Engine.cpp
    EngineApi.h
        Core/Application.cpp
        Core/Application.h
        Renderer/RenderApi/IRendererApi.h
        Renderer/RenderApi/DirectX11/DirectX11Renderer.cpp
        Renderer/RenderApi/DirectX11/DirectX11Renderer.h
        Resources/ResourceManager.cpp
        Resources/ResourceManager.h
        Renderer/Shaders/ShaderCollection.cpp
        Renderer/Shaders/ShaderCollection.h
        Renderer/Shaders/Shader.cpp
        Renderer/Shaders/Shader.h
        Renderer/Shaders/ShaderProgram.cpp
        Renderer/Shaders/ShaderProgram.h
        Renderer/Shaders/ShaderTypes.h
        Renderer/Vertex.h
        Renderer/RenderApi/DirectX11/DirectX11Mesh.h
        Renderer/RenderApi/DirectX11/DirectX11Mesh.cpp
        Renderer/RenderApi/DirectX11/DirectX11Texture.cpp
        Renderer/RenderApi/DirectX11/DirectX11Texture.h
        Renderer/RenderApi/DirectX11/DirectX11TextureArray.cpp
        Renderer/RenderApi/DirectX11/DirectX11TextureArray.h
        Resources/TextureData.cpp
        Resources/TextureData.h
        Resources/TextureArray.cpp
        Resources/TextureArray.h
        Resources/BlockRegistry.cpp
        Resources/BlockRegistry.h
        Renderer/RenderTarget.cpp
        Renderer/RenderTarget.h
        Resources/Mesh.cpp
        Resources/Mesh.h
        Resources/Texture.cpp
        Resources/Texture.h
        Core/Math/MathTypes.h
        Core/Math/Vector2.cpp
        Core/Math/Vector2.h
        Core/Math/Vector3.cpp
        Core/Math/Vector3.h
        Core/Math/Vector4.cpp
        Core/Math/Vector4.h
        Core/Math/Matrix4x4.cpp
        Core/Math/Matrix4x4.h
        Core/Math/Quaternion.cpp
        Core/Math/Quaternion.h
        Core/Math/Frustum.cpp
        Core/Math/Frustum.h
        Core/Math/MathUtils.h
        Renderer/RenderStates.h
        Resources/MeshGenerator.cpp
        Resources/MeshGenerator.h
        Resources/TextureGenerator.cpp
        Resources/TextureGenerator.h
        Core/Scene/Camera.cpp
        Core/Scene/Camera.h
        Core/Scene/Scene.cpp
        Core/Scene/Scene.h
        Core/Scene/Transform.cpp
        Core/Scene/Transform.h
        Core/Scene/SceneManager.cpp
        Core/Scene/SceneManager.h
        UI/Debug/ImGuiManager.cpp
        UI/Debug/ImGuiManager.h
        UI/Debug/SceneHierarchyWindow.cpp
        UI/Debug/SceneHierarchyWindow.h
        UI/Debug/PerformanceWindow.cpp
        UI/Debug/PerformanceWindow.h
        UI/Debug/CompassWindow.cpp
        UI/Debug/CompassWindow.h
        Core/Input/InputManager.cpp
        Core/Input/InputManager.h
        Core/Voxel/VoxelType.cpp
        Core/Voxel/VoxelType.h
        Core/Math/Noise/PerlinNoise.cpp
        Core/Math/Noise/PerlinNoise.h
        Core/Procedural/IStructureGenerator.h
        Core/Procedural/StructurePlacer.cpp
        Core/Procedural/StructurePlacer.h
        Core/Voxel/ChunkManager.cpp
        Core/Voxel/ChunkManager.h
        Core/Profiler/Profiler.cpp
        Core/Profiler/Profiler.h
        UI/Debug/ProfilerWindow.cpp
        UI/Debug/ProfilerWindow.h
        Core/Voxel/Voxel.cpp
        Core/Voxel/Voxel.h
        Core/Voxel/Block.cpp
        Core/Voxel/Block.h
        Core/Voxel/Chunk.cpp
        Core/Voxel/Chunk.h
        Renderer/Debug/DebugLineRenderer.cpp
        Renderer/Debug/DebugLineRenderer.h
        Renderer/Voxel/VoxelRenderer.cpp
        Renderer/Voxel/VoxelRenderer.h
        Resources/VoxelMesh.cpp
        Resources/VoxelMesh.h
        Core/Entity/Entity.cpp
        Core/Entity/Entity.h
        Core/Entity/EntityID.h
        Core/Entity/EntityManager.cpp
        Core/Entity/EntityManager.h
        Core/Entity/EntityType.h
        Core/Entity/Living/LivingEntity.cpp
        Core/Entity/Living/LivingEntity.h
        Core/Entity/Living/Player/PlayerEntity.cpp
        Core/Entity/Living/Player/PlayerEntity.h
        Resources/ModelLoader/Vox/VoxLoader.cpp
        Resources/ModelLoader/Vox/VoxLoader.h
        Resources/ModelLoader/Vox/VoxModel.h
        Resources/ModelLoader/Vox/VoxMesher.cpp
        Resources/ModelLoader/Vox/VoxMesher.h
        Core/Entity/Living/Player/PlayerController.cpp
        Core/Entity/Living/Player/PlayerController.h
        Core/Physics/Voxtau/AABB.h
        Core/Physics/Voxtau/VoxelBody.h
        Core/Physics/Voxtau/VoxelPhysics.cpp
        Core/Physics/Voxtau/VoxelPhysics.h
        Renderer/Sky/SkyRenderer.cpp
        Renderer/Sky/SkyRenderer.h
        Renderer/PostProcess/PostProcessPipeline.cpp
        Renderer/PostProcess/PostProcessPipeline.h
        Renderer/RenderPipeline.cpp
        Renderer/RenderPipeline.h
        Resources/ModelLoader/ModelLoader.cpp
        Resources/ModelLoader/ModelLoader.h
        Resources/SkinnedMesh.cpp
        Resources/SkinnedMesh.h
        Resources/Animation/Skeleton.cpp
        Resources/Animation/Skeleton.h
        Resources/Animation/AnimationClip.cpp
        Resources/Animation/AnimationClip.h
        Resources/Animation/Animator.cpp
        Resources/Animation/Animator.h
        Renderer/Entity/EntityRenderer.cpp
        Renderer/Entity/EntityRenderer.h

)


file(GLOB SHADER_FILES "Renderer/Shaders/*.hlsl")

target_sources(Engine
        PRIVATE

        ${SHADER_FILES}
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS src
)

target_sources(Engine
        PRIVATE
        EngineApi.h
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
        NOMINMAX
        PRIVATE
        ENGINE_EXPORT
)

if(WIN32)
    target_link_libraries(Engine
            PUBLIC
            glfw
            PRIVATE
            d3d11
            dxgi
            d3dcompiler
            imgui
            nlohmann_json::nlohmann_json
            assimp
    )
endif()

add_custom_target(copy_assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/Assets ${CMAKE_BINARY_DIR}/bin/Assets
)
add_dependencies(Engine copy_assets)

set_target_properties(Engine PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        WINDOWS_EXPORT_ALL_SYMBOLS OFF  # Use explicit exports
)