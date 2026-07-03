/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: TimeService.cpp
 */

#include "TimeService.h"
#include "LoggerService.h"
#include "Constants.h"

using namespace sentinel::constants;

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

TimeService& TimeService::instance() {
    static TimeService inst;
    return inst;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void TimeService::begin() {
    if (syncStarted_) return;
    attemptSync();
}

void TimeService::update() {
    if (synced_) return;

    // Check if the first sync attempt succeeded
    if (syncStarted_ && !synced_) {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo, 0)) {
            synced_ = true;
            LOGI("TimeService: NTP synced – %s %s", currentDate().c_str(), currentTime().c_str());
            return;
        }

        // Timed out – schedule a retry
        if ((millis() - syncStartMs_) > kNtpSyncTimeoutMs) {
            if ((millis() - lastRetryMs_) >= kNtpRetryIntervalMs) {
                LOGW("TimeService: NTP sync not yet complete, retrying...");
                lastRetryMs_ = millis();
                attemptSync();
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

void TimeService::attemptSync() {
    LOGI("TimeService: starting NTP sync (tz=%s)", kTimezone);
    configTzTime(kTimezone, kNtpServer1, kNtpServer2, kNtpServer3);
    syncStarted_ = true;
    syncStartMs_ = millis();
    lastRetryMs_ = millis();
}

// ---------------------------------------------------------------------------
// Formatted output
// ---------------------------------------------------------------------------

String TimeService::formatTime(const char* fmt) const {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 0)) {
        return "--";
    }
    char buf[32];
    strftime(buf, sizeof(buf), fmt, &timeinfo);
    return String(buf);
}

String TimeService::currentDate() const {
    return formatTime("%d/%m/%Y");
}

String TimeService::currentTime() const {
    return formatTime("%H:%M:%S");
}

time_t TimeService::unixTimestamp() const {
    return time(nullptr);
}
