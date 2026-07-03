<div align="center">
  <h1>Sentinel Power Monitor</h1>
  <p><strong>An ESP32-Based Remote Electricity Status Notification System via Telegram</strong></p>

  <!-- Badges -->
  <p>
    <img src="https://img.shields.io/badge/Platform-ESP32-blue?style=for-the-badge&logo=espressif" alt="ESP32" />
    <img src="https://img.shields.io/badge/Framework-Arduino-00979D?style=for-the-badge&logo=arduino" alt="Arduino Framework" />
    <img src="https://img.shields.io/badge/Bot-Telegram-2CA5E0?style=for-the-badge&logo=telegram" alt="Telegram Bot API" />
    <img src="https://img.shields.io/badge/IDE-PlatformIO-orange?style=for-the-badge&logo=platformio" alt="PlatformIO" />
    <img src="https://img.shields.io/badge/Contributions-Welcome-brightgreen?style=for-the-badge" alt="Contributions Welcome" />
    <a href="https://www.linkedin.com/in/sunilkumarsekar/"><img src="https://img.shields.io/badge/LinkedIn-Connect-0A66C2?style=for-the-badge&logo=linkedin" alt="LinkedIn" /></a>
  </p>
</div>

<br />

> **Sentinel Power Monitor** is a low-cost, zero-extra-hardware IoT solution designed to notify you of your home's electrical power status (restoration & availability) while you are away, using just an ESP32 and a standard USB adapter.

---

## Features at a Glance

| Capability | Description |
| :--- | :--- |
| **Automatic restoration alert** | Sends a Telegram message the moment the device successfully reboots — the strongest signal that mains power is back. |
| **On-demand status from your phone** | `Status`, `Device Info`, `Restart`, and `Help` — all via a persistent Telegram reply keyboard, no app install needed. |
| **Zero extra hardware** | One ESP32 + one USB adapter. No current sensors, no relays, no battery pack. |
| **First-boot provisioning wizard** | Configure WiFi, bot token, and chat ID over Serial — no hardcoded credentials, no reflashing to change WiFi. |
| **Self-healing networking** | WiFi reconnects forever (never reboots the device), Telegram calls back off exponentially and respect rate limits. |
| **Owner-only command handling** | Every incoming command is checked against your chat ID before it's processed; secrets are masked in logs. |
| **Clean singleton-service architecture** | Cooperative scheduling, no RTOS, no `delay()` — a design meant to be easy to read, extend, and fork. |

