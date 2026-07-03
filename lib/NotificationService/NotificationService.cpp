/*
 * Project:    Sentinel Power Monitor
 * Created By: Mr. Sunil Kumar S.
 * LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
 * Year:       2026
 *
 * File: NotificationService.cpp
 */

#include "NotificationService.h"
#include "TelegramService.h"
#include "NetworkService.h"
#include "LoggerService.h"
#include "TimeService.h"
#include "Constants.h"

#include <cstring>

using namespace sentinel::constants;

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

NotificationService& NotificationService::instance() {
    static NotificationService inst;
    return inst;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void NotificationService::begin() {
    for (auto& m : queue_) {
        m = sentinel::QueuedMessage{};
    }
    head_  = 0;
    tail_  = 0;
    count_ = 0;
    LOGI("NotificationService: queue capacity=%u × %u bytes", kQueueMaxMessages, kQueueMaxMsgBytes);
}

// ---------------------------------------------------------------------------
// Enqueue
// ---------------------------------------------------------------------------

void NotificationService::enqueue(const String& text, bool withKeyboard) {
    if (count_ >= kQueueMaxMessages) {
        // Drop the oldest message to make room (ring buffer)
        LOGW("NotificationService: queue full, dropping oldest message");
        tail_ = (tail_ + 1) % kQueueMaxMessages;
        --count_;
    }

    sentinel::QueuedMessage& slot = queue_[head_];
    strncpy(slot.text, text.c_str(), kQueueMaxMsgBytes - 1);
    slot.text[kQueueMaxMsgBytes - 1] = '\0';
    slot.withKeyboard = withKeyboard;
    slot.used         = true;

    head_ = (head_ + 1) % kQueueMaxMessages;
    ++count_;

    LOGD("NotificationService: enqueued (pending=%u)", count_);
}

// ---------------------------------------------------------------------------
// Drain
// ---------------------------------------------------------------------------

void NotificationService::drain() {
    if (count_ == 0) return;
    if (!NetworkService::instance().isOnline()) return;

    // Do not send ANY messages until time is synced!
    if (!TimeService::instance().isSynced()) return;

    sentinel::QueuedMessage& msg = queue_[tail_];
    if (!msg.used) {
        // Slot is empty – advance anyway to self-heal
        tail_  = (tail_ + 1) % kQueueMaxMessages;
        if (count_ > 0) --count_;
        return;
    }

    const bool sent = TelegramService::instance().sendMessage(
        String(msg.text), msg.withKeyboard);

    if (sent) {
        msg = sentinel::QueuedMessage{};  // clear slot
        tail_  = (tail_ + 1) % kQueueMaxMessages;
        --count_;
        LOGD("NotificationService: message sent (remaining=%u)", count_);
    }
    // If send failed, leave it in queue for next drain() call
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

uint8_t NotificationService::pending() const {
    return count_;
}
