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
#include "Network/NetContext.h"
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
    void* _windowHandle;
    int _windowWidth;
    int _windowHeight;
    bool _isRunning;
    bool _vsync;

    // Core systems
    std::unique_ptr<IRendererApi> _renderer;
    std::unique_ptr<ShaderCollection> _shaderCollection;
    std::unique_ptr<ResourceManager> _resourceManager;
    std::unique_ptr<SceneManager> _sceneManager;
    std::unique_ptr<NetContext> _netContext;

    // ImGui
    std::unique_ptr<ImGuiManager> _imguiManager;
    std::unique_ptr<SceneHierarchyWindow> _sceneHierarchyWindow;
    std::unique_ptr<PerformanceWindow> _performanceWindow;
    std::unique_ptr<ProfilerWindow> _profilerWindow;
    std::unique_ptr<CompassWindow> _compassWindow;

    std::unique_ptr<InputManager> _inputManager;

    // Timing
    float _deltaTime;
    float _lastFrameTime;
public:
    Application(const std::string &title);
    ~Application();

    bool Initialize();
    void Run();
    void Shutdown();

    void Update();
    void Render();

    void SetVsync(bool vsync);
    [[nodiscard]] SceneManager* GetSceneManager() const { return _sceneManager.get(); }
    [[nodiscard]] InputManager* GetInputManager() const { return _inputManager.get(); }
    [[nodiscard]] ResourceManager* GetResourceManager() const { return _resourceManager.get(); }
    [[nodiscard]] NetContext* GetNetContext() const { return _netContext.get(); }

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