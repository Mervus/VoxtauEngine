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
    : _windowHandle(nullptr)
      , _windowWidth(0)
      , _windowHeight(0)
      , _isRunning(false)
      , _deltaTime(0.0f)
      , _lastFrameTime(0.0f)
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
    std::cout << "[Client] Initializing Application..." << std::endl;

    // Create GLFW window FIRST
    if (!InitializeGLFW()) {
        std::cerr << "[Client] Failed to initialize GLFW!" << std::endl;
        return false;
    }

    // Now get the window handle AFTER the window exists
    this->_windowHandle = glfwGetWin32Window(_window);
    this->_windowWidth = _width;
    this->_windowHeight = _height;

    // INITIALIZE RENDERER
    _renderer = std::make_unique<DirectX11Renderer>();
    if (!_renderer->Initialize(_windowHandle, _width, _height)) {
        std::cerr << "[Client] Failed to initialize renderer!" << std::endl;
        return false;
    }

    _netContext = std::make_unique<NetContext>();
    _inputManager = std::make_unique<InputManager>(_window);

    // INITIALIZE SHADERS
    _shaderCollection = std::make_unique<ShaderCollection>(_renderer.get());
    _shaderCollection->Initialize();

    // INITIALIZE RESOURCE MANAGER
    _resourceManager = std::make_unique<ResourceManager>(_renderer.get(), _shaderCollection.get());

    assert(_netContext.get()->GetClient());

    // INITIALIZE SCENE MANAGER
    _sceneManager = std::make_unique<SceneManager>(
                _renderer.get(),
                _shaderCollection.get(),
                _resourceManager.get(),
                _inputManager.get(),
                _netContext.get()->GetClient() //nullptr
                );


    _imguiManager = std::make_unique<ImGuiManager>();
    if (!_imguiManager->Initialize(_window, _renderer->GetDevice(), _renderer->GetContext())) {
        std::cerr << "[Client] Failed to initialize ImGui!" << std::endl;
        return false;
    }

    _sceneHierarchyWindow = std::make_unique<SceneHierarchyWindow>();
    _performanceWindow = std::make_unique<PerformanceWindow>();
    _profilerWindow = std::make_unique<ProfilerWindow>();
    _compassWindow = std::make_unique<CompassWindow>();

    // Bind scene hierarchy update to scene loading
    _sceneManager->SetOnSceneLoaded([this](Scene* scene) {
        _sceneHierarchyWindow->SetScene(scene);
        _sceneHierarchyWindow->SetOpen(true);
    });


    std::cout << "[Client] Application initialized successfully!" << std::endl;
    _isRunning = true;
    return true;
}

void Application::Run()
{
    if (!_isRunning)
    {
        if (!Initialize())
        {
            return;
        }
    }

    while (!glfwWindowShouldClose(_window))
    {
        glfwPollEvents();

        _renderer->BeginFrame();
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
    _deltaTime = static_cast<float>(timeSpan.count() / 1000.0);

    // Begin profiler frame
    Profiler::Instance().BeginFrame();

    if (_inputManager) {
        _inputManager->Update();
    }

    // Update current scene
    _sceneManager->Update(_deltaTime);
    _performanceWindow->Update(_deltaTime);

    //TODO: shouldnt it tick consistently?
    _netContext->Tick(_deltaTime);

    // Update compass with camera forward direction
    Scene* activeScene = _sceneManager->GetCurrentScene();
    if (activeScene && activeScene->GetMainCamera()) {
        _compassWindow->Update(activeScene->GetMainCamera()->GetForward());
    }

    _sceneManager->LateUpdate(_deltaTime);

    glfwPollEvents();
}

void Application::Render()
{
    PROFILE_FUNCTION();

    // PASS 1: Main Scene Render
    {
        PROFILE_SCOPE("Scene Render");
        // Ensure we're rendering to the backbuffer
        _renderer->SetDefaultRenderTarget();

        // Clear screen
        _renderer->Clear(0.1f, 0.2f, 0.3f, 1.0f);
        _renderer->ClearDepth();

        // Set render states for scene
        _renderer->SetRasterizerMode(RasterizerMode::CullBack);
        _renderer->SetBlendMode(BlendMode::None);
        _renderer->SetDepthMode(DepthMode::Less);
        _renderer->SetDepthTestEnabled(true);
        _renderer->SetDepthWriteEnabled(true);

        // Render current scene
        _sceneManager->Render(_deltaTime);
    }

    // PASS 3: ImGui Overlay
    {
        PROFILE_SCOPE("ImGui");
        _imguiManager->BeginFrame();
        _sceneHierarchyWindow->Draw();
        _performanceWindow->Draw();
        _profilerWindow->Draw();
        _compassWindow->Draw();
        _imguiManager->EndFrame();
        _imguiManager->Render();
    }

    // Present
    _renderer->Present(_vsync);
}

void Application::Shutdown() {
    std::cout << "Shutting down Application..." << std::endl;

    _sceneManager.reset();
    _resourceManager.reset();
    _shaderCollection.reset();
    _imguiManager.reset();
    _sceneHierarchyWindow.reset();
    _performanceWindow.reset();
    _profilerWindow.reset();
    _compassWindow.reset();
    _inputManager.reset();
    _renderer.reset();

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

    if (_renderer) {
        _renderer->ResizeBuffers(width, height);
    }

    if (_sceneManager) {
        Scene* scene = _sceneManager->GetCurrentScene();
        if (scene && scene->GetMainCamera()) {
            scene->GetMainCamera()->SetAspectRatio(
                static_cast<float>(width) / static_cast<float>(height));
        }
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
    _shaderCollection = std::make_unique<ShaderCollection>(_renderer.get());
    _shaderCollection->Initialize(); // Loads all common shaders

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
