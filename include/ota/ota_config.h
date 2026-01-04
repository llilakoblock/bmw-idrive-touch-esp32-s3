// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// OTA configuration constants.

#pragma once

#include <cstdint>

namespace idrive::ota::config {

// WiFi AP Configuration
constexpr const char* kApSsid = "iDrive-OTA";
constexpr const char* kApPassword = "idrive2024";
constexpr uint8_t kApChannel = 1;
constexpr uint8_t kApMaxConnections = 2;

// Trigger Configuration
constexpr uint32_t kTriggerHoldTimeMs = 3000;  // 3 seconds
constexpr uint8_t kTriggerButton1 = 0x01;      // Menu button
constexpr uint8_t kTriggerButton2 = 0x02;      // Back button

// HTTP Server Configuration
constexpr uint16_t kHttpPort = 80;
constexpr size_t kUploadBufferSize = 4096;

// OTA Configuration
constexpr size_t kMaxFirmwareSize = 0x3F0000;  // ~3.9MB per partition (8MB flash)

// Debug
constexpr bool kDebugOta = true;

}  // namespace idrive::ota::config
