/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: PowerMonitorService.h
 * Brief: Debounced AC power presence detector via GPIO.
 *
 * Hardware:
 *   An AC optocoupler (e.g. PC817) drives GPIO34 LOW when AC mains is present.
 *   When AC is absent the pin floats high (internal/external pull-up).
 *   Therefore activeLow = true  →  LOW signal means "power present".
 *
 * Usage:
 *   PowerMonitorService::instance().begin(kPowerMonitorPin, true);
 *   PowerMonitorService::instance().onPowerLost([]() { ... });
 *   PowerMonitorService::instance().onPowerRestored([]() { ... });
 *   // call update() every ~50 ms via scheduler
 */

#pragma once

#include <Arduino.h>
#include <functional>

/**
 * @class PowerMonitorService
 * @brief Singleton, debounced GPIO power-presence detector.
 *
 * Fires onPowerLost / onPowerRestored callbacks exactly once per
 * transition, after the signal has been stable for kDebounceMs.
 */
class PowerMonitorService {
public:
    static PowerMonitorService& instance();

    PowerMonitorService(const PowerMonitorService&)            = delete;
    PowerMonitorService& operator=(const PowerMonitorService&) = delete;

    /**
     * @brief Configure the GPIO pin and read the initial state.
     * @param pin       GPIO number to monitor (input-only safe: e.g. GPIO34).
     * @param activeLow true  → LOW  signal means AC present (typical optocoupler).
     *                  false → HIGH signal means AC present.
     */
    void begin(uint8_t pin, bool activeLow = true);

    /**
     * @brief Poll the GPIO and fire callbacks on debounced transitions.
     *        Call from the scheduler every ~50 ms.
     */
    void update();

    /** @brief Register a callback invoked once when AC power is lost. */
    void onPowerLost(std::function<void()> cb)     { onLostCb_     = cb; }

    /** @brief Register a callback invoked once when AC power is restored. */
    void onPowerRestored(std::function<void()> cb) { onRestoredCb_ = cb; }

    /** @brief Returns true if AC power is currently detected as present. */
    bool isPowerPresent() const { return powerPresent_; }

private:
    PowerMonitorService()  = default;
    ~PowerMonitorService() = default;

    /** Read the raw GPIO and convert to a logical "power present" bool. */
    bool readPowerPresent() const;

    uint8_t  pin_          = 0;
    bool     activeLow_    = true;
    bool     begun_        = false;

    bool     powerPresent_ = true;   // assumed present at boot until proven otherwise
    bool     pendingState_ = true;   // candidate state being debounced
    uint32_t debounceStart_ = 0;     // millis() when the candidate state began

    std::function<void()> onLostCb_;
    std::function<void()> onRestoredCb_;

    // Debounce window — must match kPowerDebounceMs in Constants.h
    static constexpr uint32_t kDebounceMs = 200;
};
