/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: NetworkService.h
 * Brief: Lightweight wrapper around WiFiService providing a simple isOnline()
 *        gate used by all HTTP consumers before attempting connections.
 */

#pragma once

#include <Arduino.h>
#include "WiFiService.h"
#include "Types.h"

/**
 * @class NetworkService
 * @brief Singleton network availability facade.
 *
 * All services that need to send HTTP requests call isOnline() first.
 * This service holds no connection logic – that lives in WiFiService.
 */
class NetworkService {
public:
    static NetworkService& instance();

    NetworkService(const NetworkService&)            = delete;
    NetworkService& operator=(const NetworkService&) = delete;

    void begin();

    /** @brief True if WiFi is connected and an IP is assigned. */
    bool isOnline() const;

    /** @brief Current RSSI in dBm. Returns 0 if not connected. */
    int8_t rssi() const;

    /** @brief Current IP address string. */
    String ipAddress() const;

    /** @brief WiFi SSID currently connected to. */
    String ssid() const;

private:
    NetworkService() = default;
    ~NetworkService() = default;
};
