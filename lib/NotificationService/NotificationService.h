/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: NotificationService.h
 * Brief: Fixed-size message queue that drains to Telegram when online.
 *        High-priority messages (power events) are queued even while offline.
 */

#pragma once

#include <Arduino.h>
#include "Types.h"
#include "Constants.h"

/**
 * @class NotificationService
 * @brief Singleton outbound message queue for Telegram notifications.
 *
 * Enqueue a message from any service. When NetworkService reports online,
 * update() drains the queue to TelegramService one message per tick.
 *
 * Queue capacity: kQueueMaxMessages × kQueueMaxMsgBytes (see Constants.h).
 * If the queue is full, the oldest message is dropped (ring buffer semantics).
 */
class NotificationService {
public:
    static NotificationService& instance();

    NotificationService(const NotificationService&)            = delete;
    NotificationService& operator=(const NotificationService&) = delete;

    void begin();

    /**
     * @brief Add a message to the outbound queue.
     * @param text         Message text (max kQueueMaxMsgBytes chars).
     * @param withKeyboard If true, the keyboard is attached to this message.
     */
    void enqueue(const String& text, bool withKeyboard = false);

    /**
     * @brief Drain one queued message per call. Call every loop() via SchedulerService.
     */
    void drain();

    /** @brief Number of messages currently waiting in the queue. */
    uint8_t pending() const;

private:
    NotificationService() = default;
    ~NotificationService() = default;

    sentinel::QueuedMessage queue_[sentinel::constants::kQueueMaxMessages];
    uint8_t head_ = 0;  // next slot to write
    uint8_t tail_ = 0;  // next slot to read
    uint8_t count_ = 0;
};
