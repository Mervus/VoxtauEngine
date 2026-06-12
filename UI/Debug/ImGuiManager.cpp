//
// Created by Marvin on 29/01/2026.
//

#include "ImGuiManager.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_dx11.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

ImGuiManager::ImGuiManager()
    : initialized(false)
{
}

ImGuiManager::~ImGuiManager() {
    Shutdown();
}

bool ImGuiManager::Initialize(void* windowHandle, ID3D11Device* device, ID3D11DeviceContext* context) {
    if (initialized) return true;

    _logger.Info("[ImGuiManager] Initializing...");

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // When viewports are enabled, tweak WindowRounding/WindowBg
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Get GLFW window from handle
    GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(windowHandle);

    // Setup Platform/Renderer backends
    if (!ImGui_ImplGlfw_InitForOther(glfwWindow, true)) {
        _logger.Error("[ImGuiManager] Failed to initialize ImGui GLFW backend!");
        return false;
    }

    if (!ImGui_ImplDX11_Init(device, context)) {
        _logger.Error("[ImGuiManager] Failed to initialize ImGui DX11 backend!");
        ImGui_ImplGlfw_Shutdown();
        return false;
    }

    initialized = true;
    return true;
}

void ImGuiManager::Shutdown() {
    if (!initialized) return;

    _logger.Info("[ImGuiManager] Shutting down...");

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    initialized = false;
}

void ImGuiManager::BeginFrame() {
    if (!initialized) return;

    // Start the Dear ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::EndFrame() {
    if (!initialized) return;

    // Rendering
    ImGui::Render();
}

void ImGuiManager::Render() {
    if (!initialized) return;

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    ImGuiIO& io = ImGui::GetIO();

}