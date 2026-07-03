/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: KeyboardService.cpp
 */

#include "KeyboardService.h"
#include "LoggerService.h"

// ---------------------------------------------------------------------------
// Out-of-class definitions for static constexpr members (required by linker)
// ---------------------------------------------------------------------------
constexpr const char* KeyboardService::kBtnStatus;
constexpr const char* KeyboardService::kBtnDeviceInfo;
constexpr const char* KeyboardService::kBtnRestart;
constexpr const char* KeyboardService::kBtnHelp;
constexpr const char* KeyboardService::kBtnYes;
constexpr const char* KeyboardService::kBtnNo;

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

KeyboardService& KeyboardService::instance() {
    static KeyboardService inst;
    return inst;
}

// ---------------------------------------------------------------------------
// Build keyboard payload
// ---------------------------------------------------------------------------

String KeyboardService::buildSendMessageWithKeyboard(const String& chatId,
                                                      const String& text) const {
    JsonDocument doc;
    doc["chat_id"]    = chatId;
    doc["text"]       = text;
    doc["parse_mode"] = "HTML";

    // Build the 2×2 keyboard layout
    JsonObject markup = doc["reply_markup"].to<JsonObject>();
    markup["resize_keyboard"]   = true;
    markup["one_time_keyboard"] = false;
    markup["persistent"]        = true;
    markup["input_field_placeholder"] = "\xF0\x9F\x91\x87 Please use the buttons below";

    JsonArray rows = markup["keyboard"].to<JsonArray>();

    // Row 1: Status | Device Info
    JsonArray row1 = rows.add<JsonArray>();
    row1.add(kBtnStatus);
    row1.add(kBtnDeviceInfo);

    // Row 2: Restart | Help
    JsonArray row2 = rows.add<JsonArray>();
    row2.add(kBtnRestart);
    row2.add(kBtnHelp);

    String out;
    serializeJson(doc, out);
    return out;
}
