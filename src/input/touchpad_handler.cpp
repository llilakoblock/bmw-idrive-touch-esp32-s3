// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT

#include "input/touchpad_handler.h"

#include <cmath>

#include "config/config.h"
#include "esp_log.h"
#include "utils/utils.h"

namespace idrive {

namespace {
const char* kTag = "TOUCHPAD";
}

TouchpadHandler::TouchpadHandler(UsbHidDevice& hid, int min_travel,
                                 int x_multiplier, int y_multiplier)
    : InputHandler(hid),
      min_travel_(min_travel),
      x_multiplier_(x_multiplier),
      y_multiplier_(y_multiplier) {}

bool TouchpadHandler::Handle(const InputEvent& event) {
    if (event.type != InputEvent::Type::Touchpad) {
        return false;
    }

    // Check for finger removed (state indicates touch type).
    if (event.state == protocol::kTouchFingerRemoved) {
        tracking_ = false;
        ESP_LOGI(kTag, "Touchpad: finger removed");
        return true;
    }

    if (!tracking_) {
        // First touch - save initial position.
        prev_x_ = event.x;
        prev_y_ = event.y;
        tracking_ = true;
        ESP_LOGI(kTag, "Touchpad: touch started at x=%d, y=%d", event.x, event.y);
        return true;
    }

    // Calculate movement delta.
    int16_t delta_x = event.x - prev_x_;
    int16_t delta_y = event.y - prev_y_;

    // Apply threshold to avoid jitter.
    if (std::abs(delta_x) < min_travel_) {
        delta_x = 0;
    }
    if (std::abs(delta_y) < min_travel_) {
        delta_y = 0;
    }

    if (delta_x != 0 || delta_y != 0) {
        // Scale movement for better feel.
        int8_t mouse_x = utils::Constrain(
            delta_x * x_multiplier_ / 10, -127, 127);
        // Y-axis inverted (touchpad Y increases downward, screen Y increases upward).
        int8_t mouse_y = utils::Constrain(
            -delta_y * y_multiplier_ / 10, -127, 127);

        ESP_LOGI(kTag, "Touchpad move: x=%d, y=%d (delta: %d, %d)",
                 mouse_x, mouse_y, delta_x, delta_y);
        hid_.MouseMove(mouse_x, mouse_y);

        prev_x_ = event.x;
        prev_y_ = event.y;
    }

    return true;
}

}  // namespace idrive
