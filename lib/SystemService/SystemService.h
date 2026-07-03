/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: SystemService.h
 * Brief: Chip metadata, reset reason, uptime, heap, and restart counter.
 */

#pragma once

#include <Arduino.h>
#include <esp_system.h>

/**
 * @class SystemService
 * @brief Singleton providing read-only access to ESP32 system information.
 *
 * Captures reset reason at begin() before it is overwritten. Increments
 * the persistent restart counter in NVS each boot.
 */
class SystemService {
public:
    static SystemService& instance();

    SystemService(const SystemService&)            = delete;
    SystemService& operator=(const SystemService&) = delete;

    /**
     * @brief Capture reset reason and increment restart counter. Call once at boot.
     */
    void begin();

    // -----------------------------------------------------------------------
    // Uptime
    // -----------------------------------------------------------------------
    /** @brief Milliseconds since boot. */
    uint32_t uptimeMs() const { return millis(); }

    /** @brief Human-readable uptime string, e.g. "5h 17m 43s". */
    String uptimeString() const;

    // -----------------------------------------------------------------------
    // Chip info
    // -----------------------------------------------------------------------
    String chipModel()     const;
    uint32_t cpuFreqMhz()  const;
    uint32_t flashSizeMB() const;
    uint32_t flashSpeedMhz() const;
    String   flashMode()     const;
    String macAddress()    const;
    String chipRevision()  const;
    String sdkVersion()    const;

    // -----------------------------------------------------------------------
    // Memory
    // -----------------------------------------------------------------------
    uint32_t freeHeapBytes()      const { return ESP.getFreeHeap(); }
    uint32_t minFreeHeapBytes()   const { return ESP.getMinFreeHeap(); }
    uint32_t largestFreeBlock()   const { return ESP.getMaxAllocHeap(); }
    bool     hasPsram()           const { return ESP.getPsramSize() > 0; }
    uint32_t psramSizeBytes()     const { return ESP.getPsramSize(); }

    // -----------------------------------------------------------------------
    // Reset info
    // -----------------------------------------------------------------------
    /** @brief Human-readable reset reason, e.g. "Power On". */
    String resetReasonString() const { return resetReasonStr_; }

    /** @brief Number of times the device has booted (stored in NVS). */
    uint32_t restartCount() const { return restartCount_; }

private:
    SystemService() = default;
    ~SystemService() = default;

    static String describeResetReason(esp_reset_reason_t reason);

    String   resetReasonStr_;
    uint32_t restartCount_ = 0;
};
