//
// Created by Claude on 03/02/2026.
//

#include "Profiler.h"

Profiler& Profiler::Instance() {
    static Profiler instance;
    return instance;
}

void Profiler::BeginFrame() {
    if (!_enabled) return;

    auto now = std::chrono::high_resolution_clock::now();

    // Calculate frame time from last BeginFrame
    if (_frameStart.time_since_epoch().count() > 0) {
        auto duration = std::chrono::duration<double, std::milli>(now - _frameStart);
        _frameTimeMs = duration.count();
    }

    _frameStart = now;

    // Reset per-frame call counts
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto& pair : _entries) {
        pair.second.frameCallCount = 0;
    }
}

void Profiler::RecordTime(const std::string& name, double timeMs) {
    if (!_enabled) return;

    std::lock_guard<std::mutex> lock(_mutex);

    ProfileEntry& entry = _entries[name];
    if (entry.name.empty()) {
        entry.name = name;
    }

    entry.lastTimeMs = timeMs;
    entry.totalTimeMs += timeMs;
    entry.callCount++;
    entry.frameCallCount++;

    // Update min/max
    if (timeMs < entry.minTimeMs) entry.minTimeMs = timeMs;
    if (timeMs > entry.maxTimeMs) entry.maxTimeMs = timeMs;

    // Update average
    entry.avgTimeMs = entry.totalTimeMs / entry.callCount;

    // Update history (store the last time for this frame)
    entry.timeHistory[entry.historyIndex] = static_cast<float>(timeMs);
    entry.historyIndex = (entry.historyIndex + 1) % ProfileEntry::HISTORY_SIZE;
}

void Profiler::Clear() {
    std::lock_guard<std::mutex> lock(_mutex);
    _entries.clear();
}

// ProfileScope implementation
ProfileScope::ProfileScope(const char* name)
    : _name(name)
    , _start(std::chrono::high_resolution_clock::now())
{
}

ProfileScope::~ProfileScope() {
    if (!Profiler::Instance().IsEnabled()) return;

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::milli>(end - _start);
    Profiler::Instance().RecordTime(_name, duration.count());
}
