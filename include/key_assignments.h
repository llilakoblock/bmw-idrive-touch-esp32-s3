// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// HID keyboard and mouse key code definitions.

#ifndef BMW_IDRIVE_ESP32_KEY_ASSIGNMENTS_H_
#define BMW_IDRIVE_ESP32_KEY_ASSIGNMENTS_H_

#include <cstdint>

// =============================================================================
// Standard Keyboard Key Codes (USB HID Usage Table)
// =============================================================================

constexpr uint8_t kHidKeyA     = 0x04;
constexpr uint8_t kHidKeyB     = 0x05;
constexpr uint8_t kHidKeyC     = 0x06;
constexpr uint8_t kHidKeyD     = 0x07;
constexpr uint8_t kHidKeyE     = 0x08;
constexpr uint8_t kHidKeyF     = 0x09;
constexpr uint8_t kHidKeyL     = 0x0F;
constexpr uint8_t kHidKeyM     = 0x10;
constexpr uint8_t kHidKeyN     = 0x11;
constexpr uint8_t kHidKeyO     = 0x12;
constexpr uint8_t kHidKeyR     = 0x15;
constexpr uint8_t kHidKeyS     = 0x16;
constexpr uint8_t kHidKeyT     = 0x17;
constexpr uint8_t kHidKeyU     = 0x18;

constexpr uint8_t kHidKeyEnter       = 0x28;
constexpr uint8_t kHidKeyEscape      = 0x29;
constexpr uint8_t kHidKeyBackspace   = 0x2A;
constexpr uint8_t kHidKeyArrowRight  = 0x4F;
constexpr uint8_t kHidKeyArrowLeft   = 0x50;
constexpr uint8_t kHidKeyArrowDown   = 0x51;
constexpr uint8_t kHidKeyArrowUp     = 0x52;

// Volume keys (Consumer Page).
constexpr uint8_t kHidKeyVolumeUp   = 0x80;
constexpr uint8_t kHidKeyVolumeDown = 0x81;

// =============================================================================
// iDrive Button to Keyboard Key Mapping
// =============================================================================

constexpr uint8_t kKeyMenuKb   = kHidKeyM;
constexpr uint8_t kKeyBackKb   = kHidKeyB;
constexpr uint8_t kKeyOptionKb = kHidKeyO;
constexpr uint8_t kKeyRadioKb  = kHidKeyR;
constexpr uint8_t kKeyCdKb     = kHidKeyC;
constexpr uint8_t kKeyNavKb    = kHidKeyN;
constexpr uint8_t kKeyTelKb    = kHidKeyT;

// =============================================================================
// Joystick Direction to Keyboard Key Mapping
// =============================================================================

constexpr uint8_t kKeyCenterKb = kHidKeyS;
constexpr uint8_t kKeyUpKb     = kHidKeyU;
constexpr uint8_t kKeyDownKb   = kHidKeyD;
constexpr uint8_t kKeyLeftKb   = kHidKeyL;
constexpr uint8_t kKeyRightKb  = kHidKeyR;

// =============================================================================
// Rotary Encoder Key Mapping
// =============================================================================

constexpr uint8_t kKeyRotatePlusKb  = kHidKeyVolumeUp;
constexpr uint8_t kKeyRotateMinusKb = kHidKeyVolumeDown;

// =============================================================================
// Mouse Button Definitions
// =============================================================================

constexpr uint8_t kMouseButtonLeft   = 0x01;
constexpr uint8_t kMouseButtonRight  = 0x02;
constexpr uint8_t kMouseButtonMiddle = 0x04;

// =============================================================================
// Legacy Macro Definitions (for compatibility)
// =============================================================================

#define HID_KEY_A           kHidKeyA
#define HID_KEY_B           kHidKeyB
#define HID_KEY_C           kHidKeyC
#define HID_KEY_D           kHidKeyD
#define HID_KEY_E           kHidKeyE
#define HID_KEY_F           kHidKeyF
#define HID_KEY_M           kHidKeyM
#define HID_KEY_N           kHidKeyN
#define HID_KEY_O           kHidKeyO
#define HID_KEY_R           kHidKeyR
#define HID_KEY_S           kHidKeyS
#define HID_KEY_T           kHidKeyT
#define HID_KEY_U           kHidKeyU
#define HID_KEY_DD          kHidKeyD
#define HID_KEY_L           kHidKeyL
#define HID_KEY_ENTER       kHidKeyEnter
#define HID_KEY_ESC         kHidKeyEscape
#define HID_KEY_BSPACE      kHidKeyBackspace
#define HID_KEY_LEFT_ARROW  kHidKeyArrowLeft
#define HID_KEY_RIGHT_ARROW kHidKeyArrowRight
#define HID_KEY_UP_ARROW    kHidKeyArrowUp
#define HID_KEY_DOWN_ARROW  kHidKeyArrowDown
#define HID_KEY_VOLUP       kHidKeyVolumeUp
#define HID_KEY_VOLDOWN     kHidKeyVolumeDown

#define KEY_MENU_KB         kKeyMenuKb
#define KEY_BACK_KB         kKeyBackKb
#define KEY_OPTION_KB       kKeyOptionKb
#define KEY_RADIO_KB        kKeyRadioKb
#define KEY_CD_KB           kKeyCdKb
#define KEY_NAV_KB          kKeyNavKb
#define KEY_TEL_KB          kKeyTelKb
#define KEY_CENTER_KB       kKeyCenterKb
#define KEY_UP_KB           kKeyUpKb
#define KEY_DOWN_KB         kKeyDownKb
#define KEY_LEFT_KB         kKeyLeftKb
#define KEY_RIGHT_KB        kKeyRightKb
#define KEY_ROTATE_PLUS_KB  kKeyRotatePlusKb
#define KEY_ROTATE_MINUS_KB kKeyRotateMinusKb

#define MOUSE_BTN_LEFT   kMouseButtonLeft
#define MOUSE_BTN_RIGHT  kMouseButtonRight
#define MOUSE_BTN_MIDDLE kMouseButtonMiddle

#endif  // BMW_IDRIVE_ESP32_KEY_ASSIGNMENTS_H_
