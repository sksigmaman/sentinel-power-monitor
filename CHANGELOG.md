# Changelog

All notable changes to **Sentinel Power Monitor** will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [v1.0.1] — 2026-07-11

### Fixed

- **Startup Telegram message not sent after power cut** (`src/Boot.cpp`)
  - **Root cause:** `g_startupSent` was set to `true` the moment WiFi first
    connected, before NTP had synced. When the router took longer to boot than
    the ESP32 (~2 minutes is typical), the device would connect to WiFi, set
    the flag, then lose WiFi again before NTP completed. On the next reconnect,
    `g_startupSent == true` caused the code to call `drain()` on an **empty**
    notification queue — the startup message was silently lost with no log
    entry.
  - **Fix:** The `startup-notif` scheduler task now monitors the WiFi state. If
    WiFi drops before NTP syncs, it resets `g_startupSent = false` and removes
    itself, so the next successful reconnect triggers a fresh enqueue attempt.
    An additional guard in the `else if (!g_startupAcked)` branch re-enqueues
    the startup message if the queue is empty and NTP is now synced.

- **WiFi retry spam — retrying every ~170 ms instead of every 5 s** (`lib/WiFiService/WiFiService.cpp`)
  - **Root cause:** After the initial 20-second connection timeout, the state
    machine transitioned to `RECONNECTING` but **did not reset `lastAttemptMs_`**.
    Because the retry condition checks `millis() - lastAttemptMs_ >= kWifiReconnectDelayMs`,
    and `lastAttemptMs_` was still set from the previous disconnection event,
    the condition evaluated to `true` on every scheduler tick (every 10 ms),
    causing `tryConnect()` to be called hundreds of times per second. This
    flooded the LittleFS log file with thousands of identical "retrying"
    entries and caused the log dump at the next boot to take ~18 seconds.
  - **Fix:** Added `lastAttemptMs_ = millis()` when entering `RECONNECTING`
    state, so the full retry delay is respected between attempts.

- **Stale `/start` command ignored after device comes back online** (`lib/TelegramService/TelegramService.cpp`)
  - **Root cause:** The 10-second stale-message filter correctly discards
    commands sent while the device was offline, but this also silently dropped
    a `/start` command the user may have sent to re-attach the keyboard.
  - **Fix:** `/start` is now exempt from the stale filter and is always
    processed, regardless of message age. All other commands (Status, Device
    Info, Restart, Help) remain subject to the 10-second window.

### Changed

- **WiFi reconnect interval increased from 5 s to 30 s** (`include/Constants.h`)
  - During a power cut, the router typically takes ~2 minutes to fully boot.
    The previous 5-second retry interval generated dozens of pointless retry
    attempts and log entries during that window. 30 seconds gives the router
    adequate time to recover and results in only 2–3 retry log lines instead
    of hundreds.

---

## [v1.0.0] — 2026-07-11

### Added

- Initial release of **Sentinel Power Monitor**.
- Non-blocking WiFi connection manager with automatic infinite reconnection.
- NTP time sync (IST UTC+5:30) with retry on failure.
- Telegram Bot integration: send startup, power-lost, and power-restored messages.
- On-demand commands via persistent Telegram reply keyboard:
  `📊 Status`, `ℹ Device Info`, `🔄 Restart`, `❓ Help`.
- First-boot provisioning wizard over Serial (WiFi SSID/password, bot token,
  chat ID, device ID) — no hardcoded credentials.
- Stale offline command filtering — commands sent while offline are discarded
  if older than 10 seconds.
- Owner-only command security — all commands are validated against the
  authorised chat ID.
- FIFO rolling file log on LittleFS (1 MB cap) — persists across reboots,
  dumps historical log to Serial on every boot.
- Cooperative scheduler — no RTOS, no `delay()` in the main loop.
- Periodic heap usage logging (every 60 s).
- Restart count tracking in NVS.
