//
// Created by Marvin on 28/01/2026.
//

#include "Scene.h"
#include <algorithm>


Scene::Scene(const std::string& name)
    : name(name)
    , mainCamera(nullptr)
    , isActive(false)
    , isInitialized(false)
    , renderer(nullptr)
    , shaderCollection(nullptr)
    , resourceManager(nullptr)
{
}

Scene::~Scene() {
    OnDestroy();

    mainCamera = nullptr;
}

void Scene::Initialize() {
    if (isInitialized) return;

    OnInit();

    isInitialized = true;
}

void Scene::SetActive(bool active) {
    if (isActive == active) return;

    isActive = active;

    if (active) {
        OnEnter();

    } else {
        OnExit();

    }
}

void Scene::Update(float deltaTime) {
    if (!isActive) return;

}

void Scene::LateUpdate(float deltaTime) {
    if (!isActive) return;

}

void Scene::Render(float deltaTime) {
    if (!isActive) return;

}

void Scene::SetMainCamera(Camera* camera) {
    mainCamera = camera;
}

Camera* Scene::GetMainCamera() const {
    return mainCamera;
}

void Scene::SetSystems(IRendererApi* renderer, ShaderCollection* shaders, ResourceManager* resources, InputManager* input) {
    this->renderer = renderer;
    this->shaderCollection = shaders;
    this->resourceManager = resources;
    this->inputManager = input;
}