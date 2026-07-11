/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: Boot.cpp
 * Brief: Top-level orchestrator. Initialises all services in dependency order
 *        and wires together callbacks. Drives the cooperative main loop.
 */

#include "Boot.h"

// Foundation
#include "LoggerService.h"
#include "StorageService.h"
#include "SystemService.h"
#include "SchedulerService.h"

// Network
#include "WiFiService.h"
#include "NetworkService.h"
#include "TimeService.h"

// Provisioning
#include "ProvisioningService.h"

// Telegram
#include "JsonService.h"
#include "TelegramService.h"
#include "KeyboardService.h"

// Application
#include "PowerMonitorService.h"
#include "StatusService.h"
#include "NotificationService.h"

// Shared types
#include "AppCredentials.h"
#include "Constants.h"
#include "Version.h"

// File logging
#include "LogFileService.h"

#include <esp_system.h>

// ---------------------------------------------------------------------------
// Module-level state
// ---------------------------------------------------------------------------

namespace {
    sentinel::AppCredentials g_creds;
    bool g_startupSent     = false;  // true once the startup message has been ENQUEUED
    bool g_startupAcked    = false;  // true once Telegram CONFIRMED receipt (200 OK)
    bool g_awaitRestart    = false;  // true when waiting for ✅ Yes / ❌ No
}

// ---------------------------------------------------------------------------
// Command router (Telegram → actions)
// ---------------------------------------------------------------------------

static void handleCommand(const String& /*chatId*/, const String& text) {
    TelegramService& tg  = TelegramService::instance();
    StatusService&   st  = StatusService::instance();
    NotificationService& notif = NotificationService::instance();

    if (g_awaitRestart) {
        if (text == KeyboardService::kBtnYes) {
            tg.sendMessage("Restarting device...");
            delay(1000);
            esp_restart();
        } else {
            g_awaitRestart = false;
            notif.enqueue("Restart cancelled.", true);
        }
        return;
    }

    if (text == KeyboardService::kBtnStatus) {
        notif.enqueue(st.buildStatusMessage(), true);  // true = re-show keyboard with every reply

    } else if (text == KeyboardService::kBtnDeviceInfo) {
        notif.enqueue(st.buildDeviceInfoMessage(), true);

    } else if (text == KeyboardService::kBtnRestart) {
        g_awaitRestart = true;
        // Send confirmation with Yes/No keyboard
        JsonDocument doc;
        doc["chat_id"] = g_creds.telegramChatId;
        doc["text"]    = "Are you sure you want to restart the device?";
        JsonObject markup = doc["reply_markup"].to<JsonObject>();
        markup["resize_keyboard"] = true;
        JsonArray rows = markup["keyboard"].to<JsonArray>();
        JsonArray row1 = rows.add<JsonArray>();
        row1.add(KeyboardService::kBtnYes);
        row1.add(KeyboardService::kBtnNo);
        String body;
        serializeJson(doc, body);
        WiFiClientSecure client; client.setInsecure();
        HTTPClient http;
        http.begin(client, String("https://") + sentinel::constants::kTelegramApiHost +
                   "/bot" + g_creds.telegramBotToken + "/sendMessage");
        http.addHeader("Content-Type", "application/json");
        http.POST(body);
        http.end();

    } else if (text == KeyboardService::kBtnHelp || text == "/start") {
        notif.enqueue(st.buildHelpMessage(), true);  // always re-attach keyboard on help/start

    } else if (text.length() > 0) {
        // Unknown command – re-send keyboard
        notif.enqueue("Unknown command. Use the buttons below.", true);
    }
}

// ---------------------------------------------------------------------------
// bootInit
// ---------------------------------------------------------------------------

