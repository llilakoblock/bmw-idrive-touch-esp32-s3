// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// iDrive CAN bus communication interface.

#ifndef BMW_IDRIVE_ESP32_IDRIVE_H_
#define BMW_IDRIVE_ESP32_IDRIVE_H_

#include <stdint.h>

// =============================================================================
// CAN Message IDs - Incoming (from iDrive controller)
// =============================================================================

// Button and joystick input messages
constexpr uint32_t kMsgInInput = 0x267;

// Input types
constexpr uint8_t kInputTypeButton = 0xC0;
constexpr uint8_t kInputTypeStick  = 0xDD;
constexpr uint8_t kInputTypeCenter = 0xDE;

// Button identifiers
constexpr uint8_t kButtonMenu   = 0x01;
constexpr uint8_t kButtonBack   = 0x02;
constexpr uint8_t kButtonOption = 0x04;
constexpr uint8_t kButtonRadio  = 0x08;
constexpr uint8_t kButtonCd     = 0x10;
constexpr uint8_t kButtonNav    = 0x20;
constexpr uint8_t kButtonTel    = 0x40;

// Joystick directions
constexpr uint8_t kStickUp     = 0x01;
constexpr uint8_t kStickRight  = 0x02;
constexpr uint8_t kStickDown   = 0x04;
constexpr uint8_t kStickLeft   = 0x08;
constexpr uint8_t kStickCenter = 0x00;

// Input states
constexpr uint8_t kInputReleased = 0x00;
constexpr uint8_t kInputPressed  = 0x01;
constexpr uint8_t kInputHeld     = 0x02;

// Rotary encoder messages
constexpr uint32_t kMsgInRotary     = 0x264;
constexpr uint32_t kMsgInRotaryInit = 0x277;

// Status messages
constexpr uint32_t kMsgInStatus   = 0x5E7;
constexpr uint8_t  kStatusNoInit  = 0x06;

// Touchpad messages
constexpr uint32_t kMsgInTouch = 0xBF;

// Touch types
constexpr uint8_t kTouchFingerRemoved = 0x11;
constexpr uint8_t kTouchSingle        = 0x10;
constexpr uint8_t kTouchMulti         = 0x00;
constexpr uint8_t kTouchTriple        = 0x1F;
constexpr uint8_t kTouchQuad          = 0x0F;

// =============================================================================
// CAN Message IDs - Outgoing (to iDrive controller)
// =============================================================================

constexpr uint32_t kMsgOutRotaryInit = 0x273;
constexpr uint32_t kMsgOutLight      = 0x202;
constexpr uint32_t kMsgOutPoll       = 0x501;

// =============================================================================
// Public Functions
// =============================================================================

// Initializes the iDrive rotary encoder.
void IDriveInit();

// Initializes the iDrive touchpad.
void IDriveTouchpadInit();

// Sends periodic poll messages to keep iDrive active.
// Args:
//   interval_ms: Minimum interval between poll messages in milliseconds.
void IDrivePoll(unsigned long interval_ms);

// Sends periodic light keepalive messages.
// Args:
//   interval_ms: Minimum interval between light messages in milliseconds.
void IDriveLight(unsigned long interval_ms);

// Immediately sends a light control message.
void IDriveLightSend();

// Immediately sends a poll message.
void IDrivePollSend();

// Decodes incoming CAN bus messages and triggers appropriate actions.
// Args:
//   can_id: CAN message identifier.
//   length: Data length code.
//   data: Pointer to message data buffer (8 bytes max).
void DecodeCanMessage(unsigned long can_id, uint8_t length, uint8_t* data);

// Checks if a value exists in an array.
// Args:
//   value: Value to search for.
//   array: Array to search in.
//   size: Size of the array.
// Returns:
//   true if value is found, false otherwise.
bool IsValueInArray(int value, int* array, int size);

// =============================================================================
// Legacy Macro Definitions (for compatibility)
// =============================================================================

#define MSG_IN_INPUT        kMsgInInput
#define MSG_INPUT_BUTTON    kInputTypeButton
#define MSG_INPUT_BUTTON_MENU   kButtonMenu
#define MSG_INPUT_BUTTON_BACK   kButtonBack
#define MSG_INPUT_BUTTON_OPTION kButtonOption
#define MSG_INPUT_BUTTON_RADIO  kButtonRadio
#define MSG_INPUT_BUTTON_CD     kButtonCd
#define MSG_INPUT_BUTTON_NAV    kButtonNav
#define MSG_INPUT_BUTTON_TEL    kButtonTel
#define MSG_INPUT_CENTER    kInputTypeCenter
#define MSG_INPUT_STICK     kInputTypeStick
#define MSG_INPUT_STICK_UP      kStickUp
#define MSG_INPUT_STICK_RIGHT   kStickRight
#define MSG_INPUT_STICK_DOWN    kStickDown
#define MSG_INPUT_STICK_LEFT    kStickLeft
#define MSG_INPUT_STICK_CENTER  kStickCenter
#define MSG_INPUT_RELEASED  kInputReleased
#define MSG_INPUT_PRESSED   kInputPressed
#define MSG_INPUT_HELD      kInputHeld
#define MSG_IN_ROTARY       kMsgInRotary
#define MSG_IN_ROTARY_INIT  kMsgInRotaryInit
#define MSG_IN_STATUS       kMsgInStatus
#define MSG_STATUS_NO_INIT  kStatusNoInit
#define MSG_IN_TOUCH        kMsgInTouch
#define FINGER_REMOVED      kTouchFingerRemoved
#define SINGLE_TOUCH        kTouchSingle
#define MULTI_TOUCH         kTouchMulti
#define TRIPLE_TOUCH        kTouchTriple
#define QUAD_TOUCH          kTouchQuad
#define MSG_OUT_ROTARY_INIT kMsgOutRotaryInit
#define MSG_OUT_LIGHT       kMsgOutLight
#define MSG_OUT_POLL        kMsgOutPoll

#endif  // BMW_IDRIVE_ESP32_IDRIVE_H_
