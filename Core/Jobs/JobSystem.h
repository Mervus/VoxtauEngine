//
// Created by Marvin on 01/04/2026.
//

#ifndef VOXTAU_JOB_SYSTEM_H
#define VOXTAU_JOB_SYSTEM_H

#include <functional>
#include <memory>
#include <vector>
#include <cstdint>

#include "Core/Log/Logger.h"

namespace enki { class TaskScheduler; class ITaskSet; }

/// Opaque handle to a scheduled job.
/// Caller must call JobSystem::Complete() before dropping the last copy.
class JobHandle {
public:
    JobHandle() = default;
    [[nodiscard]] bool IsValid() const { return _task != nullptr; }

private:
    friend class JobSystem;
    std::shared_ptr<enki::ITaskSet> _task;
};

/// Thread pool & task scheduler (enkiTS wrapper).
/// Singleton — access via JobSystem::Instance().
class JobSystem {
public:
    static JobSystem& Instance();

    /// Start the thread pool.  numThreads = 0  →  hardware_concurrency − 1 workers.
    void Initialize(uint32_t numThreads = 0);
    void Shutdown();
    [[nodiscard]] bool IsInitialized() const;
    [[nodiscard]] uint32_t GetWorkerThreadCount() const;

    /// Schedule a single task on a worker thread.
    /// If dependency is valid, this task won't start until that dependency completes.
    JobHandle Schedule(std::function<void()> task,
                       const JobHandle& dependency = {});

    /// Schedule data-parallel iteration over [0, count) with configurable batch size.
    /// The callback receives [start, end) per batch.
    JobHandle ScheduleParallelFor(uint32_t count, uint32_t batchSize,
                                  std::function<void(uint32_t start, uint32_t end)> task,
                                  const JobHandle& dependency = {});

    /// Block the calling thread until the job completes.
    void Complete(const JobHandle& handle);

    /// Non-blocking completion check.
    [[nodiscard]] bool IsComplete(const JobHandle& handle) const;

private:
    JobSystem() = default;
    ~JobSystem();
    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;

    void CleanupCompleted();

    Log _logger {Log::ClientLog};
    std::unique_ptr<enki::TaskScheduler> _scheduler;
    std::vector<std::shared_ptr<enki::ITaskSet>> _activeTasks;
};

#endif // VOXTAU_JOB_SYSTEM_H