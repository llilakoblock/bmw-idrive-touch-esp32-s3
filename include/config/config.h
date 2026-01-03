// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// Configuration constants for iDrive controller adapter.

#pragma once

#include <cstdint>

namespace idrive {

// =============================================================================
// Runtime Configuration
// =============================================================================

struct Config {
    bool joystick_as_mouse = true;
    uint8_t light_brightness = 100;
    uint32_t poll_interval_ms = 500;
    uint32_t light_keepalive_ms = 10000;
    int min_mouse_travel = 5;
    int joystick_move_step = 30;
};

// =============================================================================
// Compile-time Constants
// =============================================================================

namespace config {

// USB Device Configuration
constexpr uint16_t kUsbVendorId = 0x303A;  // Espressif VID
constexpr uint16_t kUsbProductId = 0x4002;
constexpr const char* kUsbManufacturer = "llilakoblock";
constexpr const char* kUsbProduct = "BMW iDrive Touch Adapter";
constexpr const char* kUsbSerialNumber = "123456";

// CAN Bus Configuration
constexpr uint32_t kCanBaudrate = 500000;

// Timing Configuration (milliseconds)
constexpr uint32_t kPollIntervalMs = 500;
constexpr uint32_t kLightInitDurationMs = 1000;
constexpr uint32_t kLightKeepaliveMs = 10000;
constexpr uint32_t kControllerCooldownMs = 750;
constexpr uint32_t kInitRetryIntervalMs = 5000;

// Input Configuration
constexpr int kMinMouseTravel = 5;
constexpr int kJoystickMoveStep = 30;
constexpr int kTouchpadInitIgnoreCount = 0;
constexpr int kXMultiplier = 10;
constexpr int kYMultiplier = 10;

// Debug Configuration
constexpr bool kSerialDebug = true;
constexpr bool kDebugCan = false;
constexpr bool kDebugKeys = true;
constexpr bool kDebugTouchpad = true;

}  // namespace config

// =============================================================================
// CAN Protocol Constants
// =============================================================================

namespace can_id {

// Incoming messages (from iDrive controller)
constexpr uint32_t kInput = 0x267;       // Button and joystick input
constexpr uint32_t kRotary = 0x264;      // Rotary encoder data
constexpr uint32_t kRotaryInit = 0x277;  // Rotary initialization response
constexpr uint32_t kStatus = 0x5E7;      // Status messages
constexpr uint32_t kTouch = 0xBF;        // Touchpad data

// Outgoing messages (to iDrive controller)
constexpr uint32_t kRotaryInitCmd = 0x273;  // Rotary initialization command
constexpr uint32_t kLight = 0x202;          // Light control
constexpr uint32_t kPoll = 0x501;           // Keepalive poll

}  // namespace can_id

// =============================================================================
// iDrive Protocol Constants
// =============================================================================

namespace protocol {

// Input types
constexpr uint8_t kInputTypeButton = 0xC0;
constexpr uint8_t kInputTypeStick = 0xDD;
constexpr uint8_t kInputTypeCenter = 0xDE;

// Button identifiers
constexpr uint8_t kButtonMenu = 0x01;
constexpr uint8_t kButtonBack = 0x02;
constexpr uint8_t kButtonOption = 0x04;
constexpr uint8_t kButtonRadio = 0x08;
constexpr uint8_t kButtonCd = 0x10;
constexpr uint8_t kButtonNav = 0x20;
constexpr uint8_t kButtonTel = 0x40;

// Joystick directions
constexpr uint8_t kStickUp = 0x01;
constexpr uint8_t kStickRight = 0x02;
constexpr uint8_t kStickDown = 0x04;
constexpr uint8_t kStickLeft = 0x08;
constexpr uint8_t kStickCenter = 0x00;

// Input states
constexpr uint8_t kInputReleased = 0x00;
constexpr uint8_t kInputPressed = 0x01;
constexpr uint8_t kInputHeld = 0x02;

// Status codes
constexpr uint8_t kStatusNoInit = 0x06;

// Touch types
constexpr uint8_t kTouchFingerRemoved = 0x11;
constexpr uint8_t kTouchSingle = 0x10;
constexpr uint8_t kTouchMulti = 0x00;
constexpr uint8_t kTouchTriple = 0x1F;
constexpr uint8_t kTouchQuad = 0x0F;

}  // namespace protocol

}  // namespace idrive
