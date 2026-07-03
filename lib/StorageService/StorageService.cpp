/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: StorageService.cpp
 */

#include "StorageService.h"
#include "LoggerService.h"
#include "Constants.h"

using namespace sentinel::constants::nvs;

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

StorageService& StorageService::instance() {
    static StorageService inst;
    return inst;
}

StorageService::~StorageService() {
    if (ready_) prefs_.end();
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

bool StorageService::begin() {
    if (ready_) return true;
    ready_ = prefs_.begin(kNamespace, false);  // false = read-write
    if (!ready_) {
        LOGE("StorageService: failed to open NVS namespace '%s'", kNamespace);
    } else {
        LOGI("StorageService: NVS ready");
    }
    return ready_;
}

// ---------------------------------------------------------------------------
// String
// ---------------------------------------------------------------------------

String StorageService::getString(const char* key, const String& defaultVal) const {
    if (!ready_) return defaultVal;
    return prefs_.getString(key, defaultVal);
}

bool StorageService::putString(const char* key, const String& value) {
    if (!ready_) return false;
    // Preferences returns bytes written; 0 can be valid for empty strings
    prefs_.putString(key, value);
    return true;
}

// ---------------------------------------------------------------------------
// UInt
// ---------------------------------------------------------------------------

uint32_t StorageService::getUInt(const char* key, uint32_t defaultVal) const {
    if (!ready_) return defaultVal;
    return prefs_.getUInt(key, defaultVal);
}

bool StorageService::putUInt(const char* key, uint32_t value) {
    if (!ready_) return false;
    prefs_.putUInt(key, value);
    return true;
}

// ---------------------------------------------------------------------------
// Presence / removal
// ---------------------------------------------------------------------------

bool StorageService::contains(const char* key) const {
    if (!ready_) return false;
    return prefs_.isKey(key);
}

bool StorageService::remove(const char* key) {
    if (!ready_) return false;
    return prefs_.remove(key);
}

bool StorageService::clear() {
    if (!ready_) return false;
    return prefs_.clear();
}
