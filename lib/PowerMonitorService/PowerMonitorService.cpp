/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: PowerMonitorService.cpp
 */

#include "PowerMonitorService.h"
#include "LoggerService.h"

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

PowerMonitorService& PowerMonitorService::instance() {
    static PowerMonitorService inst;
    return inst;
}

// ---------------------------------------------------------------------------
// begin()
// ---------------------------------------------------------------------------

void PowerMonitorService::begin(uint8_t pin, bool activeLow) {
    if (begun_) return;

    pin_       = pin;
    activeLow_ = activeLow;
    begun_     = true;

    // GPIO34-39 on ESP32 are input-only — do not set INPUT_PULLUP for those
    const bool isInputOnly = (pin >= 34 && pin <= 39);
    if (isInputOnly) {
        pinMode(pin_, INPUT);
    } else {
        pinMode(pin_, INPUT_PULLUP);
    }

    // Capture initial state — no callbacks fired on first read
    powerPresent_ = readPowerPresent();
    pendingState_ = powerPresent_;
    debounceStart_ = millis();

    LOGI("PowerMonitorService: pin=%u activeLow=%s initial=%s",
         pin_,
         activeLow_ ? "true" : "false",
         powerPresent_ ? "PRESENT" : "ABSENT");
}

// ---------------------------------------------------------------------------
// update() — debounced state machine, called every ~50 ms
// ---------------------------------------------------------------------------

void PowerMonitorService::update() {
    if (!begun_) return;

    const bool raw = readPowerPresent();

    if (raw != pendingState_) {
        // Signal changed — restart debounce timer
        pendingState_  = raw;
        debounceStart_ = millis();
        return;
    }

    // Signal has been stable — check if debounce window elapsed
    if ((millis() - debounceStart_) < kDebounceMs) return;

    // State is stable and different from confirmed state → fire callback
    if (pendingState_ != powerPresent_) {
        powerPresent_ = pendingState_;

        if (powerPresent_) {
            LOGI("PowerMonitorService: AC power RESTORED");
            if (onRestoredCb_) onRestoredCb_();
        } else {
            LOGI("PowerMonitorService: AC power LOST");
            if (onLostCb_) onLostCb_();
        }
    }
}

// ---------------------------------------------------------------------------
// readPowerPresent() — convert raw GPIO to logical "power present"
// ---------------------------------------------------------------------------

bool PowerMonitorService::readPowerPresent() const {
    const int raw = digitalRead(pin_);
    // activeLow=true:  LOW (0) → present,  HIGH (1) → absent
    // activeLow=false: HIGH(1) → present,  LOW  (0) → absent
    return activeLow_ ? (raw == LOW) : (raw == HIGH);
}
