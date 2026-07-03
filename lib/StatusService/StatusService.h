/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: StatusService.h
 * Brief: Builds live status and device info message text for Telegram replies.
 *        Reads data from other services on demand – never caches state itself.
 */

#pragma once

#include <Arduino.h>

/**
 * @class StatusService
 * @brief Singleton message builder for 📊 Status and ℹ Device Info replies.
 *
 * All message text is assembled here so the command router stays thin.
 * No HTTP calls – purely formats data into a String for TelegramService.
 */
class StatusService {
public:
    static StatusService& instance();

    StatusService(const StatusService&)            = delete;
    StatusService& operator=(const StatusService&) = delete;

    /**
     * @brief Build the live 📊 Status message.
     * @return Formatted UTF-8 string ready to send via Telegram.
     */
    String buildStatusMessage() const;

    /**
     * @brief Build the ℹ Device Info message.
     * @return Formatted UTF-8 string ready to send via Telegram.
     */
    String buildDeviceInfoMessage() const;

    /**
     * @brief Build the 🟢 Device Online startup message.
     * @return Formatted UTF-8 string ready to send via Telegram.
     */
    String buildStartupMessage() const;

    /**
     * @brief Build the 🔴 Electricity Lost notification.
     * @return Formatted UTF-8 string.
     */
    String buildPowerLostMessage() const;

    /**
     * @brief Build the 🟢 Electricity Restored notification.
     * @return Formatted UTF-8 string.
     */
    String buildPowerRestoredMessage() const;

    /**
     * @brief Build the ❓ Help message.
     * @return Formatted UTF-8 string.
     */
    String buildHelpMessage() const;

private:
    StatusService() = default;
    ~StatusService() = default;

    static String divider() { return "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"; }
};
