/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: ProvisioningService.h
 * Brief: First-boot interactive Setup Wizard via Serial Monitor.
 *        Detects Serial Monitor presence, collects credentials,
 *        validates input, and persists to NVS. Never runs again
 *        until credentials are erased.
 */

#pragma once

#include <Arduino.h>
#include "AppCredentials.h"

/**
 * @class ProvisioningService
 * @brief Singleton first-boot credential provisioning via Serial.
 *
 * Workflow:
 *  1. Check if credentials already exist in NVS → skip if yes.
 *  2. Wait 15 s for a valid keypress (printable ASCII only – filters noise).
 *  3. If keypress detected → run interactive wizard.
 *  4. Collect WiFi SSID/Password + Telegram Bot Token + Chat ID.
 *  5. Validate each field. Re-prompt on error.
 *  6. Save to NVS. Reboot.
 *
 * Can also be called from Telegram "🔄 Restart" → "RESET" to erase and re-run.
 */
class ProvisioningService {
public:
    static ProvisioningService& instance();

    ProvisioningService(const ProvisioningService&)            = delete;
    ProvisioningService& operator=(const ProvisioningService&) = delete;

    /**
     * @brief Run provisioning if credentials are not yet stored.
     *        Blocks during wizard interaction, then reboots.
     *        If no Serial Monitor is detected, returns immediately.
     */
    void checkAndProvision();

    /**
     * @brief Load credentials from NVS into the out-param.
     * @return true if credentials are complete and usable.
     */
    bool loadCredentials(sentinel::AppCredentials& out);

    /**
     * @brief Erase all stored credentials and reboot.
     */
    void factoryReset();

private:
    ProvisioningService() = default;
    ~ProvisioningService() = default;

    bool isSerialMonitorActive();
    void runWizard();
    void printBanner() const;
    void printSummary(const sentinel::AppCredentials& creds) const;

    String readLine(const char* prompt, uint32_t timeoutMs = 300000);
    bool   askYesNo(const char* prompt);

    static String maskSecret(const String& s);
    static bool validateToken(const String& token);
    static bool validateChatId(const String& chatId);
};
