/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: SystemService.cpp
 */

#include "SystemService.h"
#include "StorageService.h"
#include "LoggerService.h"
#include "Constants.h"

#include <WiFi.h>
#include <esp_chip_info.h>

using namespace sentinel::constants::nvs;

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

SystemService& SystemService::instance() {
    static SystemService inst;
    return inst;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void SystemService::begin() {
    // Capture reset reason before anything else can change it
    resetReasonStr_ = describeResetReason(esp_reset_reason());

    // Increment NVS restart counter
    StorageService& s = StorageService::instance();
    restartCount_ = s.getUInt(kRestartCount, 0) + 1;
    s.putUInt(kRestartCount, restartCount_);

    LOGI("SystemService: reset reason='%s' restartCount=%u",
         resetReasonStr_.c_str(), restartCount_);
}

// ---------------------------------------------------------------------------
// Uptime
// ---------------------------------------------------------------------------

String SystemService::uptimeString() const {
    uint32_t totalSec = millis() / 1000;
    const uint32_t h  = totalSec / 3600;   totalSec %= 3600;
    const uint32_t m  = totalSec / 60;     totalSec %= 60;
    const uint32_t s  = totalSec;

    char buf[32];
    if (h > 0) {
        snprintf(buf, sizeof(buf), "%uh %um %us", h, m, s);
    } else if (m > 0) {
        snprintf(buf, sizeof(buf), "%um %us", m, s);
    } else {
        snprintf(buf, sizeof(buf), "%us", s);
    }
    return String(buf);
}

// ---------------------------------------------------------------------------
// Chip info
// ---------------------------------------------------------------------------

String SystemService::chipModel() const {
    return String(ESP.getChipModel());
}

uint32_t SystemService::cpuFreqMhz() const {
    return ESP.getCpuFreqMHz();
}

uint32_t SystemService::flashSizeMB() const {
    return ESP.getFlashChipSize() / (1024 * 1024);
}

uint32_t SystemService::flashSpeedMhz() const {
    return ESP.getFlashChipSpeed() / 1000000;
}

String SystemService::flashMode() const {
    switch (ESP.getFlashChipMode()) {
        case FM_QIO:  return "QIO";
        case FM_QOUT: return "QOUT";
        case FM_DIO:  return "DIO";
        case FM_DOUT: return "DOUT";
        default:      return "Unknown";
    }
}

String SystemService::macAddress() const {
    return WiFi.macAddress();
}

String SystemService::chipRevision() const {
    char buf[8];
    snprintf(buf, sizeof(buf), "%u", ESP.getChipRevision());
    return String(buf);
}

String SystemService::sdkVersion() const {
    return String(ESP.getSdkVersion());
}

// ---------------------------------------------------------------------------
// Reset reason description
// ---------------------------------------------------------------------------

String SystemService::describeResetReason(esp_reset_reason_t reason) {
    switch (reason) {
        case ESP_RST_POWERON:   return "Power On";
        case ESP_RST_EXT:       return "External Reset";
        case ESP_RST_SW:        return "Software Reset";
        case ESP_RST_PANIC:     return "Panic / Exception";
        case ESP_RST_INT_WDT:   return "Interrupt Watchdog";
        case ESP_RST_TASK_WDT:  return "Task Watchdog";
        case ESP_RST_WDT:       return "Other Watchdog";
        case ESP_RST_DEEPSLEEP: return "Deep Sleep Wake";
        case ESP_RST_BROWNOUT:  return "Brownout";
        case ESP_RST_SDIO:      return "SDIO Reset";
        default:                return "Unknown";
    }
}
