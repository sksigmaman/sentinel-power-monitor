/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: LoggerService.cpp
 */

#include "LoggerService.h"
#include "LogFileService.h"

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

LoggerService& LoggerService::instance() {
    static LoggerService inst;
    return inst;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void LoggerService::begin(uint32_t baudRate, LogLevel minLevel) {
    if (begun_) return;
    Serial.begin(baudRate);
    delay(100);  // brief settle after Serial.begin()
    minLevel_ = minLevel;
    begun_    = true;
}

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------

void LoggerService::log(LogLevel level, const char* fmt, ...) {
    if (!begun_ || level < minLevel_) return;

    // Timestamp: millis()-based until NTP is available
    char ts[20];
    const unsigned long ms = millis();
    const unsigned long s  = ms / 1000;
    snprintf(ts, sizeof(ts), "[%5lu.%03lu]", s, ms % 1000);

    // Format the user message
    char msg[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    Serial.printf("%s [%s] %s\r\n", ts, levelTag(level), msg);

    // Dual-write to flash log file when enabled.
    // We build the full formatted line on the stack (no heap alloc) and
    // hand it off to LogFileService, which handles appending and FIFO trim.
    if (fileLogEnabled_) {
        char fullLine[300];
        snprintf(fullLine, sizeof(fullLine), "%s [%s] %s", ts, levelTag(level), msg);
        LogFileService::instance().writeLine(fullLine);
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

const char* LoggerService::levelTag(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:  return "DBG";
        case LogLevel::INFO:   return "INF";
        case LogLevel::WARN:   return "WRN";
        case LogLevel::ERROR_: return "ERR";
    }
    return "???";
}

const char* LoggerService::mask(const String& /*secret*/) {
    return "****";
}
