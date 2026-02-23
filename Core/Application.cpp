//
// Created by Marvin on 28/01/2026.
//

#include "Application.h"

#include "Renderer/RenderApi/DirectX11/DirectX11Renderer.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "imgui.h"
#include "Math/MathUtils.h"
#include "Profiler/Profiler.h"
#include "Scene/Scene.h"
#include "Scene/Camera.h"

bool InitializeCustomShaderCollection();
bool InitializeGLFW();
void Update();
void Cleanup();

Application::Application(const std::string &title)
    : windowHandle(nullptr)
      , windowWidth(0)
      , windowHeight(0)
      , isRunning(false)
      , deltaTime(0.0f)
      , lastFrameTime(0.0f)
      , _vsync(false)
{
    _title = title;
}

Application::~Application()
{
    Application::Shutdown();
    Application::Cleanup();
}

bool Application::Initialize() {
    std::cout << "Initializing Application..." << std::endl;

    // Create GLFW window FIRST
    if (!InitializeGLFW()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return false;
    }

    // Now get the window handle AFTER the window exists
    this->windowHandle = glfwGetWin32Window(_window);
    this->windowWidth = _width;
    this->windowHeight = _height;

    // INITIALIZE RENDERER
    renderer = std::make_unique<DirectX11Renderer>();
    if (!renderer->Initialize(windowHandle, _width, _height)) {
        std::cerr << "Failed to initialize renderer!" << std::endl;
        return false;
    }

    inputManager = std::make_unique<InputManager>(_window);

    // INITIALIZE SHADERS
    shaderCollection = std::make_unique<ShaderCollection>(renderer.get());
    shaderCollection->Initialize();

    // INITIALIZE RESOURCE MANAGER
    resourceManager = std::make_unique<ResourceManager>(renderer.get(), shaderCollection.get());

    // INITIALIZE SCENE MANAGER
    sceneManager = std::make_unique<SceneManager>(renderer.get(), shaderCollection.get(), resourceManager.get(), inputManager.get());

    imguiManager = std::make_unique<ImGuiManager>();
    if (!imguiManager->Initialize(_window, renderer->GetDevice(), renderer->GetContext())) {
        std::cerr << "Failed to initialize ImGui!" << std::endl;
        return false;
    }

    sceneHierarchyWindow = std::make_unique<SceneHierarchyWindow>();
    performanceWindow = std::make_unique<PerformanceWindow>();
    profilerWindow = std::make_unique<ProfilerWindow>();
    compassWindow = std::make_unique<CompassWindow>();

    // Bind scene hierarchy update to scene loading
    sceneManager->SetOnSceneLoaded([this](Scene* scene) {
        sceneHierarchyWindow->SetScene(scene);
        sceneHierarchyWindow->SetOpen(true);
    });


    std::cout << "Application initialized successfully!" << std::endl;
    isRunning = true;
    return true;
}

void Application::Run()
{
    if (!isRunning)
    {
        if (!Initialize())
        {
            return;
        }
    }

    while (!glfwWindowShouldClose(_window))
    {
        glfwPollEvents();

        renderer->BeginFrame();
        Update();                            // Game logic + draw calls
        Application::Render();
    }
}

void Application::Update()
{
    PROFILE_FUNCTION();

    auto oldTime = _currentTime;
    _currentTime = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> timeSpan = (_currentTime - oldTime);
    deltaTime = static_cast<float>(timeSpan.count() / 1000.0);

    // Begin profiler frame
    Profiler::Instance().BeginFrame();

    if (inputManager) {
        inputManager->Update();
    }

    // Update current scene
    sceneManager->Update(deltaTime);
    performanceWindow->Update(deltaTime);

    // Update compass with camera forward direction
    Scene* activeScene = sceneManager->GetCurrentScene();
    if (activeScene && activeScene->GetMainCamera()) {
        compassWindow->Update(activeScene->GetMainCamera()->GetForward());
    }

    sceneManager->LateUpdate(deltaTime);

    glfwPollEvents();
}

