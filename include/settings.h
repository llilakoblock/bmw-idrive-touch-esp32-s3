// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// Configuration settings for iDrive controller adapter.

#ifndef BMW_IDRIVE_ESP32_SETTINGS_H_
#define BMW_IDRIVE_ESP32_SETTINGS_H_

// =============================================================================
// Feature Flags
// =============================================================================

// Enable Android-specific media key mappings.
#define ANDROID

// Joystick mode: define for mouse cursor, comment out for arrow keys.
#define IDRIVE_JOYSTICK_AS_MOUSE

// Touchpad data format version.
#define MOUSE_V1
// #define MOUSE_V2

// =============================================================================
// Timing Configuration (milliseconds)
// =============================================================================

// Interval between iDrive poll messages.
constexpr long kPollIntervalMs = 500;

// Duration of light-on state during initialization.
constexpr long kLightInitDurationMs = 1000;

// Interval between light keepalive messages.
constexpr long kLightKeepaliveIntervalMs = 10000;

// Cooldown period before controller is marked ready.
constexpr long kControllerCooldownMs = 750;

// Number of initial touchpad messages to ignore during init.
constexpr int kTouchpadInitIgnoreCount = 0;

// Minimum touchpad movement to register as input (deadzone).
constexpr int kMinMouseTravel = 5;

// Mouse movement step size for joystick input.
constexpr int kJoystickMoveStep = 30;

// =============================================================================
// Button Mapping Documentation
// =============================================================================
//
// Android-specific button mappings:
//   MENU button   -> Android Menu
//   BACK button   -> Android Back
//   OPTION button -> Media Play/Pause
//   RADIO button  -> Media Previous Track
//   CD button     -> Media Next Track
//   NAV button    -> Android Home
//   TEL button    -> Android Search
//   Rotary knob   -> Volume Up/Down
//   Touchpad      -> Mouse cursor movement
//   Joystick      -> Mouse movement or Arrow keys (configurable)
//   Center press  -> Mouse click or Enter key

// =============================================================================
// USB Device Configuration
// =============================================================================

// USB Vendor ID (Espressif default).
constexpr uint16_t kUsbVendorId = 0x303A;

// USB Product ID.
constexpr uint16_t kUsbProductId = 0x4002;

// USB device strings.
#define USB_MANUFACTURER "BMW"
#define USB_PRODUCT      "iDrive Controller"
#define USB_SERIAL_NUM   "123456"

// =============================================================================
// Debug Configuration
// =============================================================================

// Enable serial debug output.
#define SERIAL_DEBUG

// Enable CAN response logging.
#define DEBUG_CAN_RESPONSE

// Enable key press logging.
#define DEBUG_KEYS

// Enable touchpad data logging.
#define DEBUG_TOUCHPAD

// Enable logging for specific CAN ID.
#define DEBUG_SPECIFIC_CAN_ID
#define DEBUG_CAN_ID 0xBF

// CAN IDs to ignore in debug output are defined locally in idrive.cpp.

// =============================================================================
// Light Control Configuration
// =============================================================================

// Button that toggles backlight off (0 to disable).
#define LIGHT_OFF_BUTTON kButtonOption

// Auto-dim feature.
constexpr bool kAutoDimEnabled    = true;
constexpr long kAutoDimTimeoutMs  = 30000;  // Dim after 30 seconds of inactivity
constexpr int  kAutoDimBrightness = 50;     // Dimmed brightness percentage

// =============================================================================
// Legacy Compatibility Macros
// =============================================================================

#define iDriveJoystickAsMouse
#define iDrivePollTime          kPollIntervalMs
#define iDriveInitLightTime     kLightInitDurationMs
#define iDriveLightTime         kLightKeepaliveIntervalMs
#define controllerCoolDown      kControllerCooldownMs
#define TouchpadInitIgnoreCount kTouchpadInitIgnoreCount
#define min_mouse_travel        kMinMouseTravel
#define JOYSTICK_MOVE_STEP      kJoystickMoveStep

#endif  // BMW_IDRIVE_ESP32_SETTINGS_H_
