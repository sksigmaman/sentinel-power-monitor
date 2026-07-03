/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: KeyboardService.h
 * Brief: Builds the persistent Telegram Reply Keyboard layout and
 *        serializes it alongside a message payload.
 */

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

/**
 * @class KeyboardService
 * @brief Constructs the Sentinel persistent Reply Keyboard.
 *
 * Layout:
 *   [ 📊 Status ]  [ ℹ Device Info ]
 *   [ 🔄 Restart ] [ ❓ Help       ]
 */
class KeyboardService {
public:
    static KeyboardService& instance();

    KeyboardService(const KeyboardService&)            = delete;
    KeyboardService& operator=(const KeyboardService&) = delete;

    /**
     * @brief Build a sendMessage JSON payload that includes the Reply Keyboard.
     * @param chatId  Destination chat ID.
     * @param text    Message text to accompany the keyboard.
     * @return Complete JSON body for POST /sendMessage.
     */
    String buildSendMessageWithKeyboard(const String& chatId, const String& text) const;

    // Button label constants (used by command router for matching)
    static constexpr const char* kBtnStatus     = "\xF0\x9F\x93\x8A Status";       // 📊 Status
    static constexpr const char* kBtnDeviceInfo = "\xE2\x84\xB9 Device Info";      // ℹ Device Info
    static constexpr const char* kBtnRestart    = "\xF0\x9F\x94\x84 Restart";      // 🔄 Restart
    static constexpr const char* kBtnHelp       = "\xE2\x9D\x93 Help";             // ❓ Help
    static constexpr const char* kBtnYes        = "\xE2\x9C\x85 Yes";              // ✅ Yes
    static constexpr const char* kBtnNo         = "\xE2\x9D\x8C No";               // ❌ No

private:
    KeyboardService() = default;
    ~KeyboardService() = default;
};
