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
    int16_t x = 0;        // For touchpad
    int16_t y = 0;        // For touchpad
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