void Application::Render()
{
    PROFILE_FUNCTION();

    // PASS 1: Main Scene Render
    {
        PROFILE_SCOPE("Scene Render");
        // Ensure we're rendering to the backbuffer
        renderer->SetDefaultRenderTarget();

        // Clear screen
        renderer->Clear(0.1f, 0.2f, 0.3f, 1.0f);
        renderer->ClearDepth();

        // Set render states for scene
        renderer->SetRasterizerMode(RasterizerMode::CullBack);
        renderer->SetBlendMode(BlendMode::None);
        renderer->SetDepthMode(DepthMode::Less);
        renderer->SetDepthTestEnabled(true);
        renderer->SetDepthWriteEnabled(true);

        // Render current scene
        sceneManager->Render(deltaTime);
    }

    // PASS 3: ImGui Overlay
    {
        PROFILE_SCOPE("ImGui");
        imguiManager->BeginFrame();
        sceneHierarchyWindow->Draw();
        performanceWindow->Draw();
        profilerWindow->Draw();
        compassWindow->Draw();
        imguiManager->EndFrame();
        imguiManager->Render();
    }

    // Present
    renderer->Present(_vsync);
}

void Application::Shutdown() {
    std::cout << "Shutting down Application..." << std::endl;

    sceneManager.reset();
    resourceManager.reset();
    shaderCollection.reset();
    imguiManager.reset();
    sceneHierarchyWindow.reset();
    performanceWindow.reset();
    profilerWindow.reset();
    compassWindow.reset();
    inputManager.reset();
    renderer.reset();

    std::cout << "Application shutdown complete." << std::endl;
}

void Application::HandleResize(GLFWwindow* window, int32_t width, int32_t height)
{
    Application* application = static_cast<Application*>(glfwGetWindowUserPointer(window));
    application->OnResize(width, height);
}

void Application::OnResize(int32_t width, int32_t height)
{
    if (width == 0 || height == 0) return; // Minimized

    _width = width;
    _height = height;

    if (renderer) {
        renderer->ResizeBuffers(width, height);
    }
}

void Application::Cleanup()
{
    if (_window != nullptr)
    {
        glfwDestroyWindow(_window);
        _window = nullptr;
    }
    glfwTerminate();
}

bool Application::InitializeCustomShaderCollection()
{
    // Create shader collection
    shaderCollection = std::make_unique<ShaderCollection>(renderer.get());
    shaderCollection->Initialize(); // Loads all common shaders

    // Load any additional custom shaders
    // shaderCollection->LoadShader("custom_effect",
    //     "Assets/Shaders/Custom.vert.hlsl",
    //     "Assets/Shaders/custom.pixel.hlsl");

    return true;
}

bool Application::InitializeGLFW()
{
    if (!glfwInit())
    {
        std::cout << "GLFW: Unable to initialize\n";
        return false;
    }

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);
    _width = static_cast<int32_t>(videoMode->width * 0.9f);
    _height = static_cast<int32_t>(videoMode->height * 0.9f);

    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    _window = glfwCreateWindow(_width, _height, _title.data(), nullptr, nullptr);
    if (_window == nullptr)
    {
        std::cout << "GLFW: Unable to create window\n";
        return false;
    }

    const int32_t windowLeft = videoMode->width / 2 - _width / 2;
    const int32_t windowTop = videoMode->height / 2 - _height / 2;
    glfwSetWindowPos(_window, windowLeft, windowTop);

    glfwSetWindowUserPointer(_window, this);
    glfwSetFramebufferSizeCallback(_window, HandleResize);

    return true;
}

GLFWwindow* Application::GetWindow() const
{
    return _window;
}

int32_t Application::GetWindowWidth() const
{
    return _width;
}

int32_t Application::GetWindowHeight() const
{
    return _height;
}
