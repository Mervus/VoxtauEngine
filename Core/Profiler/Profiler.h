//
// Created by Claude on 03/02/2026.
//

#ifndef VOXTAU_PROFILER_H
#define VOXTAU_PROFILER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <mutex>
#include <EngineApi.h>

// Profiling macros for easy use
#define PROFILE_SCOPE(name) ProfileScope _profileScope##__LINE__(name)
#define PROFILE_FUNCTION() ProfileScope _profileScopeFunc(__FUNCTION__)

struct ProfileEntry {
    std::string name;
    double lastTimeMs = 0.0;
    double minTimeMs = 999999.0;
    double maxTimeMs = 0.0;
    double avgTimeMs = 0.0;
    double totalTimeMs = 0.0;
    int callCount = 0;
    int frameCallCount = 0;  // Calls this frame

    // History for graphing
    static const int HISTORY_SIZE = 120;
    float timeHistory[HISTORY_SIZE] = {0};
    int historyIndex = 0;
};

class ENGINE_API Profiler {
public:
    static Profiler& Instance();

    // Called at the start of each frame to reset per-frame counters
    void BeginFrame();

    // Record a timing
    void RecordTime(const std::string& name, double timeMs);

    // Get all profile entries (for UI display)
    const std::unordered_map<std::string, ProfileEntry>& GetEntries() const { return _entries; }

    // Clear all recorded data
    void Clear();

    // Enable/disable profiling
    void SetEnabled(bool enabled) { _enabled = enabled; }
    bool IsEnabled() const { return _enabled; }

    // Get total frame time
    double GetFrameTimeMs() const { return _frameTimeMs; }

private:
    Profiler() = default;

    std::unordered_map<std::string, ProfileEntry> _entries;
    std::mutex _mutex;
    bool _enabled = true;
    double _frameTimeMs = 0.0;
    std::chrono::high_resolution_clock::time_point _frameStart;
};

// RAII scope timer
class ENGINE_API ProfileScope {
public:
    explicit ProfileScope(const char* name);
    ~ProfileScope();

private:
    const char* _name;
    std::chrono::high_resolution_clock::time_point _start;
};

#endif //VOXTAU_PROFILER_H
