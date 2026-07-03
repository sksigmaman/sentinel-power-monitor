/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: NetworkService.cpp
 */

#include "NetworkService.h"
#include "LoggerService.h"

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

NetworkService& NetworkService::instance() {
    static NetworkService inst;
    return inst;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void NetworkService::begin() {
    LOGI("NetworkService: ready");
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

bool NetworkService::isOnline() const {
    return WiFiService::instance().isConnected();
}

int8_t NetworkService::rssi() const {
    return WiFiService::instance().rssi();
}

String NetworkService::ipAddress() const {
    return WiFiService::instance().ipAddress();
}

String NetworkService::ssid() const {
    return WiFiService::instance().ssid();
}
