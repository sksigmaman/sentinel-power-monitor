/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: SchedulerService.cpp
 */

#include "SchedulerService.h"
#include "LoggerService.h"

#include <cstring>

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

SchedulerService& SchedulerService::instance() {
    static SchedulerService inst;
    return inst;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void SchedulerService::begin() {
    if (begun_) return;
    for (auto& t : tasks_) t = ScheduledTask{};
    begun_ = true;
    LOGI("SchedulerService: ready (capacity=%u)", sentinel::constants::kSchedulerMaxTasks);
}

// ---------------------------------------------------------------------------
// Task management
// ---------------------------------------------------------------------------

bool SchedulerService::addTask(const char* name, uint32_t intervalMs, TaskFn fn) {
    for (auto& t : tasks_) {
        if (!t.active) {
            t.name       = name;
            t.intervalMs = intervalMs;
            t.lastRunMs  = millis();  // avoid firing immediately on first tick
            t.runOnce    = false;
            t.fn         = fn;
            t.active     = true;
            LOGD("Scheduler: added task '%s' interval=%u ms", name, intervalMs);
            return true;
        }
    }
    LOGE("SchedulerService: task pool full! Cannot add '%s'", name);
    return false;
}

bool SchedulerService::addDelayedTask(const char* name, uint32_t delayMs, TaskFn fn) {
    for (auto& t : tasks_) {
        if (!t.active) {
            t.name       = name;
            t.intervalMs = delayMs;
            t.lastRunMs  = millis();
            t.runOnce    = true;
            t.fn         = fn;
            t.active     = true;
            LOGD("Scheduler: added one-shot '%s' delay=%u ms", name, delayMs);
            return true;
        }
    }
    LOGE("SchedulerService: task pool full! Cannot add delayed '%s'", name);
    return false;
}

void SchedulerService::removeTask(const char* name) {
    for (auto& t : tasks_) {
        if (t.active && t.name != nullptr && strcmp(t.name, name) == 0) {
            t = ScheduledTask{};
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void SchedulerService::update() {
    if (!begun_) return;

    const uint32_t now = millis();
    for (auto& t : tasks_) {
        if (!t.active) continue;
        if ((now - t.lastRunMs) >= t.intervalMs) {
            t.lastRunMs = now;
            if (t.fn) t.fn();
            if (t.runOnce) {
                t = ScheduledTask{};  // remove after one execution
            }
        }
    }
}
