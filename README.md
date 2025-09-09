
# iDrive Integration with ESP32-S3 (ESP-IDF & PlatformIO)

> **Disclaimer**: This project is not affiliated with or endorsed by BMW. It is intended for educational and experimental purposes only. Use of the code in a vehicle or any other production environment is at your own risk.

## Introduction

Modern BMW iDrive controllers use CAN bus messages for communicating button presses, rotary knob movement, and touchpad data. This project aims to decode and handle these messages with an ESP32-S3 board. The code also demonstrates sending CAN frames to keep the controller’s backlight on, reinitialize the rotary position, and poll the device regularly.

This repository includes:

- **CMakeLists.txt / platformio.ini**: Build configurations for ESP-IDF (CMake) and PlatformIO.
- **Source code** (`src/` directory): ESP-IDF application logic for CAN bus communication with the iDrive controller.
- **Header files** (`include/` directory): Definitions for constants, global variables, HID key assignments, and function declarations.
- **tusb_config.h**: TinyUSB configuration for enabling the CDC and HID interfaces (as an example for HID usage, if you extend the code to emulate a keyboard or mouse).

---

## Features

- **CAN Bus Communication**: Uses `driver/twai.h` on the ESP-IDF to transmit and receive CAN frames from the iDrive controller.
- **iDrive Controller Initialization**: Automatically sends the required frames to initialize the controller’s rotary knob and touchpad.
- **Periodic Polling**: Sends periodic messages to keep the controller active and retrieve its status.
- **Lighting Control**: Toggles or keeps the iDrive’s backlight on by sending specific frames.
- **Expandable to HID**: The code references HID key assignments (e.g., for joystick movements, knob rotation), which can be extended to build a full USB HID device (e.g., a keyboard or mouse over USB or Bluetooth).

---

## Hardware & iDrive Version

This example is tested on an **ESP32-S3** DevKit board. The wiring assumes:

- **TX Pin** (GPIO 4)
- **RX Pin** (GPIO 5)

These pins connect to a CAN transceiver (e.g., an MCP2551 or similar transceiver), which then connects to the iDrive controller’s CAN lines.

The exact BMW iDrive version can vary (e.g., CCC, CIC, NBT, or later iDrive Touch controllers). The code’s CAN IDs (e.g., 0x264, 0x267, 0x277, 0x5E7, 0xBF, etc.) are known to match certain iDrive Touch controllers found in later BMW models (F-series and beyond). Adjustments may be necessary if your iDrive version uses different CAN IDs.

---

## Software Requirements

- **ESP-IDF** (v4.x or newer recommended) or **PlatformIO** with the `espressif32` platform.
- **CMake** (if using ESP-IDF directly).
- **Python 3** (for PlatformIO, or any additional scripting).
- A **CAN transceiver** library/hardware if needed on the hardware side.

---

## Project Structure

```bash
idrive/
├── .build/                       # Build output directory (auto-generated)
├── include/
│   ├── idrive.h                  # Declarations for iDrive handling
│   ├── key_assignments.h         # HID key definitions
│   ├── settings.h                # Feature flags, timings, debugging
│   ├── variables.h               # Extern declarations for global variables
├── src/
│   ├── CMakeLists.txt            # Auto-generated CMake for the 'src' dir
│   ├── idrive.cpp                # iDrive logic and CAN message handling
│   ├── main.cpp                  # Main application (esp-idf entry point)
│   ├── variables.cpp             # Definitions for global variables
├── tusb_config.h                 # TinyUSB configuration
├── CMakeLists.txt                # Top-level CMake
├── platformio.ini                # PlatformIO configuration
└── README.md                     # Project documentation (this file)
```

## Setup and Usage

1. **Clone this repository** to your local machine.  
2. **Install ESP-IDF or PlatformIO**:
   - **ESP-IDF**: Install [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/) for the ESP32-S3.
     - Run `idf.py set-target esp32s3` to select the ESP32-S3 chip.
     - Build and flash with `idf.py build` and `idf.py flash`.
   - **PlatformIO**: Install [PlatformIO](https://platformio.org/install) in VSCode or another environment.
     - The environment `[env:esp32-s3-devkitc-1-n16r8v]` is already configured in `platformio.ini`.
     - Build and upload with `pio run` and `pio run --target upload`.
3. **Wiring**:
   - Connect the ESP32-S3 TX/RX pins (as defined in `main.cpp`) to your CAN transceiver.
   - Ensure proper power supply (the iDrive controller typically requires 12V, but consult official documentation or tested setups).
4. **Monitor**:
   - Use `idf.py monitor` (ESP-IDF) or `pio device monitor` (PlatformIO) to see debug logs.
   - Adjust the `monitor_speed` in `platformio.ini` if necessary (115200 by default).

---

## How It Works

### CAN Communication

- **Initialization Frames**:
  - **`iDriveInit()`** sends a frame with ID `0x273` to tell the iDrive controller that the rotary knob is ready.
  - **`iDriveTouchpadInit()`** sends a frame with ID `0xBF` (or `0xFFFFFFBF` in extended mode if needed for debugging) to initialize the touchpad module.
- **Polling and Backlight**:
  - **`iDrivePoll()`** sends periodic frames (ID `0x501`) so the iDrive doesn’t time out.
  - **`iDriveLight()`** and **`do_iDriveLight()`** manage the backlight by toggling ID `0x202`.
- **Decoding Incoming Messages**:
  - **`decodeCanBus()`** prints each CAN frame for debugging, and relevant ones (e.g., IDs `0x267`, `0x264`, `0x277`, `0x5E7`, `0xBF`) are handled.
  - If the iDrive reports a re-initialization requirement (e.g., `0x5E7` with `buf[4] == 0x06`), the code will flag it as lost init and re-trigger the initialization sequence.

### HID Emulation (Optional Extension)

- The included `key_assignments.h` shows how each iDrive button or movement could map to a **USB HID keycode**. Although the code currently logs and prints events, you could integrate [TinyUSB](https://github.com/hathach/tinyusb) or the [ESP-IDF HID API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/usb_serial_jtag_hid.html) to emulate keyboard/mouse behavior.
- **`tusb_config.h`** enables TinyUSB’s CDC and HID classes. You can expand it further to send keyboard or mouse reports when decoding iDrive events.

---

## License

Unless otherwise specified, this project is licensed under the **MIT License**. See [LICENSE](LICENSE) for details.

---

## Further Reading

- [ESP-IDF TWAI (CAN) Driver Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/twai.html)  
- [TinyUSB GitHub Repository](https://github.com/hathach/tinyusb)  
- [BMW iDrive CAN Reverse-Engineering (various community forums)](https://www.bimmerforums.com/), [E46Fanatics](https://www.e46fanatics.com/)  
- Additional BMW/iDrive documentation (service manuals, enthusiast forums, etc.) for specifics on hardware pinouts and voltages.

