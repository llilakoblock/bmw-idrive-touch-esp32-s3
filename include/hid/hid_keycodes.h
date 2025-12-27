// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// USB HID key codes for keyboard, mouse, and consumer controls.

#pragma once

#include <cstdint>

namespace idrive::hid {

// =============================================================================
// Standard Keyboard Key Codes (USB HID Usage Table - Page 0x07)
// =============================================================================

namespace key {

// Letters
constexpr uint8_t kA = 0x04;
constexpr uint8_t kB = 0x05;
constexpr uint8_t kC = 0x06;
constexpr uint8_t kD = 0x07;
constexpr uint8_t kE = 0x08;
constexpr uint8_t kF = 0x09;
constexpr uint8_t kG = 0x0A;
constexpr uint8_t kH = 0x0B;
constexpr uint8_t kI = 0x0C;
constexpr uint8_t kJ = 0x0D;
constexpr uint8_t kK = 0x0E;
constexpr uint8_t kL = 0x0F;
constexpr uint8_t kM = 0x10;
constexpr uint8_t kN = 0x11;
constexpr uint8_t kO = 0x12;
constexpr uint8_t kP = 0x13;
constexpr uint8_t kQ = 0x14;
constexpr uint8_t kR = 0x15;
constexpr uint8_t kS = 0x16;
constexpr uint8_t kT = 0x17;
constexpr uint8_t kU = 0x18;
constexpr uint8_t kV = 0x19;
constexpr uint8_t kW = 0x1A;
constexpr uint8_t kX = 0x1B;
constexpr uint8_t kY = 0x1C;
constexpr uint8_t kZ = 0x1D;

// Numbers
constexpr uint8_t k1 = 0x1E;
constexpr uint8_t k2 = 0x1F;
constexpr uint8_t k3 = 0x20;
constexpr uint8_t k4 = 0x21;
constexpr uint8_t k5 = 0x22;
constexpr uint8_t k6 = 0x23;
constexpr uint8_t k7 = 0x24;
constexpr uint8_t k8 = 0x25;
constexpr uint8_t k9 = 0x26;
constexpr uint8_t k0 = 0x27;

// Special keys
constexpr uint8_t kEnter = 0x28;
constexpr uint8_t kEscape = 0x29;
constexpr uint8_t kBackspace = 0x2A;
constexpr uint8_t kTab = 0x2B;
constexpr uint8_t kSpace = 0x2C;

// Function keys
constexpr uint8_t kF1 = 0x3A;
constexpr uint8_t kF2 = 0x3B;
constexpr uint8_t kF3 = 0x3C;
constexpr uint8_t kF4 = 0x3D;
constexpr uint8_t kF5 = 0x3E;
constexpr uint8_t kF6 = 0x3F;
constexpr uint8_t kF7 = 0x40;
constexpr uint8_t kF8 = 0x41;
constexpr uint8_t kF9 = 0x42;
constexpr uint8_t kF10 = 0x43;
constexpr uint8_t kF11 = 0x44;
constexpr uint8_t kF12 = 0x45;

// Arrow keys
constexpr uint8_t kRight = 0x4F;
constexpr uint8_t kLeft = 0x50;
constexpr uint8_t kDown = 0x51;
constexpr uint8_t kUp = 0x52;

}  // namespace key

// =============================================================================
// Media Control Key Codes (USB HID Consumer Page - Page 0x0C)
// =============================================================================

namespace media {

constexpr uint16_t kPlayPause = 0x00CD;
constexpr uint16_t kStop = 0x00B7;
constexpr uint16_t kNextTrack = 0x00B5;
constexpr uint16_t kPrevTrack = 0x00B6;
constexpr uint16_t kVolumeUp = 0x00E9;
constexpr uint16_t kVolumeDown = 0x00EA;
constexpr uint16_t kMute = 0x00E2;
constexpr uint16_t kBassBoost = 0x00E5;
constexpr uint16_t kLoudness = 0x00E7;
constexpr uint16_t kBassUp = 0x0152;
constexpr uint16_t kBassDown = 0x0153;
constexpr uint16_t kTrebleUp = 0x0154;
constexpr uint16_t kTrebleDown = 0x0155;

}  // namespace media

// =============================================================================
// Android-Specific Key Codes (USB HID Consumer Page)
// =============================================================================

namespace android {

constexpr uint16_t kBack = 0x0224;    // AC Back
constexpr uint16_t kHome = 0x0223;    // AC Home
constexpr uint16_t kMenu = 0x0040;    // Menu key
constexpr uint16_t kSearch = 0x0221;  // AC Search

}  // namespace android

// =============================================================================
// Mouse Button Definitions
// =============================================================================

namespace mouse {

constexpr uint8_t kButtonLeft = 0x01;
constexpr uint8_t kButtonRight = 0x02;
constexpr uint8_t kButtonMiddle = 0x04;

}  // namespace mouse

}  // namespace idrive::hid
