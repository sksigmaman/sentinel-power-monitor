/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: AppCredentials.h
 * Brief: Runtime credential bundle loaded from NVS at boot.
 *        Never stored in global flash literals.
 */

#pragma once

#include <Arduino.h>

namespace sentinel {

/**
 * @brief All user-provisioned credentials loaded from NVS at startup.
 *
 * Populated by StorageService. Passed to consuming services by reference.
 * Never printed in full – always masked in logs.
 */
struct AppCredentials {
    String wifiSsid;
    String wifiPassword;
    String telegramBotToken;   ///< e.g. "123456789:ABCDef..."
    String telegramChatId;     ///< e.g. "8855654117"
    String deviceId;           ///< e.g. "SEN-A1B2C3D4" (auto-generated)

    /** True if minimum credentials are present to boot normally. */
    bool isComplete() const {
        return !wifiSsid.isEmpty()
            && !wifiPassword.isEmpty()
            && !telegramBotToken.isEmpty()
            && !telegramChatId.isEmpty();
    }
};

}  // namespace sentinel
