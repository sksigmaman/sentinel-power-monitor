/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: TelegramService.cpp
 */

#include "TelegramService.h"
#include "JsonService.h"
#include "NetworkService.h"
#include "KeyboardService.h"
#include "LoggerService.h"
#include "TimeService.h"
#include "Constants.h"

#include <ArduinoJson.h>

using namespace sentinel::constants;

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

TelegramService& TelegramService::instance() {
    static TelegramService inst;
    return inst;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void TelegramService::begin(const String& botToken, const String& chatId) {
    botToken_ = botToken;
    chatId_   = chatId;
    LOGI("TelegramService: ready (chatId=%s)", chatId_.c_str());
}

// ---------------------------------------------------------------------------
// Send message
// ---------------------------------------------------------------------------

bool TelegramService::sendMessage(const String& text, bool withKeyboard) {
    if (!isReady()) {
        LOGW("TelegramService: not ready (no credentials)");
        return false;
    }
    if (!NetworkService::instance().isOnline()) {
        LOGW("TelegramService: offline, cannot send");
        return false;
    }

    // Respect rate-limit backoff
    if (millis() < nextSendAllowMs_) {
        LOGW("TelegramService: rate-limited, skipping send");
        return false;
    }

    String body;
    if (withKeyboard) {
        // Build keyboard payload via KeyboardService
        body = KeyboardService::instance().buildSendMessageWithKeyboard(chatId_, text);
    } else {
        body = JsonService::instance().buildSendMessage(chatId_, text, "HTML");
    }

    String response;
    const bool ok = httpPost("/sendMessage", body, response);

    if (ok) {
        backoffMs_ = 1000;  // reset backoff on success
        LOGI("TelegramService: message sent successfully");
    } else {
        // Check for 429 Too Many Requests
        JsonDocument doc;
        if (deserializeJson(doc, response) == DeserializationError::Ok) {
            const uint32_t retryAfter = doc["parameters"]["retry_after"] | 0u;
            if (retryAfter > 0) {
                nextSendAllowMs_ = millis() + (retryAfter * 1000);
                LOGW("TelegramService: rate limited, retry after %u s", retryAfter);
                return false;
            }
        }
        // Exponential backoff
        backoffMs_ = min(backoffMs_ * 2, kMaxBackoffMs);
        LOGW("TelegramService: send failed, next backoff=%u ms", backoffMs_);
    }

    return ok;
}

// ---------------------------------------------------------------------------
// Update (polling)
// ---------------------------------------------------------------------------

void TelegramService::update() {
    if (!isReady() || !NetworkService::instance().isOnline()) return;

    // Do not poll until time is synced, so we can accurately ignore offline commands
    if (!TimeService::instance().isSynced()) return;

    if ((millis() - lastPollMs_) < kTelegramPollMs) return;
    lastPollMs_ = millis();

    poll();
}

void TelegramService::poll() {
    const String body = JsonService::instance().buildGetUpdates(nextOffset_);

    String response;
    if (!httpPost("/getUpdates", body, response)) {
        return;  // network error, try next tick
    }

    // Parse and dispatch
    String  chatId;
    String  text;
    int64_t updateId = 0;
    int64_t msgDate  = 0;

    if (JsonService::instance().parseFirstUpdate(response, chatId, text, updateId, msgDate)) {
        nextOffset_ = updateId + 1;  // acknowledge this update

        // Security: only process messages from the authorised chat ID
        if (chatId != chatId_) {
            LOGW("TelegramService: ignored message from unknown chatId=%s", chatId.c_str());
            return;
        }

        // Ignore stale messages (sent while device was offline).
        // Exception — always answered regardless of age:
        //   • /start — re-attaches the keyboard so user can interact again
        // All other commands (including Status) are ignored if > 10s old.
        const bool isExempt = (text == "/start");
        if (!isExempt && TimeService::instance().isSynced() && msgDate > 0) {
            const time_t now = TimeService::instance().unixTimestamp();
            if (now > msgDate && (now - msgDate) > 10) {
                LOGI("TelegramService: ignored stale offline command '%s' (age %lld s)", text.c_str(), (now - msgDate));
                return;
            }
        }

        LOGI("TelegramService: received command='%s'", text.c_str());
        if (commandHandler_) {
            commandHandler_(chatId, text);
        }
    }
}

// ---------------------------------------------------------------------------
// HTTP POST helper
// ---------------------------------------------------------------------------

bool TelegramService::httpPost(const String& endpoint, const String& body, String& responseOut) {
    WiFiClientSecure client;
    client.setInsecure();  // Telegram uses valid CA; setInsecure avoids cert bundle

    HTTPClient http;
    const String url = buildUrl(endpoint);

    if (!http.begin(client, url)) {
        LOGE("TelegramService: http.begin() failed for %s", endpoint.c_str());
        return false;
    }

    http.setTimeout(kTelegramTimeoutMs);
    http.addHeader("Content-Type", "application/json");

    const int httpCode = http.POST(body);
    responseOut        = http.getString();
    http.end();

    if (httpCode <= 0) {
        LOGE("TelegramService: POST %s failed, code=%d", endpoint.c_str(), httpCode);
        return false;
    }

    if (httpCode == 429) {
        LOGW("TelegramService: 429 Too Many Requests");
        return false;
    }

    if (httpCode < 200 || httpCode >= 300) {
        LOGW("TelegramService: POST %s returned HTTP %d", endpoint.c_str(), httpCode);
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// URL builder
// ---------------------------------------------------------------------------

String TelegramService::buildUrl(const String& method) const {
    String url = "https://";
    url += kTelegramApiHost;
    url += "/bot";
    url += botToken_;
    url += method;
    return url;
}
