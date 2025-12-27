# BMW iDrive Touch Controller for Android Head Unit

> ESP32-S3 adapter that connects BMW iDrive Touch controller to Android head units via USB HID.

**Disclaimer**: This project is not affiliated with or endorsed by BMW. It is intended for educational and experimental purposes only. Use in a vehicle is at your own risk.

## Table of Contents

- [Overview](#overview)
- [Why This Project?](#why-this-project)
- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Wiring Diagram](#wiring-diagram)
- [Button Mapping](#button-mapping)
- [Software Architecture](#software-architecture)
- [CAN Bus Protocol](#can-bus-protocol)
- [Building and Flashing](#building-and-flashing)
- [Configuration](#configuration)
- [Troubleshooting](#troubleshooting)
- [License](#license)

## Overview

This project enables you to use a genuine BMW iDrive Touch controller (from F-series and later vehicles) with aftermarket Android head units. The ESP32-S3 microcontroller acts as a bridge:

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

| Input | Function |
|-------|----------|
| **Rotary Encoder** | Volume Up/Down |
| **Rotary Push** | Enter/Select |
| **MENU Button** | Android Menu |
| **BACK Button** | Android Back |
| **NAV Button** | Android Home |
| **TEL Button** | Android Search |
| **OPTION Button** | Play/Pause |
| **RADIO Button** | Previous Track |
| **CD Button** | Next Track |
| **Joystick** | Mouse movement or Arrow keys |
| **Joystick Center** | Mouse click or Enter |
| **Touchpad** | Mouse cursor movement |

### Customizable Button Mapping

All buttons can be remapped to any Android media key or keyboard key by editing `src/idrive.cpp`. Available HID codes are defined in `include/usb_hid_device.h`:

**Android-specific keys:**
- `HID_ANDROID_BACK` — Back navigation
- `HID_ANDROID_HOME` — Home screen
- `HID_ANDROID_MENU` — Menu key
- `HID_ANDROID_SEARCH` — Search

**Media control keys:**
- `HID_MEDIA_PLAY_PAUSE` — Play/Pause toggle
- `HID_MEDIA_NEXT_TRACK` — Next track
- `HID_MEDIA_PREV_TRACK` — Previous track
- `HID_MEDIA_VOLUME_UP` — Volume up
- `HID_MEDIA_VOLUME_DOWN` — Volume down
- `HID_MEDIA_MUTE` — Mute toggle
- `HID_MEDIA_STOP` — Stop playback

**Standard keyboard keys:**
- `HID_KEY_ENTER`, `HID_KEY_ESC`, `HID_KEY_TAB`, `HID_KEY_SPACE`
- `HID_KEY_UP`, `HID_KEY_DOWN`, `HID_KEY_LEFT`, `HID_KEY_RIGHT`
- `HID_KEY_A` through `HID_KEY_Z`, `HID_KEY_0` through `HID_KEY_9`
- `HID_KEY_F1` through `HID_KEY_F12`

## Hardware Requirements

### Components

| Component | Description |
|-----------|-------------|
| ESP32-S3 DevKit | Any ESP32-S3 board with USB OTG support |
| CAN Transceiver | MCP2551, SN65HVD230, or similar 3.3V compatible |
| BMW iDrive Controller | iDrive Touch controller (see compatibility below) |
| Power Supply | 12V for iDrive, 5V for ESP32 |

### Tested Controller

All development and testing was performed with:

| Property | Value |
|----------|-------|
| **BMW Part Number** | 65826829079 |
| **Compatible Models** | BMW 5' G30, F90 M5, G31, G38, 6' G32 GT |
| **Controller Type** | iDrive Touch with ceramic surface |

### Compatibility with Other Controllers

This project should work with most **BMW iDrive Touch controllers** from F-series and G-series vehicles:

| Generation | Models | Expected Compatibility |
|------------|--------|------------------------|
| F-series | F10, F20, F30, F25, F15, etc. | High - same protocol |
| G-series | G30, G11, G01, G05, etc. | High - tested |
| E-series | E60, E90, E70, etc. | Unknown - older protocol |

**Note on CAN protocol differences:**
- All iDrive Touch controllers share very similar external design with minimal visual differences
- The CAN bus protocol is likely identical or nearly identical across all iDrive Touch variants
- Minor differences (if any) may include different CAN message IDs or data byte positions
- If your controller doesn't work, enable debug logging to capture CAN messages and compare with documented protocol

### Pin Connections

| ESP32-S3 Pin | Connection |
|--------------|------------|
| GPIO 4 | CAN RX (from transceiver RX) |
| GPIO 5 | CAN TX (to transceiver TX) |
| USB | To Android head unit |
| 5V/GND | Power supply |

## Wiring Diagram

> **CRITICAL**: All ground connections (GND) MUST be connected together! CAN bus is a differential signal that requires a common ground reference between all devices. Without common ground, the CAN transceiver cannot properly interpret the voltage difference between CAN-H and CAN-L lines.

```
                                         ┌─────────────────────┐
                                         │   BMW iDrive        │
                                         │   Controller        │
                                         │                     │
                                         │ CAN-H CAN-L +12V GND│
                                         └──┬─────┬─────┬────┬─┘
                                            │     │     │    │
    ┌───────────────────────────────────────┼─────┼─────┼────┼───────────────┐
    │                                       │     │     │    │               │
    │  COMMON GROUND BUS (GND) ═══════════════════════════════════════════   │
    │    │         │              │         │     │     │    │           │   │
    │    │         │              │         │     │     │    │           │   │
    │    │         │         ┌────┴─────────┴─────┴─────┴────┴───┐       │   │
    │    │         │         │                                   │       │   │
    │    │         │         │       CAN Transceiver             │       │   │
    │    │         │         │       (MCP2551/SN65HVD230)        │       │   │
    │    │         │         │                                   │       │   │
    │    │         │         │  RXD  TXD  VCC  GND  CAN-H  CAN-L │       │   │
    │    │         │         └───┬────┬────┬────┬────┬──────┬────┘       │   │
    │    │         │             │    │    │    │    │      │            │   │
    │    │         │             │    │    │    ╧    │      │            │   │
    │    │         │             │    │    │   GND   │      │            │   │
    │    │         │             │    │    │         │      │            │   │
    │ ┌──┴─────────┴─────────────┴────┴────┴─────────┴──────┴────┐       │   │
    │ │                                                          │       │   │
    │ │   ESP32-S3 DevKit                                        │       │   │
    │ │                                                          │       │   │
    │ │   GPIO 4 ◄─── RXD                                        │       │   │
    │ │   GPIO 5 ───► TXD                                        │       │   │
    │ │   3.3V   ───► VCC (transceiver)                          │       │   │
    │ │   GND    ═══════════════════════════════════════════════════════╧   │
    │ │                                                          │           │
    │ │   USB    ───────────────────────► To Android Head Unit   │           │
    │ │                                                          │           │
    │ └──────────────────────────────────────────────────────────┘           │
    │                                                                        │
    │                           ┌────────────────────┐                       │
    │                           │  12V Power Supply  │                       │
    │                           │  (Vehicle/PSU)     │                       │
    │                           │                    │                       │
    │                           │   +12V        GND  │                       │
    │                           └─────┬──────────┬───┘                       │
    │                                 │          │                           │
    │                                 │          ╧                           │
    │                                 │    ══════╧═══════════════════════════╧
    │                                 │         COMMON GND
    │                                 │
    │                                 └───────► To iDrive +12V
    │                                                                        │
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

### Default Android Mapping

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
│            │   (Mouse Cursor)    │                  │
│            └─────────────────────┘                  │
│                                                     │
│     Joystick: Mouse Move / Arrow Keys               │
│     Center Press: Click / Enter                     │
│                                                     │
└─────────────────────────────────────────────────────┘
```

## Software Architecture

### Project Structure

```
bmw-idrive-touch-esp32-s3/
├── include/
│   ├── idrive.h          # CAN protocol definitions and iDrive API
│   ├── settings.h        # Configuration settings
│   ├── variables.h       # Global state variables
│   ├── key_assignments.h # HID key code definitions
│   ├── usb_hid_device.h  # USB HID device API
│   └── tusb_config.h     # TinyUSB configuration
├── src/
│   ├── main.cpp          # Application entry point and main loop
│   ├── idrive.cpp        # CAN message handling and input processing
│   ├── usb_hid_device.c  # USB HID implementation
│   └── variables.cpp     # Global variable definitions
├── platformio.ini        # PlatformIO build configuration
├── sdkconfig.defaults    # ESP-IDF SDK configuration
└── README.md             # This file
```

### Module Overview

| Module | Purpose |
|--------|---------|
| **main.cpp** | Initializes hardware (USB, CAN), runs main loop, handles watchdog |
| **idrive.cpp** | Decodes CAN messages, handles button/rotary/touchpad events, sends HID reports |
| **usb_hid_device.c** | USB HID stack using TinyUSB, keyboard/mouse/media key functions |
| **settings.h** | Timing configuration, feature flags, debug options |
| **variables.h** | Shared state (init flags, positions, timing counters) |

### Data Flow

```
┌──────────────────┐    CAN Bus     ┌──────────────────┐
│  iDrive          │    500kbps     │  ESP32-S3        │
│  Controller      │ ─────────────► │  TWAI Driver     │
└──────────────────┘                └────────┬─────────┘
                                             │
                                             ▼
                                    ┌──────────────────┐
                                    │  DecodeCanMessage│
                                    │  (idrive.cpp)    │
                                    └────────┬─────────┘
                                             │
              ┌──────────────────────────────┼──────────────────────────────┐
              ▼                              ▼                              ▼
     ┌──────────────────┐         ┌──────────────────┐          ┌──────────────────┐
     │  HandleButton    │         │  HandleRotary    │          │  HandleTouchpad  │
     │  HandleJoystick  │         │  (Volume +/-)    │          │  (Mouse Move)    │
     └────────┬─────────┘         └────────┬─────────┘          └────────┬─────────┘
              │                            │                             │
              └──────────────────────────────────────────────────────────┘
                                           │
                                           ▼
                                  ┌──────────────────┐
                                  │  USB HID Device  │
                                  │  (TinyUSB)       │
                                  └────────┬─────────┘
                                           │
                                           ▼
                                  ┌──────────────────┐
                                  │  Android         │
                                  │  Head Unit       │
                                  └──────────────────┘
```

### State Machine

```
         ┌─────────────────────────────────────────┐
         │              INITIALIZATION             │
         │                                         │
         │  1. Setup USB HID                       │
         │  2. Setup CAN (TWAI) at 500kbps         │
         │  3. Send Rotary Init (0x273)            │
         │  4. Enable Backlight (0x202)            │
         └─────────────────────────────────────────┘
                            │
                            ▼
         ┌─────────────────────────────────────────┐
         │         WAIT FOR ROTARY INIT            │
         │                                         │
         │  Wait for Init Response (0x277)         │
         │  Retry every 5 seconds if no response   │
         └─────────────────────────────────────────┘
                            │ Received 0x277
                            ▼
         ┌─────────────────────────────────────────┐
         │         INIT TOUCHPAD                   │
         │                                         │
         │  Send Touchpad Init (0xBF)              │
         └─────────────────────────────────────────┘
                            │
                            ▼
         ┌─────────────────────────────────────────┐
         │            COOLDOWN                     │
         │                                         │
         │  Wait 750ms for stability               │
         └─────────────────────────────────────────┘
                            │
                            ▼
         ┌─────────────────────────────────────────┐
         │             READY                       │
         │                                         │
         │  Process all inputs:                    │
         │  - Buttons → Media/Android keys         │
         │  - Rotary → Volume                      │
         │  - Joystick → Mouse/Arrows              │
         │  - Touchpad → Mouse movement            │
         │                                         │
         │  Periodic tasks:                        │
         │  - Poll (0x501) every 500ms             │
         │  - Light (0x202) every 10s              │
         └─────────────────────────────────────────┘
                            │ Status 0x06 received
                            ▼
                   ┌─────────────────┐
                   │  REINITIALIZE   │
                   └────────┬────────┘
                            │
                            └──────────► (back to INITIALIZATION)
```

## CAN Bus Protocol

### Message IDs

| ID (Hex) | Direction | Description |
|----------|-----------|-------------|
| 0x273 | TX | Rotary encoder initialization |
| 0x202 | TX | Backlight control |
| 0x501 | TX | Keep-alive poll |
| 0x264 | RX | Rotary encoder position |
| 0x267 | RX | Button and joystick input |
| 0x277 | RX | Initialization response |
| 0x5E7 | RX | Status messages |
| 0xBF | TX/RX | Touchpad init and data |

### Message Details

#### Rotary Init (0x273)
```
Data: 1D E1 00 F0 FF 7F DE 04
```

#### Light Control (0x202)
```
Data: 02 FD 00  (ON)
Data: 02 FE 00  (OFF)
```

#### Poll (0x501)
```
Data: 01 00 00 00 00 00 00 00
```

#### Button Input (0x267)
```
Byte 3: State (lower 4 bits: 0=released, 1=pressed, 2=held)
Byte 4: Input type (0xC0=button, 0xDD=stick, 0xDE=center)
Byte 5: Button ID or direction
```

#### Rotary Position (0x264)
```
Byte 3-4: 16-bit position counter (little endian)
```

#### Touchpad (0xBF)
```
Byte 1: X position (0-255)
Byte 2: X quadrant (0=left, 1=right)
Byte 3: Y position (0-30)
Byte 4: Touch type (0x10=single, 0x11=removed)
```

## Building and Flashing

### Prerequisites

- [PlatformIO](https://platformio.org/) or [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/)
- USB cable for ESP32-S3

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

### Using ESP-IDF

```bash
# Set target
idf.py set-target esp32s3

# Configure (optional)
idf.py menuconfig

# Build
idf.py build

# Flash
idf.py flash

# Monitor
idf.py monitor
```

## Configuration

### settings.h Options

| Option | Description | Default |
|--------|-------------|---------|
| `IDRIVE_JOYSTICK_AS_MOUSE` | Joystick controls mouse instead of arrow keys | Enabled |
| `kPollIntervalMs` | Polling interval | 500ms |
| `kLightKeepaliveIntervalMs` | Light keepalive | 10000ms |
| `kControllerCooldownMs` | Ready delay | 750ms |
| `kMinMouseTravel` | Touch deadzone | 5 |
| `kJoystickMoveStep` | Joystick mouse step | 30 |

### Debug Options

Enable in `settings.h`:
```cpp
#define SERIAL_DEBUG       // Serial output
#define DEBUG_CAN_RESPONSE // Log CAN messages
#define DEBUG_KEYS         // Log key events
#define DEBUG_TOUCHPAD     // Log touchpad data
```

## Troubleshooting

### No Response from iDrive

1. Check 12V power to iDrive controller
2. Verify CAN-H and CAN-L wiring
3. Check CAN bus termination (120Ω between CAN-H and CAN-L)
4. Verify CAN speed is 500kbps
5. Check serial monitor for CAN errors

### USB Not Recognized

1. Ensure ESP32-S3 has USB OTG support
2. Use a data-capable USB cable
3. Check for USB enumeration in `dmesg` (Linux) or Device Manager (Windows)
4. Verify TinyUSB configuration in `tusb_config.h`

### Touchpad Not Working

1. Touchpad requires rotary init to complete first
2. Check for touchpad messages (0xBF) in serial log
3. Verify touchpad connector is secure

### CAN Bus Errors

- **Error Passive**: Check termination and wiring
- **Bus Off**: Severe bus error, check for shorts
- **TX Failed**: Controller not responding, check power

### Known Limitations

- Some iDrive variants may use different CAN IDs
- Very old (E-series) or very new (G-series) controllers may require protocol adjustments
- Touchpad sensitivity may need tuning for your preference

## License

MIT License - See LICENSE file for details.

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Follow Google C++ Style Guide
4. Submit a pull request

## Acknowledgments

- BMW enthusiast community for protocol documentation
- [TinyUSB](https://github.com/hathach/tinyusb) project
- ESP-IDF team at Espressif
