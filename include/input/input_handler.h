// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// Input handler base class and event structures.

#pragma once

#include <cstdint>

#include "hid/usb_hid_device.h"

namespace idrive {

// =============================================================================
// Input Event Structure
// =============================================================================

struct InputEvent {
    enum class Type {
        Button,
        Joystick,
        Rotary,
        Touchpad
    };

    Type type;
    uint8_t id = 0;       // Button ID or direction
    uint8_t state = 0;    // Pressed/Released/Held
    int16_t x = 0;        // Finger 1 X (0-255)
    int16_t y = 0;        // Finger 1 Y (0-8191, 12-bit)
    int16_t x2 = 0;       // Finger 2 X (0-255, valid when two_fingers=true)
    int16_t y2 = 0;       // Finger 2 Y (0-8191, valid when two_fingers=true)
    bool two_fingers = false;  // Multi-touch active
    int16_t delta = 0;    // For rotary encoder
};

// =============================================================================
// Input Handler Base Class
// =============================================================================

class InputHandler {
public:
    explicit InputHandler(UsbHidDevice& hid) : hid_(hid) {}
    virtual ~InputHandler() = default;

    // Handle an input event. Returns true if event was handled.
    virtual bool Handle(const InputEvent& event) = 0;

protected:
    UsbHidDevice& hid_;
};

}  // namespace idrive
