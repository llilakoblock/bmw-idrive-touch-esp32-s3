// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// USB HID device class for keyboard, mouse, and media controls.

#pragma once

#include <cstdint>
#include <mutex>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace idrive {

// =============================================================================
// USB HID Device Class
// =============================================================================

class UsbHidDevice {
public:
    UsbHidDevice() = default;
    ~UsbHidDevice() = default;

    // Non-copyable, non-movable (singleton-like behavior for USB).
    UsbHidDevice(const UsbHidDevice&) = delete;
    UsbHidDevice& operator=(const UsbHidDevice&) = delete;

    // Initialize USB HID device.
    bool Init();

    // Check if USB device is connected and ready.
    bool IsConnected() const;

    // =========================================================================
    // Keyboard Functions
    // =========================================================================

    void KeyPress(uint8_t keycode);
    void KeyRelease(uint8_t keycode);
    void KeyPressAndRelease(uint8_t keycode);

    // =========================================================================
    // Media Control Functions (Consumer Page)
    // =========================================================================

    void MediaKeyPress(uint16_t keycode);
    void MediaKeyRelease(uint16_t keycode);
    void MediaKeyPressAndRelease(uint16_t keycode);

    // =========================================================================
    // Mouse Functions
    // =========================================================================

    void MouseMove(int8_t x, int8_t y);
    void MouseButtonPress(uint8_t button);
    void MouseButtonRelease(uint8_t button);
    void MouseClick(uint8_t button);
    void MouseScroll(int8_t wheel);

    // =========================================================================
    // TinyUSB Callbacks (called from C callbacks)
    // =========================================================================

    void OnMount();
    void OnUnmount();

private:
    SemaphoreHandle_t mutex_ = nullptr;
    bool connected_ = false;
    bool initialized_ = false;

    // Current report states.
    struct {
        uint8_t modifier = 0;
        uint8_t reserved = 0;
        uint8_t keycode[6] = {0};
    } keyboard_report_ = {};

    struct {
        uint8_t buttons = 0;
        int8_t x = 0;
        int8_t y = 0;
        int8_t wheel = 0;
        int8_t pan = 0;
    } mouse_report_ = {};

    void SendKeyboardReport();
    void SendMouseReport();
};

// Global instance for TinyUSB callbacks.
UsbHidDevice& GetUsbHidDevice();

}  // namespace idrive
