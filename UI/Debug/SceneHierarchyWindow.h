//
// Created by Marvin on 29/01/2026.
//

#ifndef DIRECTX11_SCENEHIERARCHYWINDOW_H
#define DIRECTX11_SCENEHIERARCHYWINDOW_H
#pragma once
#include "Core/Scene/Scene.h"

class SceneHierarchyWindow {
private:
    Scene* currentScene;
    //GameObject* selectedObject;
    bool isOpen;

    void DrawGameObject(Voxel* obj);
    void DrawComponentInspector(Voxel* obj);

public:
    SceneHierarchyWindow();

    void SetScene(Scene* scene);
    void Draw();

    void SetOpen(bool open) { isOpen = open; }
    bool IsOpen() const { return isOpen; }

    //GameObject* GetSelectedObject() const { return selectedObject; }
};


#endif //DIRECTX11_SCENEHIERARCHYWINDOW_H