// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// Touchpad input handler - mouse cursor movement with tap gestures.
// Supports: single tap (click), tap-tap-hold (drag), two-finger tap (right-click)

#pragma once

#include <cstdint>

#include "input/input_handler.h"

namespace idrive {

class TouchpadHandler : public InputHandler {
public:
    TouchpadHandler(UsbHidDevice& hid, int min_travel = 5,
                    int x_multiplier = 10, int y_multiplier = 10);

    bool Handle(const InputEvent& event) override;

private:
    int min_travel_;
    int x_multiplier_;
    int y_multiplier_;

    // Single finger tracking
    int16_t prev_x_ = 0;
    int16_t prev_y_ = 0;
    bool tracking_ = false;

    // Two-finger tracking for gestures
    int16_t prev_x2_ = 0;
    int16_t prev_y2_ = 0;
    bool tracking_two_fingers_ = false;

    // =========================================================================
    // Tap gesture detection (laptop-style)
    // Timing constants are in config/config.h for easy tuning
    // =========================================================================

    // Tap state machine
    enum class TapState {
        Idle,              // No tap activity
        WaitingForSecondTap,  // Single tap detected, waiting for possible second tap
        Dragging           // Tap-tap-hold: dragging in progress
    };

    TapState tap_state_ = TapState::Idle;

    // Tap detection variables
    uint32_t touch_start_time_ = 0;        // When finger touched
    int16_t touch_start_x_ = 0;            // Where finger touched (for movement check)
    int16_t touch_start_y_ = 0;
    uint32_t last_tap_time_ = 0;           // When last tap occurred
    bool touch_moved_ = false;             // Did finger move significantly?

    // Two-finger tap detection
    uint32_t two_finger_start_time_ = 0;
    bool two_finger_tap_candidate_ = false;

    // Helper methods
    void HandleFingerDown(const InputEvent& event);
    void HandleFingerUp(const InputEvent& event);
    void HandleTwoFingerDown(const InputEvent& event);
    void HandleTwoFingerUp();
    uint32_t GetMillis() const;
};

}  // namespace idrive