void bootInit() {
    delay(500);  // Power supply stabilization

    // --- Layer 1: Foundation ---
    LoggerService::instance().begin(sentinel::constants::kSerialBaudRate);
    LOGI("=== %s %s ===", SENTINEL_PROJECT, SENTINEL_VERSION);

    // Enable FIFO rolling file log on LittleFS (persists across reboots)
    if (LogFileService::instance().begin()) {
        LoggerService::instance().enableFileLog(true);
        LOGI("Boot: file logging active — log size %u KB / %u KB cap",
             static_cast<unsigned>(LogFileService::instance().fileSize() / 1024),
             static_cast<unsigned>(1024));  // 1 MB cap
    } else {
        LOGW("Boot: file logging unavailable — serial only");
    }

    StorageService::instance().begin();
    SystemService::instance().begin();     // increments restart count
    SchedulerService::instance().begin();

    // --- Layer 2: Provisioning ---
    ProvisioningService::instance().checkAndProvision();  // blocks if wizard needed

    const bool credLoaded = ProvisioningService::instance().loadCredentials(g_creds);
    if (!credLoaded) {
        LOGW("Boot: incomplete credentials – device will run without network");
    } else {
        LOGI("Boot: credentials loaded (SSID=%s DeviceId=%s)",
             g_creds.wifiSsid.c_str(), g_creds.deviceId.c_str());
    }

    // --- Layer 3: Network ---
    NetworkService::instance().begin();
    NotificationService::instance().begin();

    if (!g_creds.wifiSsid.isEmpty()) {
        // Wire WiFi → NTP + startup notification callback
        WiFiService::instance().onConnected([]() {
            LOGI("Boot: WiFi connected, starting NTP");
            TimeService::instance().begin();

            if (!g_startupSent) {
                // First connection this boot: enqueue the startup message
                g_startupSent = true;  // prevent duplicate enqueue on future reconnects
                SchedulerService::instance().addTask("startup-notif", 500, []() {
                    if (TimeService::instance().isSynced()) {
                        NotificationService::instance().enqueue(
                            StatusService::instance().buildStartupMessage(), true);

                        // Force an immediate drain so the "back online" message
                        // is dispatched BEFORE any stale offline commands are polled.
                        NotificationService::instance().drain();

                        // Check if the message was confirmed sent right away
                        if (NotificationService::instance().pending() == 0) {
                            g_startupAcked = true;
                            LOGI("Boot: startup message confirmed sent to Telegram ✅");
                        } else {
                            // Still in queue (network busy/rate-limit). The scheduler
                            // 'notif-drain' task will retry every 500 ms. The watcher
                            // task below logs the moment Telegram finally confirms it.
                            LOGW("Boot: startup message queued – awaiting Telegram confirmation...");
                            SchedulerService::instance().addTask("startup-ack", 1000, []() {
                                if (g_startupAcked) {
                                    // Already confirmed – self-destruct
                                    SchedulerService::instance().removeTask("startup-ack");
                                    return;
                                }
                                // When the NotificationService queue drains to zero the
                                // startup message (which was the only/first message) is gone
                                if (NotificationService::instance().pending() == 0) {
                                    g_startupAcked = true;
                                    LOGI("Boot: startup message confirmed sent to Telegram ✅");
                                    SchedulerService::instance().removeTask("startup-ack");
                                }
                            });
                        }

                        SchedulerService::instance().removeTask("startup-notif");
                    }
                });
            } else if (!g_startupAcked) {
                // Wi-Fi reconnected but startup message was not yet confirmed.
                // Drain immediately to prioritise delivery without waiting for
                // the next scheduled notif-drain tick (up to 500 ms away).
                LOGI("Boot: Wi-Fi reconnected – startup message still pending, draining now");
                NotificationService::instance().drain();
            }
        });

        WiFiService::instance().begin(g_creds.wifiSsid, g_creds.wifiPassword);
    }

    // --- Layer 4: Telegram ---
    if (!g_creds.telegramBotToken.isEmpty()) {
        TelegramService::instance().begin(g_creds.telegramBotToken, g_creds.telegramChatId);
        TelegramService::instance().onCommand(handleCommand);
    }

    // --- Layer 5: Power Monitor ---
    // Set activeLow = true since standard AC optocouplers output LOW when AC is present
    PowerMonitorService::instance().begin(sentinel::constants::kPowerMonitorPin, true);

    PowerMonitorService::instance().onPowerLost([]() {
        NotificationService::instance().enqueue(
            StatusService::instance().buildPowerLostMessage(), false);
    });

    PowerMonitorService::instance().onPowerRestored([]() {
        NotificationService::instance().enqueue(
            StatusService::instance().buildPowerRestoredMessage(), false);
    });

    // --- Layer 6: Scheduler tasks ---
    SchedulerService& sched = SchedulerService::instance();

    sched.addTask("wifi-update",    10,    []() { WiFiService::instance().update(); });
    sched.addTask("time-update",    1000,  []() { TimeService::instance().update(); });
    sched.addTask("power-update",   50,    []() { PowerMonitorService::instance().update(); });
    sched.addTask("telegram-poll",  1000,  []() { TelegramService::instance().update(); });
    sched.addTask("notif-drain",    500,   []() { NotificationService::instance().drain(); });

    // Periodic heap logging every 60 s
    sched.addTask("heap-log", 60000, []() {
        LOGI("Heap: free=%u KB, min=%u KB, uptime=%s",
             SystemService::instance().freeHeapBytes() / 1024,
             SystemService::instance().minFreeHeapBytes() / 1024,
             SystemService::instance().uptimeString().c_str());
    });

    LOGI("Boot complete. Running Sentinel Power Monitor.");
}

// ---------------------------------------------------------------------------
// bootLoop
// ---------------------------------------------------------------------------

void bootLoop() {
    SchedulerService::instance().update();
}
