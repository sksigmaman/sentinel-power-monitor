/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: TimeService.h
 * Brief: NTP time synchronization and formatted date/time output.
 *        Timezone: Asia/Kolkata (IST UTC+05:30).
 */

#pragma once

#include <Arduino.h>
#include <time.h>

/**
 * @class TimeService
 * @brief Singleton NTP synchronization and time formatting service.
 *
 * Call begin() once when WiFi is available.
 * Retries automatically every 30 s if sync fails.
 * Never blocks the main loop.
 */
class TimeService {
public:
    static TimeService& instance();

    TimeService(const TimeService&)            = delete;
    TimeService& operator=(const TimeService&) = delete;

    /**
     * @brief Start NTP synchronization. Requires WiFi to be connected.
     *        Non-blocking – sync status is polled in update().
     */
    void begin();

    /**
     * @brief Poll for sync completion and schedule retries. Call every loop().
     */
    void update();

    /** @brief True once NTP has successfully synchronized. */
    bool isSynced() const { return synced_; }

    // -----------------------------------------------------------------------
    // Formatted output
    // -----------------------------------------------------------------------

    /** @brief Current date as DD/MM/YYYY, e.g. "03/07/2026". */
    String currentDate() const;

    /** @brief Current time as HH:MM:SS, e.g. "18:25:42". */
    String currentTime() const;

    /** @brief Unix timestamp (seconds since epoch, UTC). */
    time_t unixTimestamp() const;

private:
    TimeService() = default;
    ~TimeService() = default;

    void attemptSync();
    String formatTime(const char* fmt) const;

    bool     synced_       = false;
    bool     syncStarted_  = false;
    uint32_t syncStartMs_  = 0;
    uint32_t lastRetryMs_  = 0;
};
