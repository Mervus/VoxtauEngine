//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_APPLICATION_H
#define DIRECTX11_APPLICATION_H
#include <chrono>
#include <memory>
#include <EngineApi.h>

#include "GLFW/glfw3.h"
#include "Input/InputManager.h"
#include "Renderer/RenderApi/IRendererApi.h"
#include "Renderer/Shaders/ShaderCollection.h"
#include "Scene/SceneManager.h"

#include "UI/Debug/ImGuiManager.h"
#include "UI/Debug/PerformanceWindow.h"
#include "UI/Debug/ProfilerWindow.h"
#include "UI/Debug/SceneHierarchyWindow.h"
#include "UI/Debug/CompassWindow.h"
#include "Renderer/RenderTarget.h"
#include "Resources/Mesh.h"

class VoxelShadowSystem;

class Application
{
private:
    void* windowHandle;
    int windowWidth;
    int windowHeight;
    bool isRunning;
    bool _vsync;

    // Core systems
    std::unique_ptr<IRendererApi> renderer;
    std::unique_ptr<ShaderCollection> shaderCollection;
    std::unique_ptr<ResourceManager> resourceManager;
    std::unique_ptr<SceneManager> sceneManager;

    // ImGui
    std::unique_ptr<ImGuiManager> imguiManager;
    std::unique_ptr<SceneHierarchyWindow> sceneHierarchyWindow;
    std::unique_ptr<PerformanceWindow> performanceWindow;
    std::unique_ptr<ProfilerWindow> profilerWindow;
    std::unique_ptr<CompassWindow> compassWindow;

    std::unique_ptr<InputManager> inputManager;

    // Timing
    float deltaTime;
    float lastFrameTime;
public:
    Application(const std::string &title);
    ~Application();

    bool Initialize();
    void Run();
    void Shutdown();

    void Update();
    void Render();

    void SetVsync(bool vsync);
    [[nodiscard]] SceneManager* GetSceneManager() const { return sceneManager.get(); }
    [[nodiscard]] InputManager* GetInputManager() const { return inputManager.get(); }
private:
    static void HandleResize(
        GLFWwindow* window,
        int32_t width,
        int32_t height);
    virtual void OnResize(
        int32_t width,
        int32_t height);


    void Cleanup();

    [[nodiscard]] GLFWwindow* GetWindow() const;
    [[nodiscard]] int32_t GetWindowWidth() const;
    [[nodiscard]] int32_t GetWindowHeight() const;

    int32_t _width = 0;
    int32_t _height = 0;

    std::chrono::high_resolution_clock::time_point _currentTime;
    GLFWwindow* _window = nullptr;

    std::string_view _title;

    bool InitializeCustomShaderCollection();
    bool InitializeGLFW();
};


#endif //DIRECTX11_APPLICATION_H