/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: LogFileService.h
 * Brief: FIFO rolling log file stored on LittleFS.
 *        Appends every log line to /logs/sentinel.log.
 *        When the file exceeds kLogMaxBytes (1 MB), the oldest
 *        kLogTrimBytes (256 KB) are silently dropped so the file
 *        never grows beyond the cap.
 *
 * Flash layout (4 MB, custom partitions.csv):
 *   factory  app  2.56 MB  firmware
 *   spiffs  data  1.37 MB  LittleFS  ← this service lives here
 */

#pragma once

#include <Arduino.h>
#include <LittleFS.h>

/**
 * @class LogFileService
 * @brief Singleton FIFO rolling log file on LittleFS.
 *
 * Usage:
 *   LogFileService::instance().begin();          // once, after Serial
 *   LogFileService::instance().writeLine(line);  // called by LoggerService
 *
 * IMPORTANT: writeLine() must NOT call LOGI/LOGW/LOGE/LOGD to avoid
 * infinite recursion.  Internal diagnostics go directly to Serial.
 */
class LogFileService {
public:
    static LogFileService& instance();

    LogFileService(const LogFileService&)            = delete;
    LogFileService& operator=(const LogFileService&) = delete;

    /**
     * @brief Mount LittleFS and open (or create) the log file.
     *        Safe to call multiple times – subsequent calls are no-ops.
     * @return true if the filesystem is mounted and the log is ready.
     */
    bool begin();

    /**
     * @brief Append one pre-formatted log line to the file.
     *        Triggers a FIFO trim automatically if the file exceeds kLogMaxBytes.
     *        Must NOT use LOGI/LOGW/LOGE/LOGD internally.
     */
    void writeLine(const char* line);

    /** @brief Current log file size in bytes (updated on every write). */
    size_t fileSize() const { return size_; }

    /** @brief Erase the log file entirely (e.g. factory reset). */
    void clear();

    /** @brief True if LittleFS mounted and log file is accessible. */
    bool isReady() const { return ready_; }

private:
    LogFileService() = default;
    ~LogFileService() = default;

    /**
     * @brief Drop the oldest kLogTrimBytes from the log file.
     *        Uses 4 KB chunk copy to avoid large heap allocations.
     */
    void trim();

    bool   ready_ = false;
    size_t size_  = 0;

    // ------------------------------------------------------------------
    // Tuneable constants
    // ------------------------------------------------------------------

    /** Maximum log file size before a trim is triggered. */
    static constexpr size_t kLogMaxBytes  = 1024UL * 1024UL;   // 1 MB

    /** How many bytes to drop from the front on each trim (~25 % of cap). */
    static constexpr size_t kLogTrimBytes = 256UL  * 1024UL;   // 256 KB

    /** Chunk size used during the file-copy in trim(). */
    static constexpr size_t kCopyChunk   = 4096;

    static constexpr const char* kLogDir  = "/logs";
    static constexpr const char* kLogPath = "/logs/sentinel.log";
    static constexpr const char* kTmpPath = "/logs/sentinel.tmp";
};
