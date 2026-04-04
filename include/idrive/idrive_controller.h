// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// Main iDrive controller class - orchestrates CAN communication and input handling.

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "esp_timer.h"

#include "can/can_bus.h"
#include "config/config.h"
#include "hid/usb_hid_device.h"
#include "input/button_handler.h"
#include "input/joystick_handler.h"
#include "input/rotary_handler.h"
#include "input/touchpad_handler.h"

namespace idrive {

// Forward declaration for OTA trigger.
namespace ota {
class OtaTrigger;
}

// =============================================================================
// iDrive Controller Class
// =============================================================================

class IDriveController {
   public:
    IDriveController(CanBus &can, UsbHidDevice &hid, const Config &config);

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
    JoystickHandler *GetJoystickHandler() { return joystick_handler_; }
    RotaryHandler   *GetRotaryHandler() { return rotary_handler_; }
    TouchpadHandler *GetTouchpadHandler() { return touchpad_handler_; }

    // Set OTA trigger for button combo detection.
    void SetOtaTrigger(ota::OtaTrigger *trigger) { ota_trigger_ = trigger; }

   private:
    CanBus       &can_;
    UsbHidDevice &hid_;
    Config        config_;

    // Input handlers.
    std::vector<std::unique_ptr<InputHandler>> handlers_;
    ButtonHandler                             *button_handler_   = nullptr;
    JoystickHandler                           *joystick_handler_ = nullptr;
    RotaryHandler                             *rotary_handler_   = nullptr;
    TouchpadHandler                           *touchpad_handler_ = nullptr;

    // State tracking.
    bool ready_                 = false;
    bool rotary_init_done_      = false;
    bool touchpad_init_done_    = false;
    bool touchpad_active_       = false;  // True after receiving first 0xBF response
    bool light_init_done_       = false;
    bool rotary_position_set_   = false;  // True after receiving first 0xBF response
    bool light_enabled_         = true;
    bool controller_awake_      = false;  // Legacy 0x264 rotary path initialized
    bool startup_commands_sent_ = false;  // First real incoming CAN frame seen
    bool sleep_mode_active_     = false;  // Inactivity sleep / quiet mode

    // Rotary/input parser state.
    // rotary_position_ is used by the legacy 0x264 rotary path.
    // alt_* fields are used by the main 0x25B input path on this controller.
    uint32_t rotary_position_              = 0;
    uint32_t alt_rotary_position_          = 0;
    int      touchpad_init_ignore_counter_ = 0;

    bool     alt_rotary_position_set_      = false;
    bool     alt_joystick_active_          = false;
    uint8_t  last_alt_button_id_           = 0;
    uint8_t  last_alt_joystick_id_         = 0;

    // Raw MENU/BACK state on 0x25B byte 4.
    bool     alt_menu_pressed_             = false;
    bool     alt_back_pressed_             = false;

    // OTA trigger (optional, for button combo detection).
    ota::OtaTrigger *ota_trigger_ = nullptr;

    // OPTION long-hold reset.
    bool               option_held_                 = false;
    bool               option_long_hold_triggered_  = false;
    esp_timer_handle_t option_reset_timer_          = nullptr;

    // Timing.
    uint32_t init_start_time_         = 0;
    uint32_t cooldown_start_time_     = 0;
    uint32_t last_poll_time_          = 0;
    uint32_t last_light_time_         = 0;
    uint32_t last_reinit_time_        = 0;
    uint32_t last_touchpad_init_time_ = 0;
    uint32_t last_activity_time_      = 0;

    // CAN message handlers.
    void OnCanMessage(const CanMessage &msg);
    void HandleInputAltMessage(const CanMessage &msg); // Main 0x25B input path
    void HandleInputMessage(const CanMessage &msg); // Legacy 0x267 input path
    void HandleRotaryMessage(const CanMessage &msg); // Legacy 0x267 input path
    void HandleTouchpadMessage(const CanMessage &msg);
    void HandleRotaryInitResponse(const CanMessage &msg); // Legacy 0x277 init response
    void HandleStatusMessage(const CanMessage &msg);

    // Initialization commands.
    void SendRotaryInit();
    void SendTouchpadInit();
    void SendLightCommand();
    void SendPollCommand();

    // Input event dispatch.
    void DispatchEvent(const InputEvent &event);

    // OPTION long-hold reset helpers.
    void InitOptionResetTimer();
    void HandleOptionResetState(bool pressed);
    void SendOptionShortPress();
    static void OptionResetTimerCallback(void *arg);

    // Sleep mode after inactivity period.
    void MarkActivity();
    void EnterSleepMode();
};

}  // namespace idrive
