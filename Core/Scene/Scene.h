//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_SCENE_H
#define DIRECTX11_SCENE_H
#include <EngineApi.h>

#include <string>
#include <vector>

#include "Camera.h"
#include "Core/Entity/Entity.h"
#include "Core/Input/InputManager.h"
#include "Core/Math/Vector3.h"
#include "Core/Network/NetContext.h"
#include "Core/Voxel/Voxel.h"

class IRendererApi;
class ShaderCollection;
class ResourceManager;
class ChunkManager;

class Scene {
protected:
    enum class SceneState { Connecting, Loading, InGame };
    std::string name;
    Camera* mainCamera;
    bool isActive;
    bool isInitialized;
    SceneState _sceneState = SceneState::Connecting;

    // Core systems (references, not owned by scene)
    IRendererApi* renderer;
    ShaderCollection* shaderCollection;
    ResourceManager* resourceManager;
    InputManager* inputManager;
    ClientSession* clientSession;

    float _totalTime = 0.0f;
public:
    Scene(const std::string& name);
    virtual ~Scene();

    // Lifecycle methods (override in derived scenes)
    virtual void OnInit() {}       // Called once when scene is created
    virtual void OnEnter() {}      // Called when scene becomes active
    virtual void OnExit() {}       // Called when scene becomes inactive
    virtual void OnDestroy() {}    // Called when scene is destroyed

    virtual void Update(float deltaTime);
    virtual void LateUpdate(float deltaTime);
    virtual void Render(float deltaTime);

    // Camera management
    void SetMainCamera(Camera* camera);
    [[nodiscard]] Camera* GetMainCamera() const;

    // System references (set by SceneManager)
    void SetSystems(IRendererApi* renderer, ShaderCollection* shaders, ResourceManager* resources, InputManager* input, ClientSession* clientSession);

    // State
    [[nodiscard]] bool IsActive() const { return isActive; }
    [[nodiscard]] bool IsInitialized() const { return isInitialized; }
    [[nodiscard]] const std::string& GetName() const { return name; }

    // Internal use by SceneManager
    void Initialize();
    void SetActive(bool active);

    [[nodiscard]] InputManager* GetInputManager() const { return inputManager; }
    [[nodiscard]] IRendererApi* GetRenderer() const { return renderer; }
    [[nodiscard]] ShaderCollection* GetShaderCollection() const { return shaderCollection; }
    // Virtual methods for shadow system integration
    virtual ChunkManager* GetChunkManager() const { return nullptr; }
    virtual Math::Vector3 GetSunDirection() const { return Math::Vector3(0.3f, 0.7f, 0.5f).Normalized(); }
};

#endif //DIRECTX11_SCENE_H