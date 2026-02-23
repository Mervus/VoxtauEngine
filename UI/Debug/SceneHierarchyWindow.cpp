//
// Created by Marvin on 29/01/2026.
//

#include "SceneHierarchyWindow.h"

#include <imgui.h>
#include "Core/Math/MathUtils.h"

SceneHierarchyWindow::SceneHierarchyWindow()
    : currentScene(nullptr)
    , isOpen(true)
{
}

void SceneHierarchyWindow::SetScene(Scene* scene) {
    currentScene = scene;
}

void SceneHierarchyWindow::Draw() {
    if (!isOpen || !currentScene) return;

    ImGui::Begin("Scene Hierarchy", &isOpen);

    // Scene name
    ImGui::Text("Scene: %s", currentScene->GetName().c_str());
    ImGui::Separator();

    // List all game objects
    ImGui::BeginChild("Voxels", ImVec2(0, 300), true);

    ImGui::EndChild();

    ImGui::Separator();


    ImGui::End();
}

void SceneHierarchyWindow::DrawGameObject(Voxel* obj) {
    if (!obj) return;

    // Flags for tree node
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    // Highlight if selected

    // Leaf node (no children for now)
    flags |= ImGuiTreeNodeFlags_Leaf;

    // Draw tree node
    bool opened = ImGui::TreeNodeEx("V", flags);

    // Check if clicked

    if (opened) {
        ImGui::TreePop();
    }
}

void SceneHierarchyWindow::DrawComponentInspector(Voxel* obj) {
    if (!obj) return;

    // Active checkbox
    bool active = obj->IsActive();
    if (ImGui::Checkbox("Active", &active)) {
        obj->SetActive(active);
    }

    ImGui::Separator();



    ImGui::Separator();
}