//
// Created by Marvin on 01/04/2026.
//

#include "JobSystem.h"
#include <TaskScheduler.h>
#include <cassert>
#include <iostream>

// Internal task type — wraps std::function for enkiTS's inheritance-based API
class FunctionTask : public enki::ITaskSet {
public:
    FunctionTask(uint32_t setSize, uint32_t minRange,
                 std::function<void(uint32_t, uint32_t)> func)
        : ITaskSet(setSize, minRange)
        , _func(std::move(func)) {}

    void ExecuteRange(enki::TaskSetPartition range, uint32_t /*threadnum*/) override {
        _func(range.start, range.end);
    }

    // Stored inline so its lifetime matches the task's.
    enki::Dependency dependency;

private:
    std::function<void(uint32_t, uint32_t)> _func;
};

// JobSystem
JobSystem& JobSystem::Instance() {
    static JobSystem instance;
    return instance;
}

JobSystem::~JobSystem() {
    Shutdown();
}

void JobSystem::Initialize(uint32_t numThreads) {
    if (_scheduler) return;

    _scheduler = std::make_unique<enki::TaskScheduler>();

    enki::TaskSchedulerConfig config;
    if (numThreads > 0) {
        config.numTaskThreadsToCreate = numThreads;
    }

    _scheduler->Initialize(config);

    std::cout << "[JobSystem] Initialized with "
              << _scheduler->GetNumTaskThreads() << " worker threads"
              << std::endl;
}

void JobSystem::Shutdown() {
    if (!_scheduler) return;

    _scheduler->WaitforAllAndShutdown();
    _activeTasks.clear();
    _scheduler.reset();

    std::cout << "[JobSystem] Shutdown complete" << std::endl;
}

bool JobSystem::IsInitialized() const {
    return _scheduler != nullptr;
}

uint32_t JobSystem::GetWorkerThreadCount() const {
    if (!_scheduler) return 0;
    return _scheduler->GetNumTaskThreads();
}

// Scheduling
JobHandle JobSystem::Schedule(std::function<void()> task,
                              const JobHandle& dependency) {
    assert(_scheduler && "JobSystem not initialized");

    auto internal = std::make_shared<FunctionTask>(
        1, 1,
        [fn = std::move(task)](uint32_t, uint32_t) { fn(); });

    if (dependency.IsValid()) {
        internal->SetDependency(internal->dependency, dependency._task.get());
    } else {
        _scheduler->AddTaskSetToPipe(internal.get());
    }

    CleanupCompleted();
    _activeTasks.push_back(internal);

    JobHandle handle;
    handle._task = internal;
    return handle;
}

JobHandle JobSystem::ScheduleParallelFor(
    uint32_t count, uint32_t batchSize,
    std::function<void(uint32_t start, uint32_t end)> task,
    const JobHandle& dependency) {
    assert(_scheduler && "JobSystem not initialized");

    auto internal = std::make_shared<FunctionTask>(
        count, batchSize, std::move(task));

    if (dependency.IsValid()) {
        internal->SetDependency(internal->dependency, dependency._task.get());
    } else {
        _scheduler->AddTaskSetToPipe(internal.get());
    }

    CleanupCompleted();
    _activeTasks.push_back(internal);

    JobHandle handle;
    handle._task = internal;
    return handle;
}

// Completion
void JobSystem::Complete(const JobHandle& handle) {
    assert(_scheduler && "JobSystem not initialized");
    if (!handle.IsValid()) return;
    _scheduler->WaitforTask(handle._task.get());
}

bool JobSystem::IsComplete(const JobHandle& handle) const {
    if (!handle.IsValid()) return true;
    return handle._task->GetIsComplete();
}

// Internal
void JobSystem::CleanupCompleted() {
    std::erase_if(_activeTasks, [](const auto& task) {
        return task->GetIsComplete();
    });
}