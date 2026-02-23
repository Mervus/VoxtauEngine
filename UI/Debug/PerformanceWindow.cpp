//
// Created by Marvin on 29/01/2026.
//

#include "PerformanceWindow.h"

#include <imgui.h>
#include <cstring>

PerformanceWindow::PerformanceWindow()
    : isOpen(true)
    , frameTimeHistoryIndex(0)
{
    std::memset(frameTimeHistory, 0, sizeof(frameTimeHistory));
}

void PerformanceWindow::Update(float deltaTime) {
    // Store frame time in history
    frameTimeHistory[frameTimeHistoryIndex] = deltaTime * 1000.0f; // Convert to ms
    frameTimeHistoryIndex = (frameTimeHistoryIndex + 1) % 100;
}

void PerformanceWindow::Draw() {
    if (!isOpen) return;

    ImGui::Begin("Performance", &isOpen);

    // Calculate average frame time
    float avgFrameTime = 0.0f;
    for (int i = 0; i < 100; i++) {
        avgFrameTime += frameTimeHistory[i];
    }
    avgFrameTime /= 100.0f;

    float fps = 1000.0f / avgFrameTime;

    ImGui::Text("FPS: %.1f", fps);
    ImGui::Text("Frame Time: %.2f ms", avgFrameTime);

    ImGui::Separator();

    // Plot frame time history
    ImGui::PlotLines("Frame Time (ms)", frameTimeHistory, 100, frameTimeHistoryIndex, nullptr, 0.0f, 33.33f, ImVec2(0, 80));

    ImGui::End();
}