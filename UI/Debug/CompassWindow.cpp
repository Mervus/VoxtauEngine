//
// Created by Marvin on 05/02/2026.
//

#include "CompassWindow.h"
#include <imgui.h>
#include <cmath>

CompassWindow::CompassWindow()
    : isOpen(true)
    , forward(0, 0, 1)
{
}

void CompassWindow::Update(const Math::Vector3& cameraForward) {
    forward = cameraForward;
}

void CompassWindow::Draw() {
    if (!isOpen) return;

    ImGui::SetNextWindowSize(ImVec2(150, 150), ImGuiCond_FirstUseEver);
    ImGui::Begin("Compass", &isOpen, ImGuiWindowFlags_NoResize);

    // Calculate angle from forward vector (XZ plane)
    // atan2 gives angle from +X axis, we want angle from +Z (north)
    float angle = atan2f(forward.x, forward.z);  // Radians, 0 = +Z (North)

    // Get draw list for custom drawing
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();

    // Compass center and radius
    float centerX = windowPos.x + windowSize.x * 0.5f;
    float centerY = windowPos.y + windowSize.y * 0.5f + 10.0f;
    float radius = 45.0f;

    // Draw compass circle
    drawList->AddCircle(ImVec2(centerX, centerY), radius, IM_COL32(150, 150, 150, 255), 32, 2.0f);

    // Draw cardinal directions (rotated by camera angle)
    const char* directions[] = {"N", "E", "S", "W"};
    float dirAngles[] = {0.0f, 1.5708f, 3.14159f, 4.71239f};  // 0, 90, 180, 270 degrees

    for (int i = 0; i < 4; i++) {
        float dirAngle = dirAngles[i] - angle;  // Rotate by camera angle
        float dirX = centerX + sinf(dirAngle) * (radius + 12.0f);
        float dirY = centerY - cosf(dirAngle) * (radius + 12.0f);

        // Color North red, others white
        ImU32 color = (i == 0) ? IM_COL32(255, 100, 100, 255) : IM_COL32(255, 255, 255, 255);

        ImVec2 textSize = ImGui::CalcTextSize(directions[i]);
        drawList->AddText(ImVec2(dirX - textSize.x * 0.5f, dirY - textSize.y * 0.5f), color, directions[i]);
    }

    // Draw direction indicator (pointing up = forward)
    float indicatorLen = radius * 0.7f;
    float tipX = centerX;
    float tipY = centerY - indicatorLen;

    // Triangle pointing up
    drawList->AddTriangleFilled(
        ImVec2(tipX, tipY),
        ImVec2(tipX - 8, centerY),
        ImVec2(tipX + 8, centerY),
        IM_COL32(255, 200, 50, 255)
    );

    // Draw small circle at center
    drawList->AddCircleFilled(ImVec2(centerX, centerY), 4.0f, IM_COL32(255, 200, 50, 255));

    // Show angle in degrees
    float degrees = angle * 180.0f / 3.14159f;
    if (degrees < 0) degrees += 360.0f;

    ImGui::SetCursorPosY(windowSize.y - 25.0f);
    ImGui::Text("  %.0f", degrees);

    ImGui::End();
}
