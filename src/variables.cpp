// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// Global state variable definitions.

#include "variables.h"

#include <cstddef>

// =============================================================================
// Initialization State Variables
// =============================================================================

unsigned long g_previous_millis       = 0;
unsigned long g_cooldown_millis       = 0;
int           g_init_can_count        = 0;
const int     kMaxCanInitAttempts     = 15;
bool          g_rotary_init_success   = false;
bool          g_poll_init             = false;
bool          g_touchpad_init_done    = false;
bool          g_light_init_done       = false;
bool          g_rotary_init_position_set = false;
bool          g_controller_ready      = false;
bool          g_touching              = false;
bool          g_rotary_disabled       = false;

// =============================================================================
// Light Control
// =============================================================================

bool g_idrive_light_on = false;

// =============================================================================
// Rotary Encoder State
// =============================================================================

unsigned int g_rotary_position = 0;

// =============================================================================
// Touchpad State
// =============================================================================

int g_previous_x                 = 0;
int g_previous_y                 = 0;
int g_result_x                   = 0;
int g_result_y                   = 0;
int g_touchpad_init_ignore_counter = 0;

// =============================================================================
// Mouse Range Constants (V1 - 8-bit signed)
// =============================================================================

const int8_t kMouseLowRange    = -128;
const int8_t kMouseHighRange   = 127;
const int8_t kMouseCenterRange = 0;
const float  kPowerValue       = 1.4f;

// =============================================================================
// Mouse Range Constants (V2 - 16-bit unsigned)
// =============================================================================

const int kMouseLowRangeV2    = 0;
const int kMouseHighRangeV2   = 510;
const int kMouseCenterRangeV2 = kMouseHighRangeV2 / 2;
const int kXMultiplier        = 2;
const int kYMultiplier        = 12;

// =============================================================================
// Input State Tracking
// =============================================================================

uint8_t g_key_states[256] = {0};
