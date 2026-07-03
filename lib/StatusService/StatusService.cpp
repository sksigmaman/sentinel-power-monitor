/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: StatusService.cpp
 */

#include "StatusService.h"
#include "NetworkService.h"
#include "TimeService.h"
#include "SystemService.h"
#include "PowerMonitorService.h"
#include "LoggerService.h"
#include "Version.h"

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

StatusService& StatusService::instance() {
    static StatusService inst;
    return inst;
}

// ---------------------------------------------------------------------------
// Helpers (local scope)
// ---------------------------------------------------------------------------

static String freeHeapKb() {
    char buf[16];
    snprintf(buf, sizeof(buf), "%u KB", SystemService::instance().freeHeapBytes() / 1024);
    return String(buf);
}

static String rssiStr() {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d dBm", NetworkService::instance().rssi());
    return String(buf);
}

// ---------------------------------------------------------------------------
// Startup message
// ---------------------------------------------------------------------------

String StatusService::buildStartupMessage() const {
    const TimeService& time = TimeService::instance();

    String msg;
    msg += "\xF0\x9F\x98\x87 <b>Sentinel Power Monitor</b>\n\n";
    msg += "\xE2\x9C\x85 Electricity Available\n";
    msg += "\xE2\x9C\x85 Wi-Fi Connected\n\n";
    msg += "\xF0\x9F\x93\x85 <b>Date:</b> " + time.currentDate() + "\n\n";
    msg += "\xF0\x9F\x95\x92 <b>Time:</b> " + time.currentTime();
    return msg;
}

// ---------------------------------------------------------------------------
// Status message
// ---------------------------------------------------------------------------

String StatusService::buildStatusMessage() const {
    const NetworkService&     net  = NetworkService::instance();
    const TimeService&        time = TimeService::instance();
    const SystemService&      sys  = SystemService::instance();

    String msg;
    msg += "\xF0\x9F\x93\x8A <b>Sentinel Power Monitor</b>\n\n";
    msg += "\xF0\x9F\x9F\xA2 <b>Device</b> : Online\n";
    msg += "\xE2\x9A\xA1 <b>Electricity</b> : Available\n";
    msg += "\xF0\x9F\x93\xB6 <b>Wi-Fi</b> : Connected\n";
    msg += "\xF0\x9F\x93\xA1 <b>RSSI</b> : " + rssiStr() + "\n";
    msg += "\xF0\x9F\x8C\x90 <b>IP</b> : " + net.ipAddress() + "\n";
    msg += "\xE2\x8F\xB1 <b>Uptime</b> : " + sys.uptimeString() + "\n";
    msg += "\xF0\x9F\x93\x85 <b>Date</b> : " + time.currentDate() + "\n";
    msg += "\xF0\x9F\x95\x92 <b>Time</b> : " + time.currentTime();
    return msg;
}

// ---------------------------------------------------------------------------
// Device info message
// ---------------------------------------------------------------------------

String StatusService::buildDeviceInfoMessage() const {
    const NetworkService& net = NetworkService::instance();
    const SystemService&  sys = SystemService::instance();
    const TimeService&   time = TimeService::instance();

    char buf[1024];
    snprintf(buf, sizeof(buf),
        "\xE2\x84\xB9 <b>Device Info</b>\n\n"
        "<pre>"
        "Chip Model       : %s\n"
        "Chip Revision    : %s\n"
        "CPU Frequency    : %u MHz\n"
        "Flash Size       : %u MB\n"
        "Flash Speed      : %u MHz\n"
        "Flash Mode       : %s\n"
        "SDK Version      : %s\n"
        "Firmware         : %s\n"
        "MAC Address      : %s\n"
        "IP Address       : %s\n"
        "RSSI             : %s\n"
        "Free Heap        : %s\n"
        "Min Free Heap    : %u KB\n"
        "Largest Block    : %u KB\n"
        "PSRAM            : %s\n"
        "Restart Count    : %u\n"
        "Reset Reason     : %s\n"
        "Date             : %s\n"
        "Time             : %s"
        "</pre>",
        sys.chipModel().c_str(),
        sys.chipRevision().c_str(),
        sys.cpuFreqMhz(),
        sys.flashSizeMB(),
        sys.flashSpeedMhz(),
        sys.flashMode().c_str(),
        sys.sdkVersion().c_str(),
        SENTINEL_VERSION,
        WiFi.macAddress().c_str(),
        net.ipAddress().c_str(),
        rssiStr().c_str(),
        freeHeapKb().c_str(),
        sys.minFreeHeapBytes() / 1024,
        sys.largestFreeBlock() / 1024,
        sys.hasPsram() ? "Yes" : "No",
        sys.restartCount(),
        sys.resetReasonString().c_str(),
        time.currentDate().c_str(),
        time.currentTime().c_str()
    );
    return String(buf);
}

// ---------------------------------------------------------------------------
// Power event messages
// ---------------------------------------------------------------------------

String StatusService::buildPowerLostMessage() const {
    const TimeService& time = TimeService::instance();
    String msg;
    msg += "\xF0\x9F\x94\xB4 Electricity Lost\n";
    msg += divider() + "\n";
    msg += "Power supply has been interrupted.\n\n";
    msg += "\xF0\x9F\x93\x85 Date\n" + time.currentDate() + "\n\n";
    msg += "\xF0\x9F\x95\x92 Time\n" + time.currentTime() + "\n";
    msg += divider();
    return msg;
}

String StatusService::buildPowerRestoredMessage() const {
    const TimeService& time = TimeService::instance();
    String msg;
    msg += "\xF0\x9F\x9F\xA2 Electricity Restored\n";
    msg += divider() + "\n";
    msg += "Power supply has been restored.\n\n";
    msg += "\xF0\x9F\x93\x85 Date\n" + time.currentDate() + "\n\n";
    msg += "\xF0\x9F\x95\x92 Time\n" + time.currentTime() + "\n";
    msg += divider();
    return msg;
}

// ---------------------------------------------------------------------------
// Help message
// ---------------------------------------------------------------------------

String StatusService::buildHelpMessage() const {
    String msg;
    msg += "\xE2\x9D\x93 Sentinel Help\n";    // ❓
    msg += divider() + "\n";
    msg += "\xF0\x9F\x93\x8A Status\n";       // 📊
    msg += "Shows live device status including electricity, Wi-Fi, heap, and uptime.\n\n";
    msg += "\xE2\x84\xB9 Device Info\n";      // ℹ
    msg += "Shows full hardware details: chip model, flash, SDK, MAC, IP, and more.\n\n";
    msg += "\xF0\x9F\x94\x84 Restart\n";     // 🔄
    msg += "Restarts the ESP32 remotely. Will ask for confirmation first.\n\n";
    msg += "\xE2\x9D\x93 Help\n";            // ❓
    msg += "Shows this help message.\n\n";
    msg += "Automatic Alerts:\n";
    msg += "  \xF0\x9F\x9F\xA2 Sent when electricity is restored.\n";
    msg += divider();
    return msg;
}
