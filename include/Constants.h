/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: Constants.h
 * Brief: All compile-time system-wide constants in one place.
 */

#pragma once

#include <cstdint>

namespace sentinel {
namespace constants {

// ---------------------------------------------------------------------------
// Serial
// ---------------------------------------------------------------------------
constexpr uint32_t kSerialBaudRate = 115200;

// ---------------------------------------------------------------------------
// WiFi
// ---------------------------------------------------------------------------
constexpr uint32_t kWifiConnectTimeoutMs  = 20000;  // 20 s initial connect
constexpr uint32_t kWifiReconnectDelayMs  = 5000;   // 5 s between retries
constexpr uint8_t  kWifiMaxRetries        = 0;      // 0 = infinite

// ---------------------------------------------------------------------------
// NTP
// ---------------------------------------------------------------------------
constexpr const char* kNtpServer1  = "time.google.com";
constexpr const char* kNtpServer2  = "pool.ntp.org";
constexpr const char* kNtpServer3  = "asia.pool.ntp.org";
constexpr const char* kTimezone    = "IST-5:30";      // Asia/Kolkata UTC+05:30
constexpr uint32_t   kNtpSyncTimeoutMs   = 10000;   // 10 s wait for first sync
constexpr uint32_t   kNtpRetryIntervalMs = 30000;   // 30 s retry if sync fails

// ---------------------------------------------------------------------------
// Telegram
// ---------------------------------------------------------------------------
constexpr const char* kTelegramApiHost  = "api.telegram.org";
constexpr uint16_t    kTelegramApiPort  = 443;
constexpr uint32_t    kTelegramTimeoutMs = 4000;    // HTTP timeout per request (reduced from 8 s to prevent scheduler stall)
constexpr uint32_t    kTelegramPollMs    = 1000;    // getUpdates interval
constexpr uint8_t     kTelegramMaxRetries = 3;

// ---------------------------------------------------------------------------
// Notification queue
// ---------------------------------------------------------------------------
constexpr uint8_t  kQueueMaxMessages  = 1;
constexpr uint16_t kQueueMaxMsgBytes  = 600;

// ---------------------------------------------------------------------------
// Power Monitor
// ---------------------------------------------------------------------------
constexpr uint8_t  kPowerMonitorPin      = 34;    // GPIO34 – AC detect (INPUT only)
constexpr uint32_t kPowerDebounceMs      = 200;   // debounce window

// ---------------------------------------------------------------------------
// Scheduler
// ---------------------------------------------------------------------------
constexpr uint8_t  kSchedulerMaxTasks    = 16;

// ---------------------------------------------------------------------------
// NVS keys
// ---------------------------------------------------------------------------
namespace nvs {
    constexpr const char* kNamespace      = "sentinel";
    constexpr const char* kWifiSsid       = "wifi_ssid";
    constexpr const char* kWifiPass       = "wifi_pass";
    constexpr const char* kBotToken       = "tg_token";
    constexpr const char* kChatId         = "tg_chat_id";
    constexpr const char* kDeviceId       = "device_id";
    constexpr const char* kRestartCount   = "rst_count";
}  // namespace nvs

}  // namespace constants
}  // namespace sentinel
