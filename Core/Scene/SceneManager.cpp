//
// Created by Marvin on 28/01/2026.
//

#include "SceneManager.h"
#include <iostream>

SceneManager::SceneManager(IRendererApi* renderer, ShaderCollection* shaders, ResourceManager* resources, InputManager* inputManager)
    : currentScene(nullptr)
    , nextScene(nullptr)
    , isTransitioning(false)
    , renderer(renderer)
    , shaderCollection(shaders)
    , resourceManager(resources)
    , inputManager(inputManager)
{
}

SceneManager::~SceneManager() {
    // Delete all scenes
    for (auto& pair : scenes) {
        delete pair.second;
    }
    scenes.clear();

    currentScene = nullptr;
    nextScene = nullptr;
}

void SceneManager::AddScene(const std::string& name, Scene* scene) {
    if (!scene) return;

    // Set system references
    scene->SetSystems(renderer, shaderCollection, resourceManager, inputManager);

    // Add to map
    scenes[name] = scene;

    std::cout << "Scene added: " << name << std::endl;
}

void SceneManager::RemoveScene(const std::string& name) {
    auto it = scenes.find(name);
    if (it != scenes.end()) {
        // Don't delete if it's the current scene
        if (it->second == currentScene) {
            std::cerr << "Cannot remove active scene: " << name << std::endl;
            return;
        }

        delete it->second;
        scenes.erase(it);

        std::cout << "Scene removed: " << name << std::endl;
    }
}

Scene* SceneManager::GetScene(const std::string& name) {
    auto it = scenes.find(name);
    if (it != scenes.end()) {
        return it->second;
    }
    return nullptr;
}

void SceneManager::LoadScene(const std::string& name) {
    Scene* scene = GetScene(name);

    if (!scene) {
        std::cerr << "Scene not found: " << name << std::endl;
        return;
    }

    // If already the current scene, do nothing
    if (scene == currentScene) {
        return;
    }

    std::cout << "Loading scene: " << name << std::endl;

    // Deactivate current scene
    if (currentScene) {
        currentScene->SetActive(false);
    }

    // Initialize new scene if needed
    if (!scene->IsInitialized()) {
        scene->Initialize();
    }

    // Switch to new scene
    currentScene = scene;
    currentScene->SetActive(true);

    // Notify listeners
    if (onSceneLoaded) {
        onSceneLoaded(currentScene);
    }

    std::cout << "Scene loaded: " << name << std::endl;
}

void SceneManager::LoadSceneAsync(const std::string& name) {
    // For now, just load synchronously
    // TODO: Implement async loading with loading screen
    LoadScene(name);
}

void SceneManager::Update(float deltaTime) {
    if (currentScene) {
        currentScene->Update(deltaTime);
    }
}

void SceneManager::LateUpdate(float deltaTime) {
    if (currentScene) {
        currentScene->LateUpdate(deltaTime);
    }
}

void SceneManager::Render(float deltaTime) {
    if (currentScene) {
        currentScene->Render(deltaTime);
    }
}