// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// Global state variables for iDrive controller communication.

#ifndef BMW_IDRIVE_ESP32_VARIABLES_H_
#define BMW_IDRIVE_ESP32_VARIABLES_H_

#include <cstddef>
#include <cstdint>

// =============================================================================
// Initialization State Variables
// =============================================================================

// Timestamp of last periodic task execution (milliseconds).
extern unsigned long g_previous_millis;

// Timestamp when cooldown period started (milliseconds).
extern unsigned long g_cooldown_millis;

// Number of CAN initialization attempts.
extern int g_init_can_count;

// Maximum number of CAN initialization retry attempts.
extern const int kMaxCanInitAttempts;

// True when rotary encoder initialization response received.
extern bool g_rotary_init_success;

// True when polling has been initialized.
extern bool g_poll_init;

// True when touchpad initialization is complete.
extern bool g_touchpad_init_done;

// True when light initialization is complete.
extern bool g_light_init_done;

// True when initial rotary position has been set.
extern bool g_rotary_init_position_set;

// True when controller is fully ready for use.
extern bool g_controller_ready;

// True when touchpad is currently being touched.
extern bool g_touching;

// True to disable rotary encoder input processing.
extern bool g_rotary_disabled;

// =============================================================================
// Light Control
// =============================================================================

// True to enable iDrive backlight, false to disable.
extern bool g_idrive_light_on;

// =============================================================================
// Rotary Encoder State
// =============================================================================

// Current rotary encoder position (16-bit counter).
extern unsigned int g_rotary_position;

// =============================================================================
// Touchpad State
// =============================================================================

// Previous X coordinate for delta calculation.
extern int g_previous_x;

// Previous Y coordinate for delta calculation.
extern int g_previous_y;

// Calculated X movement result.
extern int g_result_x;

// Calculated Y movement result.
extern int g_result_y;

// Counter for ignoring initial touchpad messages during init.
extern int g_touchpad_init_ignore_counter;

// =============================================================================
// Mouse Range Constants (V1 - 8-bit signed)
// =============================================================================

extern const int8_t kMouseLowRange;
extern const int8_t kMouseHighRange;
extern const int8_t kMouseCenterRange;
extern const float kPowerValue;

// =============================================================================
// Mouse Range Constants (V2 - 16-bit unsigned)
// =============================================================================

extern const int kMouseLowRangeV2;
extern const int kMouseHighRangeV2;
extern const int kMouseCenterRangeV2;
extern const int kXMultiplier;
extern const int kYMultiplier;

// =============================================================================
// Input State Tracking
// =============================================================================

// Array tracking key states (indexed by key code).
extern uint8_t g_key_states[256];

// =============================================================================
// Legacy Variable Names (for compatibility)
// =============================================================================

#define previousMillis            g_previous_millis
#define CoolDownMillis            g_cooldown_millis
#define init_can_count            g_init_can_count
#define max_can_init_attempts     kMaxCanInitAttempts
#define RotaryInitSuccess         g_rotary_init_success
#define PollInit                  g_poll_init
#define TouchpadInitDone          g_touchpad_init_done
#define LightInitDone             g_light_init_done
#define RotaryInitPositionSet     g_rotary_init_position_set
#define controllerReady           g_controller_ready
#define touching                  g_touching
#define rotaryDisabled            g_rotary_disabled
#define iDriveLightOn             g_idrive_light_on
#define rotaryposition            g_rotary_position
#define PreviousX                 g_previous_x
#define PreviousY                 g_previous_y
#define ResultX                   g_result_x
#define ResultY                   g_result_y
#define TouchpadInitIgnoreCounter g_touchpad_init_ignore_counter
#define mouse_low_range           kMouseLowRange
#define mouse_high_range          kMouseHighRange
#define mouse_center_range        kMouseCenterRange
#define powerValue                kPowerValue
#define mouse_low_range_v2        kMouseLowRangeV2
#define mouse_high_range_v2       kMouseHighRangeV2
#define mouse_center_range_v2     kMouseCenterRangeV2
#define x_multiplier              kXMultiplier
#define y_multiplier              kYMultiplier
#define stati                     g_key_states

#endif  // BMW_IDRIVE_ESP32_VARIABLES_H_
