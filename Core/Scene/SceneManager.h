//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_SCENEMANAGER_H
#define DIRECTX11_SCENEMANAGER_H
#pragma once
#include "Scene.h"
#include <map>
#include <string>
#include <functional>
#include <EngineApi.h>

#include "Core/Input/InputManager.h"

class IRendererApi;
class ShaderCollection;
class ResourceManager;

class ENGINE_API SceneManager {
private:
    std::map<std::string, Scene*> scenes;
    Scene* currentScene;
    Scene* nextScene;
    bool isTransitioning;

    // Core systems (passed to scenes)
    IRendererApi* renderer;
    ShaderCollection* shaderCollection;
    ResourceManager* resourceManager;
    InputManager* inputManager;

    // Callback when scene is loaded
    std::function<void(Scene*)> onSceneLoaded;

    void PerformTransition();

public:
    SceneManager(IRendererApi* renderer, ShaderCollection* shaders, ResourceManager* resources, InputManager* inputManager);
    ~SceneManager();

    // Scene management
    void AddScene(const std::string& name, Scene* scene);
    void RemoveScene(const std::string& name);
    Scene* GetScene(const std::string& name);

    // Scene switching
    void LoadScene(const std::string& name);
    void LoadSceneAsync(const std::string& name); // For future async loading

    // Current scene
    [[nodiscard]] Scene* GetCurrentScene() const { return currentScene; }

    // Set callback for when a scene is loaded
    void SetOnSceneLoaded(std::function<void(Scene*)> callback) { onSceneLoaded = callback; }

    // Update/Render
    void Update(float deltaTime);
    void LateUpdate(float deltaTime);
    void Render(float deltaTime);
};


#endif //DIRECTX11_SCENEMANAGER_H