/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: LogFileService.cpp
 */

#include "LogFileService.h"

// NOTE: Do NOT include LoggerService.h here — would cause a circular
// include chain (LoggerService → LogFileService → LoggerService).
// All internal diagnostics in this file go directly to Serial.printf().

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

LogFileService& LogFileService::instance() {
    static LogFileService inst;
    return inst;
}

// ---------------------------------------------------------------------------
// begin()
// ---------------------------------------------------------------------------

bool LogFileService::begin() {
    if (ready_) return true;

    // Mount LittleFS; pass true to auto-format on first use or after corruption
    if (!LittleFS.begin(true)) {
        Serial.println("[LogFileService] CRITICAL: LittleFS mount failed — file logging disabled");
        return false;
    }

    // Create the /logs directory if it does not exist
    if (!LittleFS.exists(kLogDir)) {
        if (!LittleFS.mkdir(kLogDir)) {
            Serial.println("[LogFileService] CRITICAL: could not create /logs directory");
            LittleFS.end();
            return false;
        }
    }

    // Read current file size (the log may persist across reboots)
    if (LittleFS.exists(kLogPath)) {
        File f = LittleFS.open(kLogPath, "r");
        if (f) {
            size_ = f.size();
            f.close();
        }
    }

    ready_ = true;

    Serial.printf("[LogFileService] LittleFS mounted OK. Log: %s (%u bytes / %u KB cap)\r\n",
                  kLogPath,
                  static_cast<unsigned>(size_),
                  static_cast<unsigned>(kLogMaxBytes / 1024));

    // Remove any leftover temp file from a previous interrupted trim
    if (LittleFS.exists(kTmpPath)) {
        LittleFS.remove(kTmpPath);
        Serial.println("[LogFileService] Removed stale temp file from previous trim");
    }

    return true;
}

// ---------------------------------------------------------------------------
// writeLine()
// ---------------------------------------------------------------------------

void LogFileService::writeLine(const char* line) {
    if (!ready_) return;

    File f = LittleFS.open(kLogPath, "a");
    if (!f) {
        // Filesystem may have become full or corrupt — disable to avoid spam
        Serial.println("[LogFileService] ERROR: could not open log for append — disabling file log");
        ready_ = false;
        return;
    }

    f.println(line);     // println appends \r\n on Arduino, \n on LittleFS
    size_ = f.size();
    f.close();

    // Trigger FIFO trim if cap exceeded
    if (size_ >= kLogMaxBytes) {
        trim();
    }
}

// ---------------------------------------------------------------------------
// trim() — FIFO: drop oldest kLogTrimBytes, keep the rest
// ---------------------------------------------------------------------------

void LogFileService::trim() {
    Serial.printf("[LogFileService] Log cap reached (%u KB). Trimming oldest %u KB...\r\n",
                  static_cast<unsigned>(kLogMaxBytes  / 1024),
                  static_cast<unsigned>(kLogTrimBytes / 1024));

    File src = LittleFS.open(kLogPath, "r");
    if (!src) {
        Serial.println("[LogFileService] ERROR: trim() could not open source file");
        return;
    }

    const size_t fileLen = src.size();

    // Guard: nothing to trim if file is somehow smaller than the trim boundary
    if (fileLen <= kLogTrimBytes) {
        src.close();
        Serial.println("[LogFileService] WARN: file smaller than trim boundary — skipping trim");
        return;
    }

    // Seek to the trim boundary
    src.seek(static_cast<uint32_t>(kLogTrimBytes));

    // Advance forward until we land on a complete line (skip partial line)
    while (src.available()) {
        const char c = static_cast<char>(src.read());
        if (c == '\n') break;
    }

    if (!src.available()) {
        // Edge case: trim boundary was at the very last line — clear the file
        src.close();
        LittleFS.remove(kLogPath);
        size_ = 0;
        Serial.println("[LogFileService] WARN: trim boundary beyond last line — log cleared");
        return;
    }

    // Copy everything after the trim point into the temp file in 4 KB chunks
    File dst = LittleFS.open(kTmpPath, "w");
    if (!dst) {
        src.close();
        Serial.println("[LogFileService] ERROR: trim() could not create temp file");
        return;
    }

    uint8_t buf[kCopyChunk];
    while (src.available()) {
        const size_t n = src.read(buf, kCopyChunk);
        if (n > 0) {
            dst.write(buf, n);
        }
    }

    src.close();
    const size_t newSize = dst.size();
    dst.close();

    // Atomically replace the original log with the trimmed version
    LittleFS.remove(kLogPath);
    LittleFS.rename(kTmpPath, kLogPath);
    size_ = newSize;

    Serial.printf("[LogFileService] Trim complete. New log size: %u KB (was %u KB)\r\n",
                  static_cast<unsigned>(newSize   / 1024),
                  static_cast<unsigned>(fileLen   / 1024));
}

// ---------------------------------------------------------------------------
// clear()
// ---------------------------------------------------------------------------

void LogFileService::clear() {
    if (!ready_) return;
    LittleFS.remove(kLogPath);
    size_ = 0;
    Serial.println("[LogFileService] Log file cleared");
}
