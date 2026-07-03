/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: Types.h
 * Brief: Shared enums, structs, and type aliases used across modules.
 */

#pragma once

#include <Arduino.h>

namespace sentinel {

// ---------------------------------------------------------------------------
// Power state
// ---------------------------------------------------------------------------
enum class PowerState : uint8_t {
    UNKNOWN   = 0,
    AVAILABLE = 1,
    LOST      = 2,
};

// ---------------------------------------------------------------------------
// WiFi state
// ---------------------------------------------------------------------------
enum class WifiState : uint8_t {
    IDLE         = 0,
    CONNECTING   = 1,
    CONNECTED    = 2,
    DISCONNECTED = 3,
    RECONNECTING = 4,
};

// ---------------------------------------------------------------------------
// Network readiness
// ---------------------------------------------------------------------------
enum class NetworkStatus : uint8_t {
    OFFLINE  = 0,
    ONLINE   = 1,
};

// (NotifPriority removed – LOW/HIGH clash with Arduino GPIO macros)

// ---------------------------------------------------------------------------
// Queued notification entry (fixed-size for predictable memory)
// ---------------------------------------------------------------------------
struct QueuedMessage {
    char    text[600];
    bool    withKeyboard = false;
    bool    used         = false;
};

}  // namespace sentinel
