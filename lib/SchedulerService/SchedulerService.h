/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: SchedulerService.h
 * Brief: Non-blocking millis()-based cooperative task scheduler.
 *        No RTOS. No delay(). Pure cooperative scheduling.
 */

#pragma once

#include <Arduino.h>
#include <functional>
#include "Constants.h"

using TaskFn = std::function<void()>;

struct ScheduledTask {
    const char* name        = nullptr;
    uint32_t    intervalMs  = 0;
    uint32_t    lastRunMs   = 0;
    bool        runOnce     = false;   ///< If true, task is removed after first run
    bool        active      = false;
    TaskFn      fn;
};

/**
 * @class SchedulerService
 * @brief Singleton cooperative task scheduler.
 *
 * Services register recurring tasks in begin(). The main loop calls
 * update() every iteration. No blocking, no RTOS tasks required.
 *
 * Usage:
 *   SchedulerService::instance().addTask("my-task", 5000, []{ doSomething(); });
 */
class SchedulerService {
public:
    static SchedulerService& instance();

    SchedulerService(const SchedulerService&)            = delete;
    SchedulerService& operator=(const SchedulerService&) = delete;

    /** @brief Must be called once in setup() before addTask(). */
    void begin();

    /**
     * @brief Register a recurring task.
     * @param name        Debug label.
     * @param intervalMs  How often to run (ms). 0 = every loop tick.
     * @param fn          The callback to invoke.
     * @return true if registration succeeded (pool not full).
     */
    bool addTask(const char* name, uint32_t intervalMs, TaskFn fn);

    /**
     * @brief Register a one-shot delayed task (fires once, then removed).
     */
    bool addDelayedTask(const char* name, uint32_t delayMs, TaskFn fn);

    /**
     * @brief Run all due tasks. Call every loop() iteration.
     */
    void update();

    /** @brief Remove a task by name. */
    void removeTask(const char* name);

private:
    SchedulerService() = default;
    ~SchedulerService() = default;

    ScheduledTask tasks_[sentinel::constants::kSchedulerMaxTasks];
    bool          begun_ = false;
};
