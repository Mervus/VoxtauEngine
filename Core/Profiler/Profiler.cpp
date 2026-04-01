//
// Created by Claude on 03/02/2026.
//

#include "Profiler.h"

#include <nlohmann/json.hpp>
#include <fstream>

struct ScopeStackEntry {
    const char* name;
    std::chrono::high_resolution_clock::time_point start;
    int depth;
};

static thread_local std::vector<ScopeStackEntry> t_scopeStack;
static thread_local int t_currentDepth = 0;

static uint32_t GetThreadIdU32() {
    auto id = std::this_thread::get_id();
    return static_cast<uint32_t>(std::hash<std::thread::id>{}(id));
}

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
    _flameFrameStart = now;
    _frameNumber++;

    // Commit the staging flame frame into the ring buffer
    if (!_flameFrozen) {
        std::lock_guard<std::mutex> lock(_flameMutex);
        _flameStaging.frameDurationMs = _frameTimeMs;
        _flameStaging.frameNumber = _frameNumber - 1; // This data is from the previous frame

        _flameFrames[_flameWriteIndex] = std::move(_flameStaging);
        _flameWriteIndex = (_flameWriteIndex + 1) % MAX_FLAME_FRAMES;
        if (_flameFrameCount < MAX_FLAME_FRAMES) _flameFrameCount++;

        // Reset staging for the new frame
        _flameStaging = FlameFrame{};
        _flameStaging.entries.reserve(128);
    }

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

    std::lock_guard<std::mutex> flock(_flameMutex);
    for (int i = 0; i < MAX_FLAME_FRAMES; i++) {
        _flameFrames[i] = FlameFrame{};
    }
    _flameFrameCount = 0;
    _flameWriteIndex = 0;
    _flameStaging = FlameFrame{};
    _oneShotTimings.clear();
}

void Profiler::PushScope(const char* name)
{
    if (!_enabled || _flameFrozen) return;

    ScopeStackEntry entry;
    entry.name = name;
    entry.start = std::chrono::high_resolution_clock::now();
    entry.depth = t_currentDepth;

    t_scopeStack.push_back(entry);
    t_currentDepth++;
}

void Profiler::PopScope()
{
    if (!_enabled || _flameFrozen) return;
    if (t_scopeStack.empty()) return;

    auto end = std::chrono::high_resolution_clock::now();
    ScopeStackEntry& top = t_scopeStack.back();

    auto startAbsolute = std::chrono::duration<double, std::milli>(top.start - _origin);
    auto duration = std::chrono::duration<double, std::milli>(end - top.start);

    FlameEntry fe;
    fe.name = top.name;
    fe.startMs = startAbsolute.count();
    fe.durationMs = duration.count();
    fe.depth = top.depth;
    fe.threadId = GetThreadIdU32();

    {
        std::lock_guard<std::mutex> lock(_flameMutex);
        _flameStaging.entries.push_back(fe);
    }

    t_scopeStack.pop_back();
    t_currentDepth--;
    if (t_currentDepth < 0) t_currentDepth = 0;
}

const FlameFrame& Profiler::GetFlameFrame(int index) const
{
    // Index 0 = most recent, wrapping back through ring buffer
    int actual = (_flameWriteIndex - 1 - index + MAX_FLAME_FRAMES) % MAX_FLAME_FRAMES;
    return _flameFrames[actual];
}

void Profiler::RecordOneShot(const std::string& name, double durationMs)
{
    _oneShotTimings.emplace_back(name, durationMs);
}

void Profiler::ExportJson()
{
    using nlohmann::json;

    json traceEvents = json::array();
    // Export all flame frames from the ring buffer (oldest to newest)
    // Entries use absolute timestamps relative to profiler origin
    int count = _flameFrameCount;

    for (int i = count - 1; i >= 0; i--) {
        const FlameFrame& frame = GetFlameFrame(i);

        for (const auto& entry : frame.entries) {
            traceEvents.push_back({
                {"name", entry.name},
                {"cat",  "frame"},
                {"ph",   "X"},
                {"ts",   entry.startMs * 1000.0},
                {"dur",  entry.durationMs * 1000.0},
                {"pid",  0},
                {"tid",  entry.threadId}
            });
        }
    }

    json output;
    output["traceEvents"] = traceEvents;

    std::ofstream file("profile_trace.json");
    file << output.dump(2);
}

// ProfileScope implementation
ProfileScope::ProfileScope(const char* name)
    : _name(name)
    , _start(std::chrono::high_resolution_clock::now())
{
    Profiler::Instance().PushScope(name);
}

ProfileScope::~ProfileScope() {
    if (!Profiler::Instance().IsEnabled()) return;

    Profiler::Instance().PopScope();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::milli>(end - _start);
    Profiler::Instance().RecordTime(_name, duration.count());
}
