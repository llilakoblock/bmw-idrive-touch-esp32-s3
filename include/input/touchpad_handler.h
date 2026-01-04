// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// Touchpad input handler - mouse cursor movement.

#pragma once

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
};

}  // namespace idrive
