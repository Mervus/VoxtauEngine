//
// Created by Marvin on 29/01/2026.
//

#ifndef DIRECTX11_INPUTMANAGER_H
#define DIRECTX11_INPUTMANAGER_H
#pragma once
#include <GLFW/glfw3.h>
#include <unordered_map>

enum class KeyCode {
    W, A, S, D,
    Q, E, O,
    F3,
    Space, LeftShift,
    Escape,
    LeftMouse, RightMouse
};

class InputManager {
private:
    GLFWwindow* window;

    std::unordered_map<KeyCode, int> keyMap;

    double lastMouseX;
    double lastMouseY;
    double mouseDeltaX;
    double mouseDeltaY;
    bool firstMouse;
    bool mouseCaptured;

    void InitializeKeyMap();

public:
    InputManager(GLFWwindow* window);

    void Update();

    // Keyboard
    bool IsKeyPressed(KeyCode key) const;
    bool IsKeyDown(KeyCode key) const;
    bool IsKeyReleased(KeyCode key) const;

    // Mouse
    void GetMousePosition(double& x, double& y) const;
    void GetMouseDelta(double& deltaX, double& deltaY) const;
    bool IsMouseCaptured() const { return mouseCaptured; }
    void SetMouseCaptured(bool captured);
};


#endif //DIRECTX11_INPUTMANAGER_H