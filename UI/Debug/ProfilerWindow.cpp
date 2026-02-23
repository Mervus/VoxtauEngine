//
// Created by Claude on 03/02/2026.
//

#include "ProfilerWindow.h"
#include "Core/Profiler/Profiler.h"

#include <imgui.h>
#include <algorithm>
#include <vector>

ProfilerWindow::ProfilerWindow()
    : _isOpen(true)
    , _sortByTime(true)
    , _showGraph(true)
    , _graphHeight(60.0f)
{
}

void ProfilerWindow::Draw() {
    if (!_isOpen) return;

    ImGui::Begin("Profiler", &_isOpen, ImGuiWindowFlags_None);

    Profiler& profiler = Profiler::Instance();

    // Controls
    bool enabled = profiler.IsEnabled();
    if (ImGui::Checkbox("Enabled", &enabled)) {
        profiler.SetEnabled(enabled);
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        profiler.Clear();
    }

    ImGui::SameLine();
    ImGui::Checkbox("Sort by Time", &_sortByTime);

    ImGui::SameLine();
    ImGui::Checkbox("Show Graphs", &_showGraph);

    ImGui::Separator();

    // Frame time overview
    double frameTimeMs = profiler.GetFrameTimeMs();
    float fps = frameTimeMs > 0.0 ? static_cast<float>(1000.0 / frameTimeMs) : 0.0f;
    ImGui::Text("Frame: %.2f ms (%.1f FPS)", frameTimeMs, fps);

    ImGui::Separator();

    // Get and sort entries
    const auto& entries = profiler.GetEntries();
    std::vector<const ProfileEntry*> sortedEntries;
    sortedEntries.reserve(entries.size());

    for (const auto& pair : entries) {
        sortedEntries.push_back(&pair.second);
    }

    if (_sortByTime) {
        std::sort(sortedEntries.begin(), sortedEntries.end(),
            [](const ProfileEntry* a, const ProfileEntry* b) {
                return a->lastTimeMs > b->lastTimeMs;
            });
    } else {
        std::sort(sortedEntries.begin(), sortedEntries.end(),
            [](const ProfileEntry* a, const ProfileEntry* b) {
                return a->name < b->name;
            });
    }

    // Table header
    ImGui::BeginChild("ProfilerTable", ImVec2(0, 0), true);

    if (ImGui::BeginTable("ProfileEntries", _showGraph ? 6 : 5,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY)) {

        ImGui::TableSetupColumn("Function", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Last (ms)", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Avg (ms)", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Min/Max", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Calls", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        if (_showGraph) {
            ImGui::TableSetupColumn("Graph", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        }
        ImGui::TableHeadersRow();

        for (const ProfileEntry* entry : sortedEntries) {
            ImGui::TableNextRow();

            // Color code based on time (green < 1ms, yellow < 5ms, red >= 5ms)
            ImVec4 color;
            if (entry->lastTimeMs < 1.0) {
                color = ImVec4(0.3f, 0.8f, 0.3f, 1.0f);  // Green
            } else if (entry->lastTimeMs < 5.0) {
                color = ImVec4(0.9f, 0.9f, 0.3f, 1.0f);  // Yellow
            } else {
                color = ImVec4(0.9f, 0.3f, 0.3f, 1.0f);  // Red
            }

            // Function name
            ImGui::TableNextColumn();
            ImGui::TextColored(color, "%s", entry->name.c_str());

            // Last time
            ImGui::TableNextColumn();
            ImGui::TextColored(color, "%.3f", entry->lastTimeMs);

            // Average time
            ImGui::TableNextColumn();
            ImGui::Text("%.3f", entry->avgTimeMs);

            // Min/Max
            ImGui::TableNextColumn();
            ImGui::Text("%.2f/%.2f", entry->minTimeMs, entry->maxTimeMs);

            // Call count (total / this frame)
            ImGui::TableNextColumn();
            ImGui::Text("%d", entry->frameCallCount);

            // Time history graph
            if (_showGraph) {
                ImGui::TableNextColumn();

                // Find max value for scaling
                float maxVal = 0.1f;
                for (int i = 0; i < ProfileEntry::HISTORY_SIZE; i++) {
                    if (entry->timeHistory[i] > maxVal) {
                        maxVal = entry->timeHistory[i];
                    }
                }

                // Use a unique ID for each plot
                ImGui::PushID(entry->name.c_str());
                ImGui::PlotLines("##graph", entry->timeHistory, ProfileEntry::HISTORY_SIZE,
                    entry->historyIndex, nullptr, 0.0f, maxVal, ImVec2(140, 30));
                ImGui::PopID();
            }
        }

        ImGui::EndTable();
    }

    ImGui::EndChild();

    ImGui::End();
}
