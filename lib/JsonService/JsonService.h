/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: JsonService.h
 * Brief: Sole module permitted to use ArduinoJson.
 *        Builds Telegram API payloads only. No deserialization of unknown data.
 */

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

/**
 * @class JsonService
 * @brief Singleton JSON factory for Telegram API payloads.
 *
 * Centralises all ArduinoJson usage so no other module needs the library.
 * Each method returns a self-contained JSON String ready for HTTP POST.
 */
class JsonService {
public:
    static JsonService& instance();

    JsonService(const JsonService&)            = delete;
    JsonService& operator=(const JsonService&) = delete;

    /**
     * @brief Serialize a plain sendMessage payload.
     * @param chatId   Telegram chat ID string.
     * @param text     Message text (supports HTML or Markdown).
     * @param parseMode "HTML" | "Markdown" | "" (none).
     * @return JSON string ready for POST.
     */
    String buildSendMessage(const String& chatId,
                            const String& text,
                            const String& parseMode = "") const;

    /**
     * @brief Serialize a sendMessage payload with a persistent Reply Keyboard.
     * @param chatId   Telegram chat ID.
     * @param text     Message text.
     * @param keyboard 2D array of button labels (rows × columns).
     * @return JSON string.
     */
    String buildSendMessageWithKeyboard(const String& chatId,
                                        const String& text,
                                        const JsonArray& keyboard) const;

    /**
     * @brief Serialize a getUpdates request payload.
     * @param offset Next update_id to fetch from.
     * @param timeout Long-poll seconds (0 = short poll).
     * @return JSON string.
     */
    String buildGetUpdates(int64_t offset, uint8_t timeout = 0) const;

    /**
     * @brief Extract the first update from a getUpdates response.
     * Fills outChatId, outText, outUpdateId. Returns false if no message.
     */
    bool parseFirstUpdate(const String& json,
                          String& outChatId,
                          String& outText,
                          int64_t& outUpdateId,
                          int64_t& outDate) const;

private:
    JsonService() = default;
    ~JsonService() = default;

    static constexpr size_t kDocSize = 4096;
};
