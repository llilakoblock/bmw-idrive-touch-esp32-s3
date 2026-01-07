// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT

#include "input/touchpad_handler.h"

#include <cmath>

#include "config/config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hid/hid_keycodes.h"
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

uint32_t TouchpadHandler::GetMillis() const {
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

void TouchpadHandler::HandleFingerDown(const InputEvent& event) {
    uint32_t now = GetMillis();

    // Record touch start
    touch_start_time_ = now;
    touch_start_x_ = event.x;
    touch_start_y_ = event.y;
    touch_moved_ = false;

    // Check if this is the second tap in a double-tap sequence
    if (tap_state_ == TapState::WaitingForSecondTap) {
        if (now - last_tap_time_ < config::kDoubleTapWindowMs) {
            // Second tap detected - enter drag mode
            tap_state_ = TapState::Dragging;
            hid_.MouseButtonPress(hid::mouse::kButtonLeft);
            ESP_LOGI(kTag, "Tap-drag started (tap-tap-hold)");
        } else {
            // Too slow - reset to idle
            tap_state_ = TapState::Idle;
        }
    }
}

void TouchpadHandler::HandleFingerUp(const InputEvent& event) {
    uint32_t now = GetMillis();
    uint32_t touch_duration = now - touch_start_time_;

    // Handle drag release
    if (tap_state_ == TapState::Dragging) {
        hid_.MouseButtonRelease(hid::mouse::kButtonLeft);
        tap_state_ = TapState::Idle;
        ESP_LOGI(kTag, "Tap-drag ended");
        return;
    }

    // Check if this was a valid tap (short duration, minimal movement)
    bool is_tap = (touch_duration < config::kTapMaxDurationMs) && !touch_moved_;

    if (is_tap) {
        if (tap_state_ == TapState::Idle) {
            // First tap - wait for possible second tap
            tap_state_ = TapState::WaitingForSecondTap;
            last_tap_time_ = now;
            ESP_LOGD(kTag, "Single tap detected, waiting for second tap...");

            // Send click immediately (responsive feel)
            // If user does tap-tap-hold, they'll get click + drag
            hid_.MouseClick(hid::mouse::kButtonLeft);
            ESP_LOGI(kTag, "Tap -> Left Click");
        } else if (tap_state_ == TapState::WaitingForSecondTap) {
            // This shouldn't happen (finger down would have caught it)
            tap_state_ = TapState::Idle;
        }
    } else {
        // Not a tap - reset state
        if (tap_state_ == TapState::WaitingForSecondTap) {
            // Timeout will handle this, or next touch
        }
        // Don't reset if dragging was handled above
    }
}

void TouchpadHandler::HandleTwoFingerDown(const InputEvent& event) {
    two_finger_start_time_ = GetMillis();
    two_finger_tap_candidate_ = true;
}

void TouchpadHandler::HandleTwoFingerUp() {
    if (!two_finger_tap_candidate_) {
        return;
    }

    uint32_t duration = GetMillis() - two_finger_start_time_;

    // Check if it was a quick two-finger tap (right click)
    if (duration < config::kTapMaxDurationMs) {
        hid_.MouseClick(hid::mouse::kButtonRight);
        ESP_LOGI(kTag, "Two-finger tap -> Right Click");
    }

    two_finger_tap_candidate_ = false;
}

bool TouchpadHandler::Handle(const InputEvent& event) {
    if (event.type != InputEvent::Type::Touchpad) {
        return false;
    }

    // =========================================================================
    // Finger removed
    // =========================================================================
    if (event.state == protocol::kTouchFingerRemoved) {
        // Handle tap detection on finger up
        if (tracking_two_fingers_) {
            HandleTwoFingerUp();
        } else if (tracking_) {
            HandleFingerUp(event);
        }

        tracking_ = false;
        tracking_two_fingers_ = false;
        ESP_LOGD(kTag, "Touchpad: finger(s) removed");
        return true;
    }

    // =========================================================================
    // Two-finger gesture (scroll or right-click tap)
    // =========================================================================
    if (event.two_fingers) {
        if (!tracking_two_fingers_) {
            // Start two-finger tracking
            prev_x_ = event.x;
            prev_y_ = event.y;
            prev_x2_ = event.x2;
            prev_y2_ = event.y2;
            tracking_two_fingers_ = true;
            tracking_ = false;

            HandleTwoFingerDown(event);
            ESP_LOGD(kTag, "Touchpad: two-finger gesture started");
            return true;
        }

        // Calculate average Y movement of both fingers for scrolling
        int16_t delta_y1 = event.y - prev_y_;
        int16_t delta_y2 = event.y2 - prev_y2_;
        int16_t avg_delta_y = (delta_y1 + delta_y2) / 2;

        // If significant movement, it's a scroll, not a tap
        if (std::abs(avg_delta_y) >= min_travel_ * 3) {
            two_finger_tap_candidate_ = false;  // Cancel tap candidate

            // Convert to scroll (negative = scroll down, positive = scroll up)
            int8_t scroll = utils::Constrain(
                avg_delta_y * config::kScrollMultiplier / 10, -127, 127);

            if (scroll != 0) {
                ESP_LOGD(kTag, "Touchpad scroll: %d", scroll);
                hid_.MouseScroll(scroll);
            }

            prev_y_ = event.y;
            prev_y2_ = event.y2;
        }

        prev_x_ = event.x;
        prev_x2_ = event.x2;
        return true;
    }

    // =========================================================================
    // Single finger - reset two-finger tracking
    // =========================================================================
    if (tracking_two_fingers_) {
        tracking_two_fingers_ = false;
        tracking_ = false;
    }

    // =========================================================================
    // Single finger - first touch
    // =========================================================================
    if (!tracking_) {
        prev_x_ = event.x;
        prev_y_ = event.y;
        tracking_ = true;

        HandleFingerDown(event);
        ESP_LOGD(kTag, "Touchpad: touch started at x=%d, y=%d", event.x, event.y);
        return true;
    }

    // =========================================================================
    // Single finger - movement
    // =========================================================================
    int16_t delta_x = event.x - prev_x_;
    int16_t delta_y = event.y - prev_y_;

    // Check if finger moved significantly (for tap detection)
    int16_t total_move_x = std::abs(event.x - touch_start_x_);
    int16_t total_move_y = std::abs(event.y - touch_start_y_);
    if (total_move_x > config::kTapMaxMovement || total_move_y > config::kTapMaxMovement) {
        touch_moved_ = true;

        // If we were waiting for second tap and user moved, cancel it
        if (tap_state_ == TapState::WaitingForSecondTap) {
            tap_state_ = TapState::Idle;
        }
    }

    // Apply threshold to avoid jitter
    if (std::abs(delta_x) < min_travel_) {
        delta_x = 0;
    }
    if (std::abs(delta_y) < min_travel_) {
        delta_y = 0;
    }

    if (delta_x != 0 || delta_y != 0) {
        // Scale movement for better feel
        int8_t mouse_x = utils::Constrain(
            delta_x * x_multiplier_ / 10, -127, 127);
        // Y-axis inverted (touchpad Y increases upward, screen Y increases downward)
        int8_t mouse_y = utils::Constrain(
            -delta_y * y_multiplier_ / 10, -127, 127);

        ESP_LOGD(kTag, "Touchpad move: x=%d, y=%d", mouse_x, mouse_y);
        hid_.MouseMove(mouse_x, mouse_y);

        prev_x_ = event.x;
        prev_y_ = event.y;
    }

    return true;
}

}  // namespace idrive
