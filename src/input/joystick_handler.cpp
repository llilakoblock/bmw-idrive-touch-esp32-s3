// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT

#include "input/joystick_handler.h"

#include "esp_log.h"

#include "config/config.h"
#include "hid/hid_keycodes.h"

namespace idrive {

namespace {
const char *kTag = "JOYSTICK";
}

JoystickHandler::JoystickHandler(UsbHidDevice &hid, bool as_mouse, int move_step)
    : InputHandler(hid), as_mouse_(as_mouse), move_step_(move_step)
{}

bool JoystickHandler::Handle(const InputEvent &event)
{
    if (event.type != InputEvent::Type::Joystick) {
        return false;
    }

    uint8_t direction = event.id;
    uint8_t state     = event.state;

    if (as_mouse_) {
        // Joystick as mouse movement.
        if (state == protocol::kInputPressed || state == protocol::kInputHeld) {
            int8_t x = 0;
            int8_t y = 0;

            if (direction & protocol::kStickUp) {
                y = -move_step_;
            }
            if (direction & protocol::kStickDown) {
                y = move_step_;
            }
            if (direction & protocol::kStickLeft) {
                x = -move_step_;
            }
            if (direction & protocol::kStickRight) {
                x = move_step_;
            }

            if (x != 0 || y != 0) {
                ESP_LOGI(kTag, "Joystick move: x=%d, y=%d", x, y);
                hid_.MouseMove(x, y);
            }
        }

        // Center press as left click.
        if (direction == protocol::kStickCenter) {
            if (state == protocol::kInputPressed) {
                ESP_LOGI(kTag, "Joystick center pressed - left click");
                hid_.MouseButtonPress(hid::mouse::kButtonLeft);
            } else if (state == protocol::kInputReleased) {
                ESP_LOGI(kTag, "Joystick center released");
                hid_.MouseButtonRelease(hid::mouse::kButtonLeft);
            }
        }
    } else {
        // Joystick as arrow keys.
        uint8_t key = 0;

        if (direction & protocol::kStickUp) {
            key = hid::key::kUp;
        } else if (direction & protocol::kStickDown) {
            key = hid::key::kDown;
        } else if (direction & protocol::kStickLeft) {
            key = hid::key::kLeft;
        } else if (direction & protocol::kStickRight) {
            key = hid::key::kRight;
        } else if (direction == protocol::kStickCenter) {
            key = hid::key::kEnter;
        }

        if (key != 0) {
            if (state == protocol::kInputPressed) {
                ESP_LOGI(kTag, "Joystick arrow key pressed: 0x%02X", key);
                hid_.KeyPress(key);
            } else if (state == protocol::kInputReleased) {
                ESP_LOGI(kTag, "Joystick arrow key released: 0x%02X", key);
                hid_.KeyRelease(key);
            }
        }
    }

    return true;
}

}  // namespace idrive
