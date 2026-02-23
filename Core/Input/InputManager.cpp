//
// Created by Marvin on 29/01/2026.
//

#include "InputManager.h"
#include <iostream>

InputManager::InputManager(GLFWwindow* window)
    : window(window)
    , lastMouseX(0.0)
    , lastMouseY(0.0)
    , mouseDeltaX(0.0)
    , mouseDeltaY(0.0)
    , firstMouse(true)
    , mouseCaptured(false)
{
    InitializeKeyMap();
}

void InputManager::InitializeKeyMap() {
    keyMap[KeyCode::W] = GLFW_KEY_W;
    keyMap[KeyCode::A] = GLFW_KEY_A;
    keyMap[KeyCode::S] = GLFW_KEY_S;
    keyMap[KeyCode::D] = GLFW_KEY_D;
    keyMap[KeyCode::Q] = GLFW_KEY_Q;
    keyMap[KeyCode::E] = GLFW_KEY_E;
    keyMap[KeyCode::O] = GLFW_KEY_O;
    keyMap[KeyCode::F3] = GLFW_KEY_F3;
    keyMap[KeyCode::Space] = GLFW_KEY_SPACE;
    keyMap[KeyCode::LeftShift] = GLFW_KEY_LEFT_SHIFT;
    keyMap[KeyCode::Escape] = GLFW_KEY_ESCAPE;
    keyMap[KeyCode::LeftMouse] = GLFW_MOUSE_BUTTON_LEFT;
    keyMap[KeyCode::RightMouse] = GLFW_MOUSE_BUTTON_RIGHT;
}

void InputManager::Update() {
    // Update mouse delta
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    if (firstMouse) {
        lastMouseX = mouseX;
        lastMouseY = mouseY;
        firstMouse = false;
    }

    mouseDeltaX = mouseX - lastMouseX;
    mouseDeltaY = lastMouseY - mouseY; // Reversed: y-coordinates go from bottom to top

    lastMouseX = mouseX;
    lastMouseY = mouseY;
}

bool InputManager::IsKeyPressed(KeyCode key) const {
    auto it = keyMap.find(key);
    if (it == keyMap.end()) return false;

    // Check if it's a mouse button
    if (key == KeyCode::LeftMouse || key == KeyCode::RightMouse) {
        return glfwGetMouseButton(window, it->second) == GLFW_PRESS;  // ← Use glfwGetMouseButton!
    }

    // Regular keyboard key
    return glfwGetKey(window, it->second) == GLFW_PRESS;
}

bool InputManager::IsKeyDown(KeyCode key) const {
    return IsKeyPressed(key);
}

bool InputManager::IsKeyReleased(KeyCode key) const {
    auto it = keyMap.find(key);
    if (it == keyMap.end()) return false;

    return glfwGetKey(window, it->second) == GLFW_RELEASE;
}

void InputManager::GetMousePosition(double& x, double& y) const {
    glfwGetCursorPos(window, &x, &y);
}

void InputManager::GetMouseDelta(double& deltaX, double& deltaY) const {
    deltaX = mouseDeltaX;
    deltaY = mouseDeltaY;
}

void InputManager::SetMouseCaptured(bool captured) {
    mouseCaptured = captured;

    if (captured) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}