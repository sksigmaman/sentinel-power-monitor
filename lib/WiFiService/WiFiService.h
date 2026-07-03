/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: WiFiService.h
 * Brief: Non-blocking WiFi connection manager with automatic reconnection.
 */

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <functional>
#include "Types.h"

using WiFiCallback = std::function<void()>;

/**
 * @class WiFiService
 * @brief Manages WiFi lifecycle: connect, monitor, and reconnect automatically.
 *
 * Never reboots. Never blocks. Reconnects indefinitely on drop.
 * Fires onConnected callback every time a connection is established
 * (including reconnects), so dependent services (NTP, Telegram) can
 * react appropriately.
 */
class WiFiService {
public:
    static WiFiService& instance();

    WiFiService(const WiFiService&)            = delete;
    WiFiService& operator=(const WiFiService&) = delete;

    /**
     * @brief Begin connecting to the given SSID. Non-blocking.
     * @param ssid      WiFi network name.
     * @param password  WiFi password.
     */
    void begin(const String& ssid, const String& password);

    /**
     * @brief Must be called every loop() iteration to drive the state machine.
     */
    void update();

    // -----------------------------------------------------------------------
    // State queries
    // -----------------------------------------------------------------------
    bool isConnected()   const;
    sentinel::WifiState state() const { return state_; }

    String ipAddress()   const;
    String ssid()        const { return ssid_; }
    int8_t rssi()        const;
    String gatewayIp()   const;
    String subnetMask()  const;

    // -----------------------------------------------------------------------
    // Callbacks
    // -----------------------------------------------------------------------
    /** @brief Register a callback fired on every successful (re)connection. */
    void onConnected(WiFiCallback cb)    { onConnectedCb_    = cb; }
    /** @brief Register a callback fired when connection is lost. */
    void onDisconnected(WiFiCallback cb) { onDisconnectedCb_ = cb; }

private:
    WiFiService() = default;
    ~WiFiService() = default;

    void tryConnect();
    void handleConnected();
    void handleDisconnected();

    String               ssid_;
    String               password_;
    sentinel::WifiState  state_           = sentinel::WifiState::IDLE;
    uint32_t             lastAttemptMs_   = 0;
    uint32_t             connectStartMs_  = 0;
    bool                 wasConnected_    = false;

    WiFiCallback onConnectedCb_;
    WiFiCallback onDisconnectedCb_;
};