> **Curious why it's built this way** — and what it deliberately can't do? [Part 1](#part-1-project-record--abstract) covers the reasoning, [Part 3](#part-3-architecture--how-the-code-works) covers the code.

---

## Table of Contents

- [Features at a Glance](#features-at-a-glance)
- [Part 1: Project Record & Abstract](#part-1-project-record--abstract)
  - [Abstract](#abstract)
  - [1. Introduction](#1-introduction)
  - [2. Related Considerations and Constraints](#2-related-considerations-and-constraints)
  - [3. System Architecture](#3-system-architecture)
  - [4. Operational Capabilities](#4-operational-capabilities)
  - [5. Limitations and Threats to Validity](#5-limitations-and-threats-to-validity)
  - [6. Conclusion](#6-conclusion)
- [Part 2: Complete A-to-Z Setup Guide](#part-2-complete-a-to-z-setup-guide)
  - [What You Need Before Starting](#what-you-need-before-starting)
  - [Step 1: Create the Telegram Bot](#step-1-create-the-telegram-bot)
  - [Step 2: Project Folder Structure](#step-2-project-folder-structure)
  - [Step 3: Install PlatformIO](#step-3-install-platformio)
  - [Step 4: Configure `platformio.ini`](#step-4-configure-platformioini)
  - [Step 5: Connect and Build](#step-5-connect-and-build)
  - [Step 6: First Boot & Provisioning Wizard](#step-6-first-boot--provisioning-wizard)
  - [Step 7: Verify & Deploy](#step-7-verify--deploy)
- [Part 3: Architecture & How The Code Works](#part-3-architecture--how-the-code-works)
  - [System Flow & Architecture Diagram](#system-flow--architecture-diagram)
  - [The Core Pattern: Singleton Services](#the-core-pattern-singleton-services)
  - [Separation of Concerns](#separation-of-concerns)
  - [The Master Controller: `Boot.cpp`](#the-master-controller-bootcpp)
  - [The Engine: `SchedulerService`](#the-engine-schedulerservice)
  - [Callbacks (Event-Driven Wiring)](#callbacks-event-driven-wiring)
  - [The Message Pipeline: Producer → Queue → Consumer](#the-message-pipeline-producer--queue--consumer)
  - [Summary of Design Patterns](#summary-of-design-patterns)
- [Security Notes](#security-notes)
- [Potential Future Enhancements](#potential-future-enhancements)
- [Contributing](#contributing)
- [License](#license)
- [Author](#author)

---

## Part 1: Project Record & Abstract

### Abstract
This paper presents the design, implementation, and evaluation of **Sentinel Power Monitor**, a low-cost Internet of Things (IoT) solution built on the ESP32 microcontroller platform, intended to address a common domestic problem: the inability of residents to know the electrical power status of their home while away, in the absence of an Uninterruptible Power Supply (UPS).

Unlike conventional power-monitoring systems that rely on dedicated sensing circuitry (current transformers, voltage dividers, optocouplers) and battery-backed continuous operation, this system adopts a **minimal-hardware approach**: a single ESP32 board powered directly from the same mains-derived adapter that powers the user's home network, and relies entirely on software-level inference and a Telegram bot interface to communicate power status to the user. This paper documents the system's architecture, its operational capabilities, its inherent hardware-imposed limitations, and the reasoning behind key design trade-offs, with the aim of providing an honest and technically grounded account of what such a minimal system can and cannot achieve.

### 1. Introduction
#### 1.1 Background
Electrical power outages are a common occurrence in many residential areas, particularly in regions with unstable grid infrastructure. For a resident who is away from home (at work, traveling, or otherwise outside), an unplanned power outage creates a state of uncertainty. The user cannot know:
- Whether the electricity supply at home has failed.
- Whether previously failed electricity has been restored.
- How long any outage lasted.
- Whether the home is currently powered at all.

This uncertainty compounds when the home lacks battery backup infrastructure (i.e., no UPS), because in such cases every electrical device in the home (including any monitoring or communication equipment) is subject to the same outage and cannot report its own status during the outage itself.

#### 1.2 Motivation
The immediate consequence of this uncertainty is behavioral: users resort to indirect and unreliable means of determining home power status, such as telephoning family members, contacting neighbours, or simply waiting. This is inefficient and often produces stale or inaccurate information.

Additionally, several dependent systems are affected by power loss:
- Closed-circuit television (CCTV) surveillance systems
- Home WiFi/internet routers
- General home appliances & device charging infrastructure

A remote, automated method of learning about power status, even a partial one, has clear practical value.

#### 1.3 Problem Statement
Given the above, the core problem this work addresses is:
> **How can a resident, while physically away from home, obtain information about their home's electrical power status using minimal additional hardware, when the home has no UPS or battery backup, and the monitoring device itself shares the same unprotected power source as the rest of the home?**

### 2. Related Considerations and Constraints
#### 2.1 Why Conventional Power Monitoring Does Not Apply Directly
Typical IoT power-outage detectors separate the *sensing* circuit's power source from the *monitored* circuit. This is usually achieved via batteries or optocouplers.

In the system under study, **neither of these conditions holds**. The ESP32 is powered exclusively through a single micro-USB cable connected to a wall adapter drawing from the same mains circuit as the rest of the home, including the WiFi router. There is no separate sensing line and no backup power source.

#### 2.2 The Fundamental Hardware Limitation
This single design decision has one unavoidable consequence:
> **The moment mains power is lost, the ESP32 itself loses power and ceases all execution instantaneously. It cannot detect, timestamp, log, or transmit any information about the outage at the moment it occurs.**

#### 2.3 Design Philosophy: Minimalism Over Completeness
Given this constraint, the system was deliberately designed around what *is* achievable with adapter-only power, rather than attempting to simulate capabilities that the hardware cannot support.

### 3. System Architecture
#### 3.1 Hardware Composition
The complete hardware bill of materials is intentionally minimal:

| Component | Function |
| :--- | :--- |
| **ESP32 Dev Board** | Microcontroller running all application logic |
| **Micro-USB Cable** | Delivers power only, from adapter to ESP32 |
| **5V Power Adapter** | Converts AC mains to 5V DC for the ESP32 |

#### 3.2 Software Architecture Overview
The firmware is structured as a set of independent singleton services, orchestrated by a master boot routine, and driven by a lightweight cooperative task scheduler (no RTOS). Major functional groups:
1. **Foundation layer:** Logging, storage (NVS), diagnostics, scheduler.
2. **Network layer:** WiFi management, NTP-based time synchronization.
3. **Provisioning layer:** First-boot Serial-Monitor wizard.
4. **Telegram layer:** HTTPS client for Telegram Bot API, JSON payload builder, reply-keyboard.
5. **Application layer:** Message construction and outbound queue.

### 4. Operational Capabilities
#### 4.1 Automatic Notification: Power Restoration
The system's only fully automatic notification is sent when the device successfully completes its boot sequence. Since the device shares power with the home, a successful boot is a strong practical indicator that **mains power has been restored**.

#### 4.2 On-Demand Status Queries
The system exposes a Telegram-based interactive interface with four commands:
- **Status:** Live snapshot (online state, WiFi signal, IP, uptime, time).
- **Device Info:** Detailed hardware diagnostics (chip model, CPU freq, heap, reset reason).
- **Restart:** Remotely reboot the device (with Yes/No confirmation).
- **Help:** Lists available commands.

#### 4.3 Inferential Detection of Power Loss
Because the system cannot announce a power-loss event, the user relies on an **inferential method**: if the user presses any button (e.g., Status) and receives *no reply*, it is evidence that the home is currently without power.

#### 4.4 Capability Summary
| User Information Need | Mechanism | Directness |
| :--- | :--- | :--- |
| **Has electricity gone?** | No reply to a status request | *Inferred, indirect* |
| **Has electricity come back?** | Automatic startup message | *Direct, automatic* |
| **How long was the outage?** | Not currently supported | *Not available* |
| **Is my home currently powered?** | Reply/no-reply to Status request | *Inferred, indirect* |

### 5. Limitations and Threats to Validity
- **No True Power-Loss Detection:** The system cannot detect the instant of power loss.
- **Ambiguity of "No Reply":** No reply could also mean internet failure or a firmware crash.
- **False Positives:** A startup notification fires on *any* boot (e.g., WiFi reconnection crash), not just genuine outages.

### 6. Conclusion
**Sentinel Power Monitor** demonstrates that a single ESP32 board, powered by nothing more than a standard USB adapter, can provide meaningful remote visibility into a home's electrical power status via a Telegram bot interface.

<div align="right"><a href="#table-of-contents">Back to top</a></div>

---

## Part 2: Complete A-to-Z Setup Guide

### What You Need Before Starting
- **ESP32 development board** (target: `esp32dev`)
- **Micro-USB cable** (data + power capable)
- **USB wall adapter (5V)** for standalone powering
- **VS Code** with the **PlatformIO** extension
- A **Telegram account**
- Your **WiFi SSID** and **Password**

### Step 1: Create the Telegram Bot
1. Open Telegram, search for **@BotFather**.
2. Send `/newbot`. Give it a name and username.
3. BotFather replies with a **Bot Token** (e.g., `123456789:AAEx...`). **Save this.**
4. Search for **@userinfobot** in Telegram, start it to get your numeric **Chat ID**. **Save this too.**
5. Send your new bot a message (e.g., `/start`) to initiate the chat.

### Step 2: Project Folder Structure
Ensure your project follows this layout for PlatformIO:
```text
your-project/
├── platformio.ini
├── include/
│   └── AppCredentials.h, Constants.h, Types.h, Version.h
├── src/
│   └── main.cpp, Boot.h, Boot.cpp
└── lib/
    ├── LoggerService/
    ├── StorageService/
    ├── SystemService/
    └── ... (other services)
```

### Step 3: Install PlatformIO
1. Open **VS Code**.
2. Go to Extensions, search **PlatformIO IDE**, and click Install.
3. Open your project folder (`File → Open Folder`).

### Step 4: Configure `platformio.ini`
PlatformIO will automatically detect the configuration.
> **Note on COM Port:** To allow PlatformIO to auto-detect the port, ensure `upload_port` is commented out. If it fails, explicitly define your port (e.g., `COM3` or `/dev/ttyUSB0`).

Here's a minimal example reflecting the settings above:

```ini
; Sentinel Power Monitor
; Created By: Mr. Sunil Kumar S.
; LinkedIn:   https://www.linkedin.com/in/sunilkumarsekar/
; Year:       2026

[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
; upload_port = COM3   ; uncomment and set only if auto-detect fails
```

### Step 5: Connect and Build
1. Connect the ESP32 to your computer via USB.
2. Click the PlatformIO sidebar icon (alien head).
3. Under **PROJECT TASKS → esp32dev**, click **Build**.
4. Once successful, click **Upload**.
5. Click **Monitor** (baud rate `115200`).

### Step 6: First Boot & Provisioning Wizard
1. In the Serial Monitor, wait for the banner and **press any key** within ~15 seconds.
2. The interactive wizard will prompt you for your **WiFi SSID**, **Password**, **Bot Token**, and **Chat ID**.
3. It saves everything to NVS (flash) and reboots.

### Step 7: Verify & Deploy
1. Check your Telegram bot chat: you should receive a **startup message** within 60 seconds.
2. You will see a persistent reply keyboard: Status, Device Info, Restart, Help.
3. **Deploy:** Unplug from your PC, plug into a standard 5V wall adapter, and place it anywhere with WiFi coverage!

<div align="right"><a href="#table-of-contents">Back to top</a></div>

---

## Part 3: Architecture & How The Code Works

### System Flow & Architecture Diagram
<details open>
<summary><strong>Click to expand / collapse the full architecture diagram</strong></summary>

```mermaid
%%{init: {
  'theme': 'base',
  'themeVariables': {
    'primaryColor': '#ffffff',
    'primaryTextColor': '#000000',
    'primaryBorderColor': '#000000',
    'lineColor': '#4d4d4d',
    'secondaryColor': '#e6e6e6',
    'tertiaryColor': '#f5f5f5',
    'background': '#ffffff',
    'mainBkg': '#ffffff',
    'clusterBkg': '#f2f2f2',
    'clusterBorder': '#000000',
    'edgeLabelBackground': '#ffffff',
    'fontFamily': 'Segoe UI, Helvetica, Arial, sans-serif'
  }
}}%%
flowchart TD
    %% Grayscale classes
    classDef hardware fill:#F0F0F0,stroke:#000,color:#000
    classDef boot fill:#D9D9D9,stroke:#000,color:#000
    classDef mainloop fill:#BFBFBF,stroke:#000,color:#000
    classDef services fill:#A6A6A6,stroke:#000,color:#000
    classDef interaction fill:#8C8C8C,stroke:#000,color:#000
    classDef restart fill:#737373,stroke:#000,color:#FFF
    classDef provision fill:#595959,stroke:#000,color:#FFF
    classDef shared fill:#404040,stroke:#000,color:#FFF
    classDef patterns fill:#262626,stroke:#000,color:#FFF
    classDef security fill:#0D0D0D,stroke:#000,color:#FFF
    classDef error fill:#333333,stroke:#000,color:#FFF
    classDef limits fill:#666666,stroke:#000,color:#FFF
    classDef messages fill:#999999,stroke:#000,color:#000

    %% CENTER NODE
    CENTER["`**Sentinel Power Monitor**<br/>ESP32 + Telegram Bot`"]
    class CENTER boot

    %% BRANCH 1
    subgraph HW [BRANCH 1 - HARDWARE]
        direction TB
        H1(ESP32 board):::hardware
        H2(Micro-USB cable<br/>power only):::hardware
        H3(No sensors<br/>no battery<br/>no GPIO signal):::hardware
        H1 --> H2 --> H3
    end

    %% BRANCH 2
    subgraph BOOT [BRANCH 2 - BOOT SEQUENCE]
        direction TB
        B1["main.cpp → bootInit()"]:::boot
        B2[Logger → Storage → System → Scheduler]:::boot
        B3[Load credentials / run wizard]:::boot
        B4[NetworkService → WiFi connect]:::boot
        B5[WiFi ok → TimeService NTP sync]:::boot
        B6[NTP synced → send STARTUP MESSAGE]:::boot
        B7["TelegramService.begin() + handler"]:::boot
        B8[Register tasks:<br/>wifi-update 10ms<br/>time-update 1s<br/>telegram-poll 1s<br/>notif-drain 500ms<br/>heap-log 60s]:::boot
        B1 --> B2 --> B3 --> B4 --> B5 --> B6 --> B7 --> B8
    end

    %% BRANCH 3
    subgraph LOOP [BRANCH 3 - MAIN LOOP]
        direction TB
        L1["bootLoop()"]:::mainloop
        L2["SchedulerService.update()"]:::mainloop
        L3[Check all tasks<br/>run due ones]:::mainloop
        L4[Repeat forever]:::mainloop
        L1 --> L2 --> L3 --> L4 --> L1
    end

    %% BRANCH 4
    subgraph SVCS [BRANCH 4 - SERVICES]
        direction TB
        S_ORCH[(Boot.cpp Orchestrator)]:::services
        S1[LoggerService]:::services
        S2[StorageService]:::services
        S3[SystemService]:::services
        S4[SchedulerService]:::services
        S5[WiFiService]:::services
        S6[NetworkService]:::services
        S7[TimeService]:::services
        S8[ProvisioningService]:::services
        S9[JsonService]:::services
        S10[TelegramService]:::services
        S11[KeyboardService]:::services
        S12[StatusService]:::services
        S13[NotificationService]:::services
        S_ORCH --- S1 & S2 & S3 & S4 & S5 & S6 & S7 & S8 & S9 & S10 & S11 & S12 & S13
    end

    %% BRANCH 5
    subgraph INT [BRANCH 5 - USER INTERACTION]
        direction TB
        I1[User taps Telegram button]:::interaction
        I2[TelegramService polls getUpdates]:::interaction
        I3[Validate chat ID]:::interaction
        I4["Route to handleCommand()"]:::interaction
        I5[StatusService builds message<br/>from System, Network, Time]:::interaction
        I6[Message → NotificationService.enqueue]:::interaction
        I7[1-slot queue]:::interaction
        I8["Every 500ms: drain() sends it"]:::interaction
        I9["TelegramService.sendMessage()<br/>HTTPS POST → user sees reply"]:::interaction
        I1 --> I2 --> I3 --> I4 --> I5 --> I6 --> I7 --> I8 --> I9
    end

    %% BRANCH 6
    subgraph REST [BRANCH 6 - RESTART CONFIRMATION]
        direction TB
        R1[User taps Restart]:::restart
        R2[Boot.cpp sets awaitRestart flag]:::restart
        R3[Send Yes/No keyboard<br/>directly via HTTPClient]:::restart
        R4{User choice?}:::restart
        R5["Yes → 'Restarting...' → esp_restart()"]:::restart
        R6[No → flag cleared<br/>'Restart cancelled' queued normally]:::restart
        R1 --> R2 --> R3 --> R4
        R4 -->|Yes| R5
        R4 -->|No| R6
    end

    %% BRANCH 7
    subgraph PROV [BRANCH 7 - PROVISIONING WIZARD]
        direction TB
        P1[Check NVS for saved credentials]:::provision
        P2{Found?}:::provision
        P3[Skip wizard, load credentials]:::provision
        P4[Wait 15s for Serial keypress]:::provision
        P5{Keypress?}:::provision
        P6[Continue without network]:::provision
        P7[Prompt WiFi SSID → password<br/>→ Bot Token → Chat ID]:::provision
        P8[Validate, mask secrets, print summary]:::provision
        P9[Save to NVS → reboot]:::provision
        P1 --> P2
        P2 -->|Yes| P3
        P2 -->|No| P4
        P4 --> P5
        P5 -->|No| P6
        P5 -->|Yes| P7 --> P8 --> P9
    end

    %% BRANCH 8
    subgraph SHARED [BRANCH 8 - SHARED DATA/CONFIG]
        direction LR
        SH1[Types.h<br/>shared enums + struct]:::shared
        SH2[Constants.h<br/>timings, pins, NVS keys]:::shared
        SH3[AppCredentials.h<br/>WiFi & Telegram creds]:::shared
        SH4[Version.h<br/>firmware version]:::shared
        SH1 --- SH2 --- SH3 --- SH4
    end

    %% BRANCH 9
    subgraph PAT [BRANCH 9 - DESIGN PATTERNS]
        direction TB
        PT1["Singleton pattern<br/>(every service)"]:::patterns
        PT2["Separation of Concerns<br/>(one folder = one job)"]:::patterns
        PT3["Composition Root<br/>(Boot.cpp orchestrator)"]:::patterns
        PT4["Cooperative non-blocking scheduling<br/>(no RTOS, no delay)"]:::patterns
        PT5["Observer/Callback<br/>(onConnected, onCommand)"]:::patterns
        PT6["Producer-Consumer queue<br/>(1-slot overwrite)"]:::patterns
        PT1 --- PT2 --- PT3 --- PT4 --- PT5 --- PT6
    end

    %% BRANCH 10
    subgraph SEC [BRANCH 10 - SECURITY MEASURES]
        direction TB
        SE1[Validate incoming chat_id<br/>ignore strangers]:::security
        SE2["LoggerService.mask()<br/>redact tokens/passwords"]:::security
        SE3[ProvisioningService validates<br/>token & chat ID format]:::security
        SE4["TLS for HTTPS calls<br/>(cert validation disabled)"]:::security
        SE1 --- SE2 --- SE3 --- SE4
    end

    %% BRANCH 11
    subgraph ERR [BRANCH 11 - ERROR HANDLING & RETRY]
        direction TB
        E1[WiFi: reconnect forever, never reboot]:::error
        E2[Telegram: exponential backoff<br/>1s → double → cap 60s]:::error
        E3[Telegram: respect 429 rate-limit retry_after]:::error
        E4[TimeService: retry NTP every 30s]:::error
        E5[NotificationService: 1-slot queue<br/>new message overwrites unsent one]:::error
        E1 --- E2 --- E3 --- E4 --- E5
    end

    %% BRANCH 12
    subgraph LIM [BRANCH 12 - KNOWN LIMITATIONS]
        direction TB
        LM1["Cannot detect power LOSS<br/>(no battery, dies instantly)"]:::limits
        LM2[CAN detect power RESTORATION<br/>via startup message]:::limits
        LM3[Power loss inferred only<br/>no reply = likely no power]:::limits
        LM4[Offline commands processed 1/s<br/>dropped if older than 10s]:::limits
        LM5[Startup message can come from<br/>WiFi drop, crash, manual restart]:::limits
        LM1 --- LM2 --- LM3 --- LM4 --- LM5
    end

    %% BRANCH 13
    subgraph MSG [BRANCH 13 - ACTUAL MESSAGE OUTPUTS]
        direction TB
        M1["Startup: Sentinel Power Monitor<br/>Electricity Available<br/>Wi-Fi Connected"]:::messages
        M2["Status: online, RSSI, IP, uptime, date/time"]:::messages
        M3["Device Info: chip model, flash, SDK, MAC,<br/>heap, restart count, reset reason"]:::messages
        M4["Help: lists commands<br/>(no power-loss alerts, only restoration)"]:::messages
        M1 --- M2 --- M3 --- M4
    end

    %% ===== VERTICAL CHAIN - mobile-friendly order =====
    CENTER --> HW --> BOOT --> LOOP --> SVCS --> INT --> REST --> PROV --> SHARED --> PAT --> SEC --> ERR --> LIM --> MSG
```

</details>

### The Core Pattern: Singleton Services
Every piece of functionality (WiFi, Telegram, Logging, Storage) is a **class** with only **one instance** (Singleton pattern).
```cpp
WiFiService::instance().begin(ssid, password);
```
This avoids the overhead of creating/destroying multiple instances and simplifies passing references.

### Separation of Concerns
Each folder in `lib/` does exactly one job:
- `StatusService` builds text messages.
- `TelegramService` talks to the Telegram API.
This makes files small, understandable, and independent.

### The Master Controller: `Boot.cpp`
`Boot.cpp` acts as the **Orchestrator**:
- **`bootInit()`** starts every service in strict dependency order.
- **`bootLoop()`** runs forever doing just one thing: `SchedulerService::instance().update();`

### The Engine: `SchedulerService`
The firmware uses **cooperative multitasking** instead of an RTOS. Tasks are registered to run every X milliseconds. Nothing ever calls `delay()`, ensuring the main loop never blocks.

### Callbacks (Event-Driven Wiring)
Services react to one another using **callback functions**:
```cpp
WiFiService::instance().onConnected([]() {
    TimeService::instance().begin();
});
```

### The Message Pipeline: Producer → Queue → Consumer
Sending a Telegram message happens asynchronously via a queue:
1. `NotificationService::enqueue(text)` is called.
2. The message sits in a queue.
3. Every 500ms, `NotificationService::drain()` runs via the scheduler and dispatches it via Telegram.

### Summary of Design Patterns
This project uses a **layered, event-driven, singleton-service architecture with cooperative scheduling**: a lightweight design style perfect for ESP32 firmware requiring modularity and clarity without RTOS overhead.

<div align="right"><a href="#table-of-contents">Back to top</a></div>

---

## Security Notes

This project handles real credentials (WiFi password, Telegram Bot Token, Chat ID). A few things worth keeping in mind before pushing to a public repo:

- **Never commit real secrets.** If `AppCredentials.h` ends up holding hardcoded WiFi or Telegram values, add it to `.gitignore` and commit an `AppCredentials.h.example` with placeholder values instead.
- **Keep the chat ID check.** The firmware validates the incoming `chat_id` against the configured owner ID (Branch 10 in the diagram above). That's what stops a stranger who finds your bot from issuing commands to it.
- **TLS cert validation is currently disabled** on the HTTPS client (also Branch 10). Traffic is encrypted but not authenticated against a pinned certificate, which is a reasonable trade-off on a microcontroller but worth knowing if your threat model needs more than that.
- The Provisioning Wizard already masks secrets when printing the config summary to Serial, which is worth preserving if you add logging elsewhere.

## Potential Future Enhancements

The current design deliberately trades completeness for hardware simplicity (see the Design Philosophy above). A few ideas that extend it without abandoning that spirit:

- **Small supercapacitor, not a full UPS:** a cheap 1-2F supercap across the 5V rail could keep the ESP32 alive for a few hundred extra milliseconds after mains loss, just long enough to fire one last "power lost" message before it dies.
- **RTC + NVS "last seen" timestamp:** periodically writing a heartbeat timestamp to NVS would let the startup message estimate outage duration ("last seen ~47 minutes ago"), closing the gap in the capability table above.
- **A second, battery-powered node:** an ESP-NOW companion elsewhere in the house could notice the main device going silent and relay that as a genuine power-loss alert, at the cost of needing its own power source.
- **Historical logging:** pushing boot/uptime events to a lightweight external endpoint would let you graph outage frequency over time instead of digging through Telegram chat history.

## Contributing

Contributions, bug reports, and feature ideas are welcome.

1. Fork the repository.
2. Create a feature branch (`git checkout -b feature/your-idea`).
3. Commit your changes with a clear message.
4. Open a Pull Request describing what changed and why.

For larger changes, opening an issue first to discuss the approach is appreciated.

## License

© 2026 Sunil Kumar S. This project doesn't specify a license yet, which by default means all rights are reserved and others don't have explicit permission to reuse the code. If you'd like it to be open source, [choosealicense.com](https://choosealicense.com/) can help you pick one. MIT is a common permissive choice for hobby/IoT firmware like this.

## Author

<div align="center">

### Sunil Kumar S.

**Embedded Systems Developer&nbsp;·&nbsp;IoT & Firmware Engineering**

[![LinkedIn](https://img.shields.io/badge/LinkedIn-Sunil_Kumar_S.-0A66C2?style=for-the-badge&logo=linkedin&logoColor=white)](https://www.linkedin.com/in/sunilkumarsekar/)

<br/>

<img src="https://img.shields.io/badge/ESP32-000000?style=flat-square&logo=espressif&logoColor=white" alt="ESP32" />
<img src="https://img.shields.io/badge/C%2B%2B-00599C?style=flat-square&logo=cplusplus&logoColor=white" alt="C++" />
<img src="https://img.shields.io/badge/Arduino-00979D?style=flat-square&logo=arduino&logoColor=white" alt="Arduino Framework" />
<img src="https://img.shields.io/badge/PlatformIO-FF7F00?style=flat-square&logo=platformio&logoColor=white" alt="PlatformIO" />
<img src="https://img.shields.io/badge/Telegram_Bot_API-26A5E4?style=flat-square&logo=telegram&logoColor=white" alt="Telegram Bot API" />
<img src="https://img.shields.io/badge/IoT-4B4B4B?style=flat-square" alt="IoT" />

<br/><br/>

<table>
<tr>
<td align="center" width="600">
<sub>
Designed and built <strong>Sentinel Power Monitor</strong> end-to-end — hardware selection, firmware architecture, and documentation — as a case study in solving a real household problem with deliberately minimal hardware, and in documenting the trade-offs honestly rather than hiding them.
</sub>
</td>
</tr>
</table>

</div>

<br />

<div align="center">
  <sub>Sentinel Power Monitor — designed and maintained by <a href="https://www.linkedin.com/in/sunilkumarsekar/">Sunil Kumar S.</a></sub>
</div>
