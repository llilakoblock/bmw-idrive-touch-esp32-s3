// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// TinyUSB configuration for ESP32-S3 HID device.

#ifndef BMW_IDRIVE_ESP32_TUSB_CONFIG_H_
#define BMW_IDRIVE_ESP32_TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Board Configuration
// =============================================================================

#define CFG_TUSB_MCU OPT_MCU_ESP32S3

// RHPort number used for device (default to port 0).
#define BOARD_TUD_RHPORT 0

// RHPort max operational speed.
#define BOARD_TUD_MAX_SPEED OPT_MODE_DEFAULT_SPEED

// =============================================================================
// Common Configuration
// =============================================================================

#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE | OPT_MODE_DEFAULT_SPEED)
#define CFG_TUSB_OS           OPT_OS_FREERTOS

// Debug level (0 = off).
#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG 0
#endif

// Memory section configuration.
#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif

#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))
#endif

// =============================================================================
// Device Configuration
// =============================================================================

// Endpoint 0 max packet size.
#define CFG_TUD_ENDPOINT0_SIZE 64

// =============================================================================
// Class Driver Configuration
// =============================================================================

#define CFG_TUD_HID    1  // Enable HID class
#define CFG_TUD_CDC    0  // Disable CDC class
#define CFG_TUD_MSC    0  // Disable MSC class
#define CFG_TUD_MIDI   0  // Disable MIDI class
#define CFG_TUD_VENDOR 0  // Disable Vendor class

// HID endpoint buffer size.
#define CFG_TUD_HID_EP_BUFSIZE 64

#ifdef __cplusplus
}
#endif

#endif  // BMW_IDRIVE_ESP32_TUSB_CONFIG_H_
