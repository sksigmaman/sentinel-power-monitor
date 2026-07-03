/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: StorageService.h
 * Brief: Thin, type-safe wrapper around Arduino Preferences (NVS).
 *        Single namespace "sentinel". All reads/writes go through here.
 */

#pragma once

#include <Arduino.h>
#include <Preferences.h>

/**
 * @class StorageService
 * @brief Singleton NVS storage accessor.
 *
 * All persistent data (credentials, restart count, device ID) is stored
 * exclusively through this service. Preferences is opened once at begin()
 * and closed only on explicit close() or destructor.
 */
class StorageService {
public:
    static StorageService& instance();

    StorageService(const StorageService&)            = delete;
    StorageService& operator=(const StorageService&) = delete;

    /**
     * @brief Open the NVS namespace. Must be called before any read/write.
     * @return true on success.
     */
    bool begin();

    /** @brief True if begin() succeeded. */
    bool isReady() const { return ready_; }

    // -----------------------------------------------------------------------
    // String
    // -----------------------------------------------------------------------
    /** @brief Read a String value. Returns defaultVal if key not found. */
    String  getString(const char* key, const String& defaultVal = "") const;
    /** @brief Write a String value. Returns true on success. */
    bool    putString(const char* key, const String& value);

    // -----------------------------------------------------------------------
    // Unsigned integer
    // -----------------------------------------------------------------------
    uint32_t getUInt(const char* key, uint32_t defaultVal = 0) const;
    bool     putUInt(const char* key, uint32_t value);

    // -----------------------------------------------------------------------
    // Presence / removal
    // -----------------------------------------------------------------------
    bool contains(const char* key) const;
    bool remove(const char* key);
    bool clear();   ///< Erase entire namespace (factory reset)

private:
    StorageService() = default;
    ~StorageService();

    mutable Preferences prefs_;
    bool                ready_ = false;
};
