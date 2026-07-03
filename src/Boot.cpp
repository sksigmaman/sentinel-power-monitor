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

#include <esp_system.h>

// ---------------------------------------------------------------------------
// Module-level state
// ---------------------------------------------------------------------------

namespace {
    sentinel::AppCredentials g_creds;
    bool g_startupSent     = false;
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
        notif.enqueue(st.buildStatusMessage(), false);

    } else if (text == KeyboardService::kBtnDeviceInfo) {
        notif.enqueue(st.buildDeviceInfoMessage(), false);

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
        notif.enqueue(st.buildHelpMessage(), false);

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

            // Send startup message once per boot
            if (!g_startupSent) {
                g_startupSent = true; // Mark as queued to prevent duplicates on reconnect
                SchedulerService::instance().addTask("startup-notif", 500, []() {
                    if (TimeService::instance().isSynced()) {
                        NotificationService::instance().enqueue(
                            StatusService::instance().buildStartupMessage(), true);
                        SchedulerService::instance().removeTask("startup-notif");
                    }
                });
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
