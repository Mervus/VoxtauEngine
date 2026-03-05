//
// Created by Marvin on 28/01/2026.
//

#include "SceneManager.h"
#include <iostream>

SceneManager::SceneManager(IRendererApi* renderer, ShaderCollection* shaders, ResourceManager* resources, InputManager* inputManager, ClientSession* clientSession)
    : _currentScene(nullptr)
    , _nextScene(nullptr)
    , _isTransitioning(false)
    , _renderer(renderer)
    , _shaderCollection(shaders)
    , _resourceManager(resources)
    , _inputManager(inputManager)
    , _clientSession(clientSession)
{
}

SceneManager::~SceneManager() {
    // Delete all scenes
    for (auto& pair : _scenes) {
        delete pair.second;
    }
    _scenes.clear();

    _currentScene = nullptr;
    _nextScene = nullptr;
}

void SceneManager::AddScene(const std::string& name, Scene* scene) {
    if (!scene) return;

    // Set system references
    scene->SetSystems(_renderer, _shaderCollection, _resourceManager, _inputManager, _clientSession);

    // Add to map
    _scenes[name] = scene;

    std::cout << "Scene added: " << name << std::endl;
}

void SceneManager::RemoveScene(const std::string& name) {
    auto it = _scenes.find(name);
    if (it != _scenes.end()) {
        // Don't delete if it's the current scene
        if (it->second == _currentScene) {
            std::cerr << "Cannot remove active scene: " << name << std::endl;
            return;
        }

        delete it->second;
        _scenes.erase(it);

        std::cout << "Scene removed: " << name << std::endl;
    }
}

Scene* SceneManager::GetScene(const std::string& name) {
    auto it = _scenes.find(name);
    if (it != _scenes.end()) {
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
    if (scene == _currentScene) {
        return;
    }

    std::cout << "Loading scene: " << name << std::endl;

    // Deactivate current scene
    if (_currentScene) {
        _currentScene->SetActive(false);
    }

    // Initialize new scene if needed
    if (!scene->IsInitialized()) {
        scene->Initialize();
    }

    // Switch to new scene
    _currentScene = scene;
    _currentScene->SetActive(true);

    // Notify listeners
    if (onSceneLoaded) {
        onSceneLoaded(_currentScene);
    }

    std::cout << "Scene loaded: " << name << std::endl;
}

void SceneManager::LoadSceneAsync(const std::string& name) {
    // For now, just load synchronously
    // TODO: Implement async loading with loading screen
    LoadScene(name);
}

void SceneManager::Update(float deltaTime) {
    if (_currentScene) {
        _currentScene->Update(deltaTime);
    }
}

void SceneManager::LateUpdate(float deltaTime) {
    if (_currentScene) {
        _currentScene->LateUpdate(deltaTime);
    }
}

void SceneManager::Render(float deltaTime) {
    if (_currentScene) {
        _currentScene->Render(deltaTime);
    }
}