/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: TelegramService.h
 * Brief: Direct HTTPS client for the Telegram Bot API.
 *        Handles sendMessage, getUpdates, polling, retry with backoff.
 */

#pragma once

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <functional>

using CommandHandler = std::function<void(const String& chatId, const String& text)>;

/**
 * @class TelegramService
 * @brief Singleton Telegram Bot API client.
 *
 * Responsibilities:
 *  - Send messages to the configured chat via HTTPS POST.
 *  - Poll getUpdates every kTelegramPollMs milliseconds.
 *  - Route received messages to a registered CommandHandler.
 *  - Retry failed sends with exponential backoff (capped at 60 s).
 *  - Respect Telegram 429 retry_after.
 *  - Validate incoming chat_id against the stored chat_id (security).
 */
class TelegramService {
public:
    static TelegramService& instance();

    TelegramService(const TelegramService&)            = delete;
    TelegramService& operator=(const TelegramService&) = delete;

    /**
     * @brief Provide credentials. Must be called before any send/poll.
     * @param botToken  Telegram Bot Token.
     * @param chatId    Authorised chat ID.
     */
    void begin(const String& botToken, const String& chatId);

    /**
     * @brief Send a plain text message. Fire-and-forget (queued internally).
     *        Returns true if the HTTP POST succeeded (2xx response).
     */
    bool sendMessage(const String& text, bool withKeyboard = false);

    /**
     * @brief Poll for new updates. Call every loop().
     */
    void update();

    /** @brief Register a handler for inbound commands/messages. */
    void onCommand(CommandHandler handler) { commandHandler_ = handler; }

    /** @brief True if bot token and chat ID have been provided. */
    bool isReady() const { return !botToken_.isEmpty() && !chatId_.isEmpty(); }

private:
    TelegramService() = default;
    ~TelegramService() = default;

    bool     httpPost(const String& endpoint, const String& body, String& responseOut);
    void     poll();
    String   buildUrl(const String& method) const;

    String         botToken_;
    String         chatId_;
    int64_t        nextOffset_      = 0;
    uint32_t       lastPollMs_      = 0;
    uint32_t       backoffMs_       = 1000;    // starts at 1 s, doubles on error
    uint32_t       nextSendAllowMs_ = 0;       // for 429 rate limiting

    CommandHandler commandHandler_;

    static constexpr uint32_t kMaxBackoffMs = 60000;
};
