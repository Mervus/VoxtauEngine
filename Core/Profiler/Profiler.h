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

struct FlameEntry {
    const char* name;          // Pointer to string literal (from PROFILE_SCOPE)
    double startMs;            // Relative to profiler origin (absolute timeline)
    double durationMs;
    int depth;                 // Nesting depth (0 = top-level)
    uint32_t threadId;         // Thread that recorded this entry
};

struct FlameFrame {
    double frameDurationMs = 0.0;
    uint64_t frameNumber = 0;
    std::vector<FlameEntry> entries;
};

class Profiler {
public:
    static Profiler& Instance();

    FlameFrame _buffers[2];
    std::atomic<int> _readIndex{0};   // profiler thread reads this one
    int _writeIndex = 1;              // main thread writes this one (no atomic needed, single writer)

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

    void PushScope(const char* name);
    void PopScope();

    static const int MAX_FLAME_FRAMES = 500000;
    const FlameFrame& GetFlameFrame(int index) const;
    int GetFlameFrameCount() const { return _flameFrameCount; }
    int GetCurrentFlameFrameIndex() const { return _flameWriteIndex; }
    uint64_t GetFrameNumber() const { return _frameNumber; }

    // Freeze/unfreeze flame capture (for inspecting a specific frame)
    void SetFlameFrozen(bool frozen) { _flameFrozen = frozen; }
    bool IsFlameFrozen() const { return _flameFrozen; }

    // One-shot timing for startup / loading phases
    void RecordOneShot(const std::string& name, double durationMs);
    const std::vector<std::pair<std::string, double>>& GetOneShotTimings() const { return _oneShotTimings; }

    void ExportJson();
private:
    Profiler() : _origin(std::chrono::high_resolution_clock::now()) {}

    std::unordered_map<std::string, ProfileEntry> _entries;
    std::mutex _mutex;
    bool _enabled = true;
    double _frameTimeMs = 0.0;
    std::chrono::high_resolution_clock::time_point _frameStart;

    // Fixed time origin for all absolute timestamps
    std::chrono::high_resolution_clock::time_point _origin;
    std::chrono::high_resolution_clock::time_point _flameFrameStart;

    // Ring buffer of flame frames
    FlameFrame _flameFrames[MAX_FLAME_FRAMES];
    int _flameWriteIndex = 0;
    int _flameFrameCount = 0;
    uint64_t _frameNumber = 0;
    bool _flameFrozen = false;

    // Staging buffer: scopes push entries here during the frame,
    // then on BeginFrame we swap it into the ring buffer.
    FlameFrame _flameStaging;
    std::mutex _flameMutex;

    // One-shot timings (startup / loading)
    std::vector<std::pair<std::string, double>> _oneShotTimings;
};

// RAII scope timer
class ProfileScope {
public:
    explicit ProfileScope(const char* name);
    ~ProfileScope();

private:
    const char* _name;
    std::chrono::high_resolution_clock::time_point _start;
};

#endif //VOXTAU_PROFILER_H
