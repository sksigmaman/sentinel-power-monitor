/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: LoggerService.h
 * Brief: Timestamped, tagged Serial logging. Thread-safe via critical section.
 */

#pragma once

#include <Arduino.h>
#include <stdarg.h>

// ---------------------------------------------------------------------------
// Convenience macros (file/line context injected automatically)
// ---------------------------------------------------------------------------
#define LOGI(fmt, ...) LoggerService::instance().log(LogLevel::INFO,  fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) LoggerService::instance().log(LogLevel::WARN,  fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) LoggerService::instance().log(LogLevel::ERROR_, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) LoggerService::instance().log(LogLevel::DEBUG, fmt, ##__VA_ARGS__)

enum class LogLevel : uint8_t {
    DEBUG  = 0,
    INFO   = 1,
    WARN   = 2,
    ERROR_ = 3,  // trailing _ avoids clash with errno.h ERROR macro
};

/**
 * @class LoggerService
 * @brief Singleton serial logger with level filtering and timestamp prefix.
 *
 * Uses `millis()` for timestamps before NTP sync and formatted time after.
 */
class LoggerService {
public:
    static LoggerService& instance();

    LoggerService(const LoggerService&)             = delete;
    LoggerService& operator=(const LoggerService&)  = delete;

    /**
     * @brief Initialize Serial at the given baud rate.
     * @param baudRate Serial speed (default 115200).
     * @param minLevel Minimum log level to print (default INFO).
     */
    void begin(uint32_t baudRate = 115200, LogLevel minLevel = LogLevel::INFO);

    /**
     * @brief Emit a formatted log line with level tag and timestamp.
     */
    void log(LogLevel level, const char* fmt, ...) __attribute__((format(printf, 3, 4)));

    /** @brief Set runtime minimum log level. */
    void setLevel(LogLevel level) { minLevel_ = level; }

    /** @brief Mask a sensitive string for safe logging. Returns "****". */
    static const char* mask(const String& secret);

private:
    LoggerService() = default;
    ~LoggerService() = default;

    LogLevel minLevel_ = LogLevel::INFO;
    bool     begun_    = false;

    static const char* levelTag(LogLevel level);
};
