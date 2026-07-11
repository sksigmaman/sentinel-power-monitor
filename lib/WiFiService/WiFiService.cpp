/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: WiFiService.cpp
 */

#include "WiFiService.h"
#include "LoggerService.h"
#include "Constants.h"

using namespace sentinel::constants;

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

WiFiService& WiFiService::instance() {
    static WiFiService inst;
    return inst;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void WiFiService::begin(const String& ssid, const String& password) {
    ssid_     = ssid;
    password_ = password;

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);  // We manage reconnection ourselves

    LOGI("WiFiService: starting connection to '%s'", ssid_.c_str());
    state_         = sentinel::WifiState::CONNECTING;
    connectStartMs_ = millis();
    tryConnect();
}

// ---------------------------------------------------------------------------
// Update (called every loop)
// ---------------------------------------------------------------------------

void WiFiService::update() {
    const bool nowConnected = (WiFi.status() == WL_CONNECTED);

    switch (state_) {
        case sentinel::WifiState::CONNECTING:
            if (nowConnected) {
                handleConnected();
            } else if ((millis() - connectStartMs_) > kWifiConnectTimeoutMs) {
                LOGW("WiFiService: connection timed out, will retry in %u ms", kWifiReconnectDelayMs);
                state_       = sentinel::WifiState::DISCONNECTED;
                lastAttemptMs_ = millis();
            }
            break;

        case sentinel::WifiState::CONNECTED:
            if (!nowConnected) {
                handleDisconnected();
            }
            break;

        case sentinel::WifiState::DISCONNECTED:
        case sentinel::WifiState::RECONNECTING:
            if (nowConnected) {
                handleConnected();
            } else if ((millis() - lastAttemptMs_) >= kWifiReconnectDelayMs) {
                LOGI("WiFiService: retrying connection to '%s'...", ssid_.c_str());
                state_          = sentinel::WifiState::RECONNECTING;
                connectStartMs_ = millis();
                lastAttemptMs_  = millis();  // reset so the next retry waits a full kWifiReconnectDelayMs
                tryConnect();
            }
            break;

        case sentinel::WifiState::IDLE:
        default:
            break;
    }
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

void WiFiService::tryConnect() {
    WiFi.disconnect(true);
    delay(100);
    WiFi.begin(ssid_.c_str(), password_.c_str());
}

void WiFiService::handleConnected() {
    state_        = sentinel::WifiState::CONNECTED;
    wasConnected_ = true;
    LOGI("WiFiService: connected to '%s' IP=%s RSSI=%d dBm",
         WiFi.SSID().c_str(),
         WiFi.localIP().toString().c_str(),
         WiFi.RSSI());

    if (onConnectedCb_) onConnectedCb_();
}

void WiFiService::handleDisconnected() {
    state_         = sentinel::WifiState::DISCONNECTED;
    lastAttemptMs_ = millis();
    LOGW("WiFiService: connection lost, reconnecting in %u ms...", kWifiReconnectDelayMs);

    if (onDisconnectedCb_) onDisconnectedCb_();
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

bool WiFiService::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

String WiFiService::ipAddress() const {
    return WiFi.localIP().toString();
}

int8_t WiFiService::rssi() const {
    return static_cast<int8_t>(WiFi.RSSI());
}

String WiFiService::gatewayIp() const {
    return WiFi.gatewayIP().toString();
}

String WiFiService::subnetMask() const {
    return WiFi.subnetMask().toString();
}
