/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: JsonService.cpp
 */

#include "JsonService.h"
#include "LoggerService.h"

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

JsonService& JsonService::instance() {
    static JsonService inst;
    return inst;
}

// ---------------------------------------------------------------------------
// sendMessage
// ---------------------------------------------------------------------------

String JsonService::buildSendMessage(const String& chatId,
                                     const String& text,
                                     const String& parseMode) const {
    JsonDocument doc;
    doc["chat_id"] = chatId;
    doc["text"]    = text;
    if (!parseMode.isEmpty()) {
        doc["parse_mode"] = parseMode;
    }
    String out;
    serializeJson(doc, out);
    return out;
}

// ---------------------------------------------------------------------------
// sendMessage with Reply Keyboard
// ---------------------------------------------------------------------------

String JsonService::buildSendMessageWithKeyboard(const String& chatId,
                                                  const String& text,
                                                  const JsonArray& keyboard) const {
    JsonDocument doc;
    doc["chat_id"] = chatId;
    doc["text"]    = text;

    JsonObject replyMarkup = doc["reply_markup"].to<JsonObject>();
    replyMarkup["keyboard"]          = keyboard;
    replyMarkup["resize_keyboard"]   = true;
    replyMarkup["one_time_keyboard"] = false;
    replyMarkup["persistent"]        = true;

    String out;
    serializeJson(doc, out);
    return out;
}

// ---------------------------------------------------------------------------
// getUpdates
// ---------------------------------------------------------------------------

String JsonService::buildGetUpdates(int64_t offset, uint8_t timeout) const {
    JsonDocument doc;
    if (offset > 0)  doc["offset"]  = offset;
    if (timeout > 0) doc["timeout"] = timeout;
    String out;
    serializeJson(doc, out);
    return out;
}

// ---------------------------------------------------------------------------
// Parse getUpdates response
// ---------------------------------------------------------------------------

bool JsonService::parseFirstUpdate(const String& json,
                                   String& outChatId,
                                   String& outText,
                                   int64_t& outUpdateId,
                                   int64_t& outDate) const {
    JsonDocument doc;
    const DeserializationError err = deserializeJson(doc, json);
    if (err) {
        LOGE("JsonService: parse error: %s", err.c_str());
        return false;
    }

    if (!doc["ok"].as<bool>()) {
        return false;
    }

    JsonArray results = doc["result"].as<JsonArray>();
    if (results.isNull() || results.size() == 0) {
        return false;
    }

    JsonObject update = results[0].as<JsonObject>();
    outUpdateId = update["update_id"] | static_cast<int64_t>(0);

    // Support both regular messages and callback queries
    JsonObject message = update["message"].as<JsonObject>();
    if (message.isNull()) {
        message = update["callback_query"]["message"].as<JsonObject>();
    }

    if (message.isNull()) {
        outText = "";
        return true; // Return true so offset is incremented and we don't get stuck in a death loop
    }

    outChatId = message["chat"]["id"].as<String>();
    outText   = message["text"] | "";
    outDate   = message["date"] | static_cast<int64_t>(0);

    // Also handle callback_query data
    if (outText.isEmpty()) {
        outText = update["callback_query"]["data"] | "";
    }

    return true; // Always return true if updateId was found, so we skip it if it's empty
}
