/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: ProvisioningService.cpp
 */

#include "ProvisioningService.h"
#include "StorageService.h"
#include "LoggerService.h"
#include "Constants.h"

#include <esp_system.h>

using namespace sentinel::constants::nvs;

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

ProvisioningService& ProvisioningService::instance() {
    static ProvisioningService inst;
    return inst;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void ProvisioningService::checkAndProvision() {
    StorageService& s = StorageService::instance();

    // Check if already provisioned
    const bool hasWifi = s.contains(kWifiSsid) && s.contains(kWifiPass);
    const bool hasTg   = s.contains(kBotToken) && s.contains(kChatId);

    if (hasWifi && hasTg) {
        LOGI("Provisioning: credentials found, skipping wizard");
        return;
    }

    Serial.println();
    Serial.println("No credentials found.");
    Serial.println("--> PRESS ANY KEY within 15 seconds to enter Setup Wizard...");

    if (!isSerialMonitorActive()) {
        Serial.println("No input detected. Booting without credentials.");
        LOGW("Provisioning: no serial detected, booting without credentials");
        return;
    }

    runWizard();
}

bool ProvisioningService::loadCredentials(sentinel::AppCredentials& out) {
    StorageService& s = StorageService::instance();

    out.wifiSsid          = s.getString(kWifiSsid, "");
    out.wifiPassword      = s.getString(kWifiPass, "");
    out.telegramBotToken  = s.getString(kBotToken, "");
    out.telegramChatId    = s.getString(kChatId, "");
    out.deviceId          = s.getString(kDeviceId, "");

    // Generate device ID if not set
    if (out.deviceId.isEmpty()) {
        const uint64_t mac = ESP.getEfuseMac();
        char buf[24];
        snprintf(buf, sizeof(buf), "SEN-%04X%08X",
                 static_cast<uint16_t>((mac >> 32) & 0xFFFF),
                 static_cast<uint32_t>(mac));
        out.deviceId = String(buf);
        s.putString(kDeviceId, out.deviceId);
        LOGI("Provisioning: generated deviceId=%s", out.deviceId.c_str());
    }

    return out.isComplete();
}

void ProvisioningService::factoryReset() {
    LOGW("Provisioning: factory reset requested");
    Serial.println("Erasing all credentials...");
    StorageService::instance().clear();
    Serial.println("Done. Restarting...");
    delay(500);
    esp_restart();
}

// ---------------------------------------------------------------------------
// Serial Monitor detection (with noise filtering)
// ---------------------------------------------------------------------------

bool ProvisioningService::isSerialMonitorActive() {
    const unsigned long startMs = millis();
    int lastRemaining = -1;

    while (true) {
        const unsigned long elapsed = millis() - startMs;
        if (elapsed >= 15000) break;

        const int remaining = 15 - static_cast<int>(elapsed / 1000);
        if (remaining != lastRemaining) {
            Serial.printf("Waiting... %d s \r", remaining);
            lastRemaining = remaining;
        }

        if (Serial.available() > 0) {
            bool validKey = false;
            while (Serial.available() > 0) {
                const int c = Serial.read();
                // Only printable ASCII or newline – ignores floating-pin noise bytes
                if ((c >= 32 && c <= 126) || c == '\r' || c == '\n') {
                    validKey = true;
                }
            }
            if (validKey) {
                Serial.println("\nSerial Monitor detected!");
                return true;
            }
        }
        delay(50);
    }
    Serial.println("\nNo input. Skipping setup.");
    return false;
}

// ---------------------------------------------------------------------------
// Wizard
// ---------------------------------------------------------------------------

void ProvisioningService::runWizard() {
    sentinel::AppCredentials creds;
    printBanner();

    // -- WiFi SSID
    while (true) {
        creds.wifiSsid = readLine("Enter WiFi SSID");
        if (!creds.wifiSsid.isEmpty()) break;
        Serial.println("Error: SSID cannot be empty.");
    }

    // -- WiFi Password
    while (true) {
        creds.wifiPassword = readLine("Enter WiFi Password");
        if (!creds.wifiPassword.isEmpty()) break;
        Serial.println("Error: Password cannot be empty.");
    }

    // -- Telegram Bot Token
    while (true) {
        creds.telegramBotToken = readLine("Enter Telegram Bot Token (e.g. 123456:ABC-DEF...)");
        if (validateToken(creds.telegramBotToken)) break;
        Serial.println("Error: Invalid token. Must contain ':' and not be empty.");
    }

    // -- Telegram Chat ID
    while (true) {
        creds.telegramChatId = readLine("Enter Telegram Chat ID (e.g. 8855654117)");
        if (validateChatId(creds.telegramChatId)) break;
        Serial.println("Error: Chat ID cannot be empty.");
    }

    printSummary(creds);

    if (!askYesNo("Save configuration?")) {
        Serial.println("Configuration discarded. Rebooting...");
        delay(500);
        esp_restart();
    }

    // Save to NVS
    StorageService& s = StorageService::instance();
    s.putString(kWifiSsid,  creds.wifiSsid);
    s.putString(kWifiPass,  creds.wifiPassword);
    s.putString(kBotToken,  creds.telegramBotToken);
    s.putString(kChatId,    creds.telegramChatId);

    Serial.println("\n  ✓ WiFi SSID");
    Serial.println("  ✓ WiFi Password");
    Serial.println("  ✓ Telegram Bot Token");
    Serial.println("  ✓ Telegram Chat ID");
    Serial.println("\nCredentials saved. Restarting...\n");
    delay(500);
    esp_restart();
}

// ---------------------------------------------------------------------------
// UI helpers
// ---------------------------------------------------------------------------

void ProvisioningService::printBanner() const {
    Serial.println("\n=================================");
    Serial.println("  Sentinel Power Monitor");
    Serial.println("  Setup Wizard");
    Serial.println("  Type EXIT to quit at any prompt");
    Serial.println("=================================\n");
}

void ProvisioningService::printSummary(const sentinel::AppCredentials& creds) const {
    Serial.println("\n--- Configuration Summary ---");
    Serial.print("WiFi SSID     : "); Serial.println(creds.wifiSsid);
    Serial.print("WiFi Password : "); Serial.println(maskSecret(creds.wifiPassword));
    Serial.print("Bot Token     : "); Serial.println(maskSecret(creds.telegramBotToken));
    Serial.print("Chat ID       : "); Serial.println(creds.telegramChatId);
    Serial.println("-----------------------------\n");
}

String ProvisioningService::readLine(const char* prompt, uint32_t timeoutMs) {
    Serial.println();
    Serial.print(prompt);
    Serial.print("\n> ");

    String buf;
    const unsigned long startMs = millis();

    while (true) {
        if ((millis() - startMs) > timeoutMs) {
            Serial.println("\n[Input timed out]");
            return String();
        }
        if (Serial.available()) {
            const char ch = static_cast<char>(Serial.read());
            if (ch == '\r') continue;
            if (ch == '\n') break;
            buf += ch;
        }
    }

    buf.trim();
    if (buf.equalsIgnoreCase("EXIT")) {
        Serial.println("Exiting wizard. Rebooting...");
        delay(500);
        esp_restart();
    }

    return buf;
}

bool ProvisioningService::askYesNo(const char* prompt) {
    while (true) {
        Serial.print(prompt);
        Serial.print(" (Y/N): ");
        const String ans = readLine("");
        if (ans.equalsIgnoreCase("Y") || ans.equalsIgnoreCase("YES")) return true;
        if (ans.equalsIgnoreCase("N") || ans.equalsIgnoreCase("NO"))  return false;
        Serial.println("Please type Y or N.");
    }
}

// ---------------------------------------------------------------------------
// Validation
// ---------------------------------------------------------------------------

String ProvisioningService::maskSecret(const String& s) {
    if (s.length() <= 4) return "****";
    return String("****") + " (" + String(s.length()) + " chars)";
}

bool ProvisioningService::validateToken(const String& token) {
    return !token.isEmpty() && token.indexOf(':') > 0;
}

bool ProvisioningService::validateChatId(const String& chatId) {
    return !chatId.isEmpty();
}
