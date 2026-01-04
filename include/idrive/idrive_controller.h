// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// Main iDrive controller class - orchestrates CAN communication and input handling.

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "can/can_bus.h"
#include "config/config.h"
#include "hid/usb_hid_device.h"
#include "input/button_handler.h"
#include "input/joystick_handler.h"
#include "input/rotary_handler.h"
#include "input/touchpad_handler.h"

namespace idrive {

// =============================================================================
// iDrive Controller Class
// =============================================================================

class IDriveController {
public:
    IDriveController(CanBus& can, UsbHidDevice& hid, const Config& config);

    // Initialize the controller. Call after CAN and USB are initialized.
    void Init();

    // Update the controller state. Call regularly in the main loop.
    void Update();

    // Check if controller is fully initialized and ready.
    bool IsReady() const { return ready_; }

    // Set light brightness (0-100).
    void SetLightBrightness(uint8_t brightness);

    // Toggle light on/off.
    void SetLightEnabled(bool enabled);

    // Access to handlers for runtime configuration.
    JoystickHandler* GetJoystickHandler() { return joystick_handler_; }
    RotaryHandler* GetRotaryHandler() { return rotary_handler_; }
    TouchpadHandler* GetTouchpadHandler() { return touchpad_handler_; }

private:
    CanBus& can_;
    UsbHidDevice& hid_;
    Config config_;

    // Input handlers.
    std::vector<std::unique_ptr<InputHandler>> handlers_;
    ButtonHandler* button_handler_ = nullptr;
    JoystickHandler* joystick_handler_ = nullptr;
    RotaryHandler* rotary_handler_ = nullptr;
    TouchpadHandler* touchpad_handler_ = nullptr;

    // State tracking.
    bool ready_ = false;
    bool rotary_init_done_ = false;
    bool touchpad_init_done_ = false;
    bool touchpad_active_ = false;  // True after receiving first 0xBF response
    bool light_init_done_ = false;
    bool rotary_position_set_ = false;
    bool light_enabled_ = true;

    uint32_t rotary_position_ = 0;
    int touchpad_init_ignore_counter_ = 0;

    // Timing.
    uint32_t init_start_time_ = 0;
    uint32_t cooldown_start_time_ = 0;
    uint32_t last_poll_time_ = 0;
    uint32_t last_light_time_ = 0;
    uint32_t last_reinit_time_ = 0;
    uint32_t last_touchpad_init_time_ = 0;

    // CAN message handlers.
    void OnCanMessage(const CanMessage& msg);
    void HandleInputMessage(const CanMessage& msg);
    void HandleRotaryMessage(const CanMessage& msg);
    void HandleTouchpadMessage(const CanMessage& msg);
    void HandleRotaryInitResponse(const CanMessage& msg);
    void HandleStatusMessage(const CanMessage& msg);

    // Initialization commands.
    void SendRotaryInit();
    void SendTouchpadInit();
    void SendLightCommand();
    void SendPollCommand();

    // Input event dispatch.
    void DispatchEvent(const InputEvent& event);
};

}  // namespace idrive
