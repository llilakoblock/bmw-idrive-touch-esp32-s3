# BMW iDrive Touch Controller for Android Head Unit

> ESP32-S3 adapter that connects BMW G-series iDrive Touch controller (ZBE4) to Android head units via USB HID.

**Status: FULLY WORKING** - All features operational including touchpad!

**Disclaimer**: This project is not affiliated with or endorsed by BMW. It is intended for educational and experimental purposes only. Use in a vehicle is at your own risk.

## Table of Contents

- [Overview](#overview)
- [Why This Project?](#why-this-project)
- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Wiring Diagram](#wiring-diagram)
- [Button Mapping](#button-mapping)
- [Custom Button Remapping (Android)](#custom-button-remapping-android)
- [Software Architecture](#software-architecture)
- [OTA Firmware Updates](#ota-firmware-updates)
- [CAN Bus Protocol](#can-bus-protocol)
- [Building and Flashing](#building-and-flashing)
- [Configuration](#configuration)
- [Troubleshooting](#troubleshooting)
- [License](#license)

## Overview

This project enables you to use a genuine BMW iDrive Touch controller (G-series ZBE4) with aftermarket Android head units. The ESP32-S3 microcontroller acts as a bridge:

1. **Reads** button presses, rotary encoder rotation, joystick movement, and touchpad gestures from the iDrive controller via CAN bus
2. **Converts** these inputs into standard USB HID events (keyboard, mouse, and media controls)
3. **Sends** the HID events to the Android head unit via USB

The result is seamless integration of the premium BMW iDrive interface with your Android head unit.

## Why This Project?

### The Problem

When replacing a BMW's original head unit with an Android unit, the iDrive controller becomes useless because:
- Android units don't speak BMW's proprietary CAN bus protocol
- There's no direct way to connect the iDrive to Android
- Losing the iDrive means losing an ergonomic, eyes-on-road control interface

### The Solution

This adapter solves the problem by:
- Decoding BMW's CAN bus protocol for the iDrive controller
- Translating all inputs to standard USB HID that Android understands
- Presenting itself as a composite USB device (keyboard + mouse + media controls)
- Maintaining the original feel and functionality of the iDrive controller

### Benefits

- **Native Android Integration**: Works with any Android app, not just specific head unit software
- **Full Control**: Volume, media playback, navigation, and cursor control
- **Premium Feel**: Keep using the high-quality BMW hardware
- **No Root Required**: Standard USB HID works out of the box on Android

## Features

### What Works

| Input | Function | Status |
|-------|----------|--------|
| **Rotary Encoder** | Volume Up/Down | ✅ Working |
| **Rotary Push** | Enter/Select | ✅ Working |
| **MENU Button** | Android Menu | ✅ Working |
| **BACK Button** | Android Back | ✅ Working |
| **NAV Button** | Android Home | ✅ Working |
| **TEL Button** | Android Search | ✅ Working |
| **OPTION Button** | Play/Pause | ✅ Working |
| **RADIO Button** | Previous Track | ✅ Working |
| **CD Button** | Next Track | ✅ Working |
| **Joystick** | Mouse movement / Arrow keys | ✅ Working |
| **Joystick Center** | Mouse click / Enter | ✅ Working |
| **Touchpad** | Mouse cursor movement | ✅ Working |
| **Two-finger scroll** | Scroll wheel emulation | ✅ Working |
| **Backlight** | Illumination control | ✅ Working |
| **OTA Updates** | WiFi firmware upload | ✅ Working |

### Touchpad Specifications

| Parameter | Value |
|-----------|-------|
| X Resolution | 512 steps (9-bit) |
| Y Resolution | 512 steps (9-bit) |
| Poll Rate | 200 Hz (5ms) |
| Multi-touch | Up to 2 fingers with coordinates |

## Hardware Requirements

### Components

| Component | Description |
|-----------|-------------|
| ESP32-S3 DevKit | Any ESP32-S3 board with USB OTG support |
| CAN Transceiver | MCP2551, SN65HVD230, or similar 3.3V compatible |
| BMW iDrive Controller | G-series ZBE4 (tested: 65826829079) |
| Power Supply | 12V for iDrive, 5V for ESP32 |

### Tested Controller

| Property | Value |
|----------|-------|
| **BMW Part Number** | 65826829079 |
| **Controller Type** | ZBE-06-TC (iDrive Touch with ceramic surface \ non ceramic) |
| **Compatible controllers** | BMW 5' G30, F90 M5, G31, G38, 6' G32 GT |
| **CAN Bus** | K-CAN4 at 500 kbps |

### Pin Connections

| ESP32-S3 Pin | Connection |
|--------------|------------|
| GPIO 4 | CAN RX (from transceiver RX) |
| GPIO 5 | CAN TX (to transceiver TX) |
| USB | To Android head unit |
| 5V/GND | Power supply |

## Wiring Diagram

> **CRITICAL**: All ground connections (GND) MUST be connected together!

```
                                         ┌─────────────────────┐
                                         │   BMW iDrive        │
                                         │   ZBE4 Controller   │
                                         │                     │
                                         │ CAN-H CAN-L +12V GND│
                                         └──┬─────┬─────┬────┬─┘
                                            │     │     │    │
    ┌───────────────────────────────────────┼─────┼─────┼────┼───────────────┐
    │                                       │     │     │    │               │
    │  COMMON GROUND BUS (GND) ═══════════════════════════════════════════   │
    │    │         │              │         │     │     │    │           │   │
    │    │         │         ┌────┴─────────┴─────┴─────┴────┴───┐       │   │
    │    │         │         │       CAN Transceiver             │       │   │
    │    │         │         │       (MCP2551/SN65HVD230)        │       │   │
    │    │         │         │  RXD  TXD  VCC  GND  CAN-H  CAN-L │       │   │
    │    │         │         └───┬────┬────┬────┬────┬──────┬────┘       │   │
    │    │         │             │    │    │    ╧    │      │            │   │
    │ ┌──┴─────────┴─────────────┴────┴────┴─────────┴──────┴────┐       │   │
    │ │   ESP32-S3 DevKit                                        │       │   │
    │ │   GPIO 4 ◄─── RXD                                        │       │   │
    │ │   GPIO 5 ───► TXD                                        │       │   │
    │ │   3.3V   ───► VCC (transceiver)                          │       │   │
    │ │   GND    ════════════════════════════════════════════════════════╧   │
    │ │   USB    ───────────────────────► To Android Head Unit   │           │
    │ └──────────────────────────────────────────────────────────┘           │
    │                           ┌────────────────────┐                       │
    │                           │  12V Power Supply  │                       │
    │                           │   +12V        GND  │                       │
    │                           └─────┬──────────┬───┘                       │
    │                                 │          ╧═══════════════════════════╧
    │                                 └───────► To iDrive +12V    COMMON GND │
    └────────────────────────────────────────────────────────────────────────┘

    ═══════ = Common Ground Bus (CRITICAL - must connect all GND together)
```

### Why Common Ground is Essential

CAN bus uses **differential signaling**:
- **CAN-H** (High) and **CAN-L** (Low) carry complementary signals
- The CAN transceiver measures the **voltage difference** between CAN-H and CAN-L
- This difference is measured **relative to the local ground (GND)**

**Without common ground:**
- Each device has its own ground reference at different potentials
- The transceiver cannot correctly interpret the differential signal
- Result: No communication, garbage data, or intermittent failures

### Power Supply Notes

| Component | Voltage | Source | Notes |
|-----------|---------|--------|-------|
| iDrive Controller | +12V | Vehicle or DC-DC | Requires stable 12V |
| ESP32-S3 | 5V | USB or regulator | Can be USB-powered |
| CAN Transceiver | 3.3V or 5V | ESP32 or regulator | Match to transceiver model |

**Ground connections checklist:**
- [ ] ESP32 GND → CAN Transceiver GND
- [ ] CAN Transceiver GND → iDrive Controller GND
- [ ] 12V Power Supply GND → iDrive Controller GND
- [ ] All GND points connected to common ground bus

### iDrive Controller Connector

The iDrive controller typically uses a 4-pin or 6-pin connector:

| Pin | Function | Color (typical) | Notes |
|-----|----------|-----------------|-------|
| 1 | CAN-High | Orange/Yellow | Differential pair with CAN-L |
| 2 | CAN-Low | Orange/Brown | Differential pair with CAN-H |
| 3 | +12V Power | Red | 12V DC input |
| 4 | Ground | Brown/Black | **MUST connect to common GND** |

**Note**: Pin colors may vary. Use a multimeter to identify pins.

### CAN Bus Termination

For reliable CAN communication:
- CAN bus requires 120Ω termination resistors at each end of the bus
- If using a short cable (<1m), one 120Ω resistor between CAN-H and CAN-L may be sufficient
- Some CAN transceivers have built-in termination (check datasheet)

### Bench Testing Setup

You can test the entire system on your desk without a vehicle:

**12V Power Supply Options:**

| Option | Description | Notes |
|--------|-------------|-------|
| **ATX PSU** | Old PC power supply | Yellow wire = +12V, Black = GND. Short green wire to black to turn on |
| **DC-DC Converter** | 12V module from AliExpress | Input: 15-24V AC adapter, Output: 12V DC |
| **12V AC Adapter** | Wall adapter 12V 2A+ | Direct 12V, simplest option |
| **Lab Power Supply** | Adjustable bench PSU | Set to 12V, current limit ~500mA |

**Android Device for Testing:**

Instead of a car head unit, use any Android smartphone or tablet with USB OTG support:
- Connect ESP32-S3 USB to phone via **USB OTG adapter/cable**
- Phone will recognize it as USB HID device (keyboard + mouse)
- Test all buttons, rotary encoder, and touchpad
- Use any app to verify media keys (music player for Play/Pause, etc.)

**Bench Test Wiring:**

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│  ATX PSU    │     │   ESP32-S3  │     │  Android    │
│  or 12V     │     │             │     │  Phone      │
│  Adapter    │     │             │     │             │
│             │     │         USB ├─────┤ USB OTG     │
│  +12V ──────┼─────┼──► iDrive   │     │             │
│             │     │             │     │             │
│  GND ═══════╪═════╪═════════════╪═════╪═════════════│ ← COMMON GND!
│             │     │  GND    3.3V│     │             │
└─────────────┘     │    │     │  │     └─────────────┘
                    │    │     │  │
                    │    ▼     ▼  │
                    │  ┌─────────┐│
                    │  │   CAN   ││
                    │  │Transceiv││
                    │  └─────────┘│
                    └─────────────┘

⚠️  IMPORTANT: All GND must be connected together!
    - ATX/Adapter GND
    - ESP32 GND
    - CAN Transceiver GND
    - iDrive Controller GND
```

**ATX Power Supply Quick Start:**
1. Find the 24-pin motherboard connector
2. Locate the green wire (PS_ON) and any black wire (GND)
3. Short green to black with a paperclip or jumper wire — PSU turns on
4. Yellow wires provide +12V, black wires are GND
5. Connect +12V and GND to iDrive controller

## Button Mapping

```
┌─────────────────────────────────────────────────────┐
│                                                     │
│     [MENU]          [OPTION]          [RADIO]       │
│    Android           Play/           Previous       │
│      Menu           Pause             Track         │
│                                                     │
│     [BACK]     ╔═══════════════╗     [CD]          │
│    Android     ║   ↑  Rotary   ║    Next           │
│      Back      ║ ← ● → Volume  ║    Track          │
│                ║   ↓           ║                    │
│     [NAV]      ╚═══════════════╝     [TEL]         │
│    Android          Push:           Android         │
│      Home          Enter            Search          │
│                                                     │
│            ┌─────────────────────┐                  │
│            │      Touchpad       │                  │
│            │  512x512 resolution │                  │
│            │   (Mouse Cursor)    │                  │
│            └─────────────────────┘                  │
│                                                     │
│     Joystick: Mouse Move / Arrow Keys               │
│     Center Press: Click / Enter                     │
│                                                     │
└─────────────────────────────────────────────────────┘
```

## Custom Button Remapping (Android)

The iDrive buttons send standard USB HID media keys that Android recognizes. While the default mapping (see table above) works out of the box, you can customize what each button does using the **Key Mapper** app.

### Why Key Mapper?

USB HID standard doesn't have keycodes for "Open Settings" or "Launch Navigation" - it only defines generic actions like Play/Pause, Home, Back, etc. Key Mapper intercepts these key presses and lets you trigger any action: open apps, toggle settings, run shortcuts, and more.

### Installation

| Source | Link |
|--------|------|
| **F-Droid** | [Key Mapper](https://f-droid.org/packages/io.github.sds100.keymapper/) |
| **Google Play** | [Key Mapper](https://play.google.com/store/apps/details?id=io.github.sds100.keymapper) |
| **GitHub** | [sds100/KeyMapper](https://github.com/sds100/KeyMapper) |

> **Note**: F-Droid version is recommended - it's open source and ad-free.

### Setup Steps

1. **Install Key Mapper** from F-Droid or Play Store
2. **Grant Accessibility Service** - required for intercepting key presses
   - Open Key Mapper → Settings → "Enable accessibility service"
3. **Create New Key Map**:
   - Tap "+" to add new mapping
   - Tap "Record Trigger" → press the iDrive button you want to remap
   - Select Action → "App" → choose the app you want to launch
4. **Enable the mapping** - toggle it on

### Example Mappings

Here are useful remapping ideas for iDrive buttons:

| iDrive Button | Default HID | Suggested Remap | Use Case |
|---------------|-------------|-----------------|----------|
| **TEL** | Search | Phone/Dialer app | Quick calling |
| **NAV** | Home | Google Maps / Waze | Navigation |
| **OPTION** | Play/Pause | Settings | Quick access |
| **RADIO** | Prev Track | Music app (Spotify) | Launch player |
| **CD** | Next Track | Android Auto | Toggle AA mode |

### Advanced Key Mapper Features

- **Long press actions** - different action for hold vs tap
- **Double tap** - trigger on quick double press
- **Constraints** - only active in certain apps
- **Key sequences** - combine multiple buttons
- **System actions** - toggle WiFi, Bluetooth, flashlight, etc.

### iDrive HID Key Codes Reference

When recording keys in Key Mapper, you'll see these codes:

| Button | HID Usage Code | Key Mapper Shows |
|--------|---------------|------------------|
| MENU | 0x0040 | "Menu" |
| BACK | 0x0224 | "AC Back" |
| NAV | 0x0223 | "AC Home" |
| TEL | 0x0221 | "AC Search" |
| OPTION | 0x00CD | "Play/Pause" |
| RADIO | 0x00B6 | "Scan Previous Track" |
| CD | 0x00B5 | "Scan Next Track" |

> **Tip**: If a button doesn't register in Key Mapper, try enabling "Log key events" in Key Mapper settings to debug.

## Software Architecture

### Project Structure

```
bmw-idrive-touch-esp32-s3/
├── include/
│   ├── can/
│   │   ├── can_bus.h              # CAN bus communication (TWAI driver)
│   │   └── can_task.h             # Event-driven CAN task (Core 1)
│   ├── config/
│   │   └── config.h               # Configuration & CAN protocol constants
│   ├── hid/
│   │   ├── hid_keycodes.h         # USB HID key codes
│   │   └── usb_hid_device.h       # USB HID device interface
│   ├── idrive/
│   │   └── idrive_controller.h    # IDriveController class - main orchestrator
│   ├── input/
│   │   ├── input_handler.h        # InputHandler base class & InputEvent
│   │   ├── button_handler.h       # ButtonHandler - media key mapping
│   │   ├── joystick_handler.h     # JoystickHandler - mouse/arrows
│   │   ├── rotary_handler.h       # RotaryHandler - volume control
│   │   └── touchpad_handler.h     # TouchpadHandler - mouse cursor
│   ├── ota/
│   │   ├── ota_config.h           # OTA configuration (WiFi AP, etc.)
│   │   ├── ota_manager.h          # OTA orchestrator
│   │   ├── ota_trigger.h          # Button combo detection
│   │   └── web_server.h           # HTTP upload server
│   ├── utils/
│   │   └── utils.h                # Utility functions (GetMillis, etc.)
│   └── tusb_config.h              # TinyUSB configuration
├── src/
│   ├── main.cpp                   # Application entry point
│   ├── can/
│   │   ├── can_bus.cpp
│   │   └── can_task.cpp           # Event-driven CAN processing
│   ├── hid/usb_hid_device.cpp
│   ├── idrive/idrive_controller.cpp
│   ├── input/*.cpp
│   └── ota/*.cpp                  # OTA implementation
├── docs/
│   └── BMW_iDrive_CAN_Protocol_Research.md  # Detailed protocol documentation
├── partitions_ota.csv             # 8MB flash partition table
└── platformio.ini
```

### Data Flow

```
┌──────────────────┐    CAN Bus     ┌──────────────────┐
│  iDrive ZBE4     │    500kbps     │  CanBus          │
│  Controller      │ ─────────────► │  (TWAI driver)   │
└──────────────────┘                └────────┬─────────┘
                                             │ callback
                                             ▼
                                    ┌──────────────────┐
                                    │ IDriveController │
                                    │ - HandleInput()  │
                                    │ - HandleRotary() │
                                    │ - HandleTouch()  │
                                    └────────┬─────────┘
                                             │ InputEvent
              ┌──────────────────────────────┼──────────────────────────────┐
              ▼                              ▼                              ▼
     ┌──────────────────┐         ┌──────────────────┐          ┌──────────────────┐
     │  ButtonHandler   │         │  RotaryHandler   │          │  TouchpadHandler │
     │  JoystickHandler │         │  (Volume +/-)    │          │  (Mouse Move)    │
     └────────┬─────────┘         └────────┬─────────┘          └────────┬─────────┘
              │                            │                             │
              └────────────────────────────┴─────────────────────────────┘
                                           │
                                           ▼
                                  ┌──────────────────┐
                                  │  UsbHidDevice    │
                                  │  (TinyUSB)       │
                                  └────────┬─────────┘
                                           │
                                           ▼
                                  ┌──────────────────┐
                                  │  Android Device  │
                                  └──────────────────┘
```

### Key Code Patterns

**Dependency Injection in main.cpp:**
```cpp
idrive::CanBus can(GPIO_NUM_4, GPIO_NUM_5);
idrive::UsbHidDevice& hid = idrive::GetUsbHidDevice();
idrive::Config config{ .joystick_as_mouse = true };
idrive::IDriveController controller(can, hid, config);
```

**Input Handler Pattern:**
```cpp
class ButtonHandler : public InputHandler {
    bool Handle(const InputEvent& event) override;
};
```

**RAII Mutex in UsbHidDevice:**
```cpp
if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE) {
    // Thread-safe HID report
    xSemaphoreGive(mutex_);
}
```

### State Machine

```
         ┌─────────────────────────────────────────┐
         │              INITIALIZATION             │
         │                                         │
         │  1. Setup USB HID (TinyUSB)             │
         │  2. Setup CAN bus (TWAI) at 500kbps     │
         │  3. Send Rotary Init (0x273)            │
         │  4. Enable Backlight (0x202)            │
         └────────────────────┬────────────────────┘
                              │
                              ▼
         ┌─────────────────────────────────────────┐
         │         WAIT FOR ROTARY INIT            │
         │                                         │
         │  Wait for response on 0x277             │
         │  Retry every 5 seconds if no response   │
         └────────────────────┬────────────────────┘
                              │ Received 0x277
                              ▼
         ┌─────────────────────────────────────────┐
         │         START TOUCHPAD POLLING          │
         │                                         │
         │  Send poll to 0x317 every 5ms           │
         │  (G-series specific!)                   │
         └────────────────────┬────────────────────┘
                              │
                              ▼
         ┌─────────────────────────────────────────┐
         │            COOLDOWN (750ms)             │
         └────────────────────┬────────────────────┘
                              │
                              ▼
         ┌─────────────────────────────────────────┐
         │               READY                     │
         │                                         │
         │  Process inputs:                        │
         │  - Buttons (0x267) → Media keys         │
         │  - Rotary (0x264) → Volume              │
         │  - Touchpad (0x0BF) → Mouse             │
         │                                         │
         │  Periodic tasks:                        │
         │  - Touchpad poll (0x317) every 5ms      │
         │  - Keepalive (0x501) every 500ms        │
         │  - Light (0x202) every 10s              │
         └─────────────────────────────────────────┘
```

### Task Architecture (Dual-Core ESP32-S3)

The firmware utilizes both cores of the ESP32-S3 for optimal real-time performance:

```
┌─────────────────────────────────────────────────────────────────────┐
│                         ESP32-S3 Dual-Core                          │
├─────────────────────────────┬───────────────────────────────────────┤
│        CORE 0 (PRO)         │           CORE 1 (APP)                │
│                             │                                       │
│  ┌───────────────────────┐  │  ┌─────────────────────────────────┐  │
│  │     TinyUSB Task      │  │  │       CAN Task (Event-Driven)   │  │
│  │   - USB enumeration   │  │  │   - twai_read_alerts() blocks   │  │
│  │   - HID reports       │  │  │   - Process CAN messages        │  │
│  │   - 1ms loop          │  │  │   - High priority (10)          │  │
│  └───────────────────────┘  │  └─────────────────────────────────┘  │
│                             │                                       │
│  ┌───────────────────────┐  │                                       │
│  │     Main Loop         │  │                                       │
│  │   - Controller.Update │  │                                       │
│  │   - OTA check         │  │                                       │
│  │   - 50ms loop         │  │                                       │
│  └───────────────────────┘  │                                       │
├─────────────────────────────┴───────────────────────────────────────┤
│  Why this distribution:                                             │
│  • USB stack (TinyUSB) runs on Core 0 - default ESP-IDF behavior    │
│  • CAN processing on Core 1 - no interference with USB              │
│  • Event-driven CAN - blocks on twai_read_alerts(), low CPU usage   │
│  • Main loop can be slow (50ms) - CAN handles real-time events      │
└─────────────────────────────────────────────────────────────────────┘
```

## OTA Firmware Updates

The adapter supports Over-The-Air firmware updates via WiFi, allowing you to update the firmware without physical access to the USB port.

### How to Trigger OTA Mode

1. **Hold MENU + BACK buttons** on the iDrive controller for **3 seconds**
2. The adapter will create a WiFi access point
3. Connect to the AP and upload new firmware

### OTA WiFi Configuration

| Parameter | Value |
|-----------|-------|
| SSID | `iDrive-OTA` |
| Password | `idrive2024` |
| IP Address | `192.168.4.1` |
| Upload URL | `http://192.168.4.1/` |

### Firmware Upload Steps

1. Trigger OTA mode (MENU + BACK for 3 seconds)
2. Connect your phone/laptop to `iDrive-OTA` WiFi
3. Open browser and go to `http://192.168.4.1/`
4. Select the new `firmware.bin` file and upload
5. Wait for upload to complete and device to reboot

### OTA Partition Layout (8MB Flash)

```
┌──────────────────────────────────────────────────────────────┐
│  0x000000  │  bootloader (0x8000)                            │
├────────────┼─────────────────────────────────────────────────┤
│  0x008000  │  partition table (0x1000)                       │
├────────────┼─────────────────────────────────────────────────┤
│  0x009000  │  NVS storage (0x6000)                           │
├────────────┼─────────────────────────────────────────────────┤
│  0x00F000  │  OTA data (0x2000)                              │
├────────────┼─────────────────────────────────────────────────┤
│  0x020000  │  ota_0 - Main firmware (~3.9MB)                 │
├────────────┼─────────────────────────────────────────────────┤
│  0x410000  │  ota_1 - Backup firmware (~3.9MB)               │
└────────────┴─────────────────────────────────────────────────┘
```

## CAN Bus Protocol

### G-Series ZBE4 Specifics

This controller uses **K-CAN4** at **500 kbps**. Key difference from F-series: touchpad uses different TX ID.

### Message IDs

| ID (Hex) | Direction | Description |
|----------|-----------|-------------|
| 0x273 | TX | Rotary encoder init |
| 0x277 | RX | Rotary init response |
| 0x264 | RX | Rotary encoder position |
| 0x267 | RX | Button and joystick input |
| 0x317 | **TX** | **Touchpad poll (G-series!)** |
| 0x0BF | RX | Touchpad response data |
| 0x202 | TX | Backlight control |
| 0x501 | TX | Keepalive poll |
| 0x5E7 | RX | Status messages |

### Touchpad Protocol (KEY DISCOVERY)

**Critical**: byte[0] bit4 (0x10) must be SET for coordinates to update!

```cpp
// Correct touchpad poll message
uint8_t data[8] = {0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
can.Send(0x317, data, 8);  // Send every 5ms for 200Hz polling
```

### Touchpad Response Format (0x0BF)

```
Byte 0: Counter (low nibble cycles 0-F)
Byte 1: Finger 1 X low byte (0-255)
Byte 2: [high nibble = Y low 4 bits] [low nibble = X high bit (0/1)]
Byte 3: Finger 1 Y high 5 bits (0-31)
Byte 4: Touch state (0x10=1 finger, 0x11=removed, 0x00=2 fingers)
Byte 5-7: Finger 2 data (same format as bytes 1-3)
```

**Coordinate Processing (9-bit X and Y, range 0-511):**

```cpp
// X: 9-bit (byte1 + high bit from byte2)
int16_t x = msg.data[1] + 256 * (msg.data[2] & 0x01);

// Y: 9-bit (byte3 << 4 | byte2 high nibble)
int16_t y = (msg.data[3] << 4) | (msg.data[2] >> 4);
```

### Button Input Format (0x267)

```
Byte 0: Counter
Byte 1-2: Reserved
Byte 3: State (lower nibble) | Direction (upper nibble for joystick)
Byte 4: Input type (0xC0=button, 0xDD=stick, 0xDE=center press)
Byte 5: Button ID
```

**Button IDs:**
- 0x01: Menu, 0x02: Back, 0x04: Option
- 0x08: Radio, 0x10: CD, 0x20: Nav, 0x40: Tel

**Joystick Directions (byte[3] upper nibble):**
- 0x01: Up, 0x02: Right, 0x04: Down, 0x08: Left

**States (byte[3] lower nibble):**
- 0x00: Released, 0x01: Pressed, 0x02: Held

### Rotary Encoder Format (0x264)

```
Byte 3: Position low byte
Byte 4: Position high byte
```

16-bit counter that wraps around. Calculate delta from previous value.

### Other Messages

**Rotary Init (0x273):**
```
Data: {0x1D, 0xE1, 0x00, 0xF0, 0xFF, 0x7F, 0xDE, 0x04}
```

**Light Control (0x202):**
```
Data: {0xFD, 0x00}  // ON
Data: {0xFE, 0x00}  // OFF
```

**Keepalive Poll (0x501):**
```
Data: {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
```

## Building and Flashing

### Using PlatformIO (Recommended)

```bash
# Clone repository
git clone https://github.com/llilakoblock/bmw-idrive-touch-esp32-s3.git
cd bmw-idrive-touch-esp32-s3

# Build
pio run

# Upload
pio run --target upload

# Monitor serial output
pio device monitor
```

## Configuration

All configuration is in `include/config/config.h`.

### Timing Configuration

```cpp
constexpr uint32_t kPollIntervalMs = 5;        // Touchpad poll rate (200Hz)
constexpr uint32_t kLightKeepaliveMs = 10000;  // Backlight refresh
constexpr uint32_t kControllerCooldownMs = 750; // Init delay
```

### Touchpad Sensitivity

```cpp
// Raw coordinate multipliers (both X and Y: 0-511, 9-bit resolution)
constexpr int kXMultiplier = 5;   // X delta * 5 / 10 = 0.5 px/step
constexpr int kYMultiplier = 5;   // Y delta * 5 / 10 = 0.5 px/step (same as X!)
constexpr int kMinMouseTravel = 1; // Dead zone threshold
```

### Debug Options

```cpp
constexpr bool kSerialDebug = true;    // Enable serial output
constexpr bool kDebugCan = false;      // Log all CAN messages
constexpr bool kDebugKeys = true;      // Log button events
constexpr bool kDebugTouchpad = true;  // Log touchpad data
```

## Troubleshooting

### No Response from iDrive

1. Check 12V power to iDrive controller
2. Verify CAN-H and CAN-L wiring (not swapped)
3. Add 120Ω termination resistor between CAN-H and CAN-L
4. Verify CAN speed is 500kbps
5. Check serial monitor for CAN errors

### Touchpad Not Working

1. Ensure rotary init completed first (watch for "Rotary Init Success" in logs)
2. Verify poll message uses `0x10` in byte[0] (not `0x21`!)
3. Check for touchpad messages on 0x0BF in serial log
4. Touchpad is poll-based - must send 0x317 continuously

### USB Not Recognized

1. Ensure ESP32-S3 has USB OTG support (not all boards do)
2. Use a data-capable USB cable (not charge-only)
3. Try different USB port on host device

### Touchpad Moves Wrong Direction

Y-axis is inverted in code. If still wrong, check `touchpad_handler.cpp`:
```cpp
int8_t mouse_y = -delta_y * y_multiplier_ / 10;  // Negative = inverted
```

### Movement is Choppy

1. Increase poll rate: `kPollIntervalMs = 5` (200Hz)
2. Adjust multipliers for smoother feel
3. Check for CAN bus errors causing dropped messages

## Protocol Documentation

For detailed protocol research and byte-level documentation, see:
[docs/BMW_iDrive_CAN_Protocol_Research.md](docs/BMW_iDrive_CAN_Protocol_Research.md)

## License

MIT License - See LICENSE file for details.

## Acknowledgments

- BMW enthusiast community for initial protocol research
- [thatdamnranga/iDrive](https://github.com/thatdamnranga/iDrive) - F-series protocol reference
- [IAmOrion/BMW-iDrive-HID](https://github.com/IAmOrion/BMW-iDrive-HID) - BLE HID implementation reference
- [TinyUSB](https://github.com/hathach/tinyusb) project
- ESP-IDF team at Espressif