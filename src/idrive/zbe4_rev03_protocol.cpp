// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// Protocol implementation for iDrive ZBE4 revision -03.
// Contributed by: Aleksandar Danilović (github.com/alexxxa399).

#include "idrive/zbe4_rev03_protocol.h"

#include "config/config.h"
#include "esp_log.h"

namespace idrive {

namespace {
const char *kTag = "ZBE4-03";

// Idle frame signature: bytes[3..7] all at rest.
constexpr uint8_t kIdleByte3 = 0x00;
constexpr uint8_t kIdleByte4 = 0x00;
constexpr uint8_t kIdleByte5 = 0x00;
constexpr uint8_t kIdleByte6 = 0xC0;
constexpr uint8_t kIdleByte7 = 0xF8;
}  // namespace

void ZBE4Rev03Protocol::OnDetected()
{
    init_done_ = true;
    ESP_LOGI(kTag, "Detected on 0x25B");
}

void ZBE4Rev03Protocol::Reset()
{
    init_done_        = false;
    position_set_     = false;
    last_button_id_   = 0;
    last_joystick_id_ = 0;
    joystick_active_  = false;
    menu_pressed_     = false;
    back_pressed_     = false;
}

void ZBE4Rev03Protocol::HandleButtonEdge(bool now, bool &previous, uint8_t button_id)
{
    if (now == previous)
        return;
    previous = now;

    if (config::kDebugKeys) {
        ESP_LOGI(kTag, "Button %s: 0x%02X", now ? "press" : "release", button_id);
    }

    InputEvent event;
    event.type  = InputEvent::Type::Button;
    event.id    = button_id;
    event.state = now ? protocol::kInputPressed : protocol::kInputReleased;
    Emit(event);
}

void ZBE4Rev03Protocol::OnMessage(const CanMessage &msg)
{
    if (msg.length < 8)
        return;

    // Position is always in bytes[1..2] (little-endian).
    uint16_t new_position = static_cast<uint16_t>(msg.data[1]) |
                            (static_cast<uint16_t>(msg.data[2]) << 8);

    // MENU and BACK are bitfields in byte[4] - handle edge detection first.
    // They can be pressed simultaneously (needed for OTA button combo).
    bool menu_now = (msg.data[4] & 0x04) != 0;
    bool back_now = (msg.data[4] & 0x20) != 0;
    HandleButtonEdge(menu_now, menu_pressed_, protocol::kButtonMenu);
    HandleButtonEdge(back_now, back_pressed_, protocol::kButtonBack);

    if (menu_now || back_now)
        return;

    // Idle frame: bytes[3..7] at rest values.
    bool idle = (msg.data[3] == kIdleByte3 && msg.data[4] == kIdleByte4 &&
                 msg.data[5] == kIdleByte5 && msg.data[6] == kIdleByte6 &&
                 msg.data[7] == kIdleByte7);

    if (!idle) {
        // Press frame: decode joystick (byte[3]) or button (bytes[5..7]).

        if (msg.data[3] != 0x00) {
            uint8_t joy_id = 0;
            bool    known  = true;

            switch (msg.data[3]) {
                case 0x01: joy_id = protocol::kStickCenter; break;
                case 0x10: joy_id = protocol::kStickUp;     break;
                case 0x70: joy_id = protocol::kStickDown;   break;
                case 0xA0: joy_id = protocol::kStickLeft;   break;
                case 0x40: joy_id = protocol::kStickRight;  break;
                default:   known  = false;                  break;
            }

            if (known) {
                last_joystick_id_ = joy_id;
                joystick_active_  = true;

                if (config::kDebugKeys) {
                    ESP_LOGI(kTag, "Joystick press: 0x%02X", joy_id);
                }

                InputEvent event;
                event.type  = InputEvent::Type::Joystick;
                event.id    = joy_id;
                event.state = protocol::kInputPressed;
                Emit(event);
                return;
            }
        }

        // Buttons encoded as bits relative to idle baseline.
        uint8_t button_id    = 0;
        bool    known_button = true;

        if      (msg.data[5] & 0x01) button_id = protocol::kButtonOption;
        else if (msg.data[5] & 0x08) button_id = protocol::kButtonTel;
        else if (msg.data[6] & 0x01) button_id = protocol::kButtonCd;
        else if (msg.data[6] & 0x08) button_id = protocol::kButtonNav;
        else if (msg.data[7] & 0x01) button_id = protocol::kButtonMap;
        else                         known_button = false;

        if (known_button && button_id != 0) {
            last_button_id_ = button_id;

            if (config::kDebugKeys) {
                ESP_LOGI(kTag, "Button press: 0x%02X", button_id);
            }

            InputEvent event;
            event.type  = InputEvent::Type::Button;
            event.id    = button_id;
            event.state = protocol::kInputPressed;
            Emit(event);
            return;
        }

        if (config::kDebugKeys) {
            ESP_LOGW(kTag, "Unknown press frame: [%02X %02X %02X %02X %02X]",
                     msg.data[3], msg.data[4], msg.data[5], msg.data[6], msg.data[7]);
        }
        return;
    }

    // Idle frame: emit pending releases, then check for rotary movement.

    if (joystick_active_) {
        if (config::kDebugKeys) {
            ESP_LOGI(kTag, "Joystick release: 0x%02X", last_joystick_id_);
        }
        InputEvent event;
        event.type  = InputEvent::Type::Joystick;
        event.id    = last_joystick_id_;
        event.state = protocol::kInputReleased;
        Emit(event);
        joystick_active_  = false;
        last_joystick_id_ = 0;
        return;
    }

    if (last_button_id_ != 0) {
        if (config::kDebugKeys) {
            ESP_LOGI(kTag, "Button release: 0x%02X", last_button_id_);
        }
        InputEvent event;
        event.type  = InputEvent::Type::Button;
        event.id    = last_button_id_;
        event.state = protocol::kInputReleased;
        Emit(event);
        last_button_id_ = 0;
        return;
    }

    // Rotary: idle frame where byte[1] position changed.
    if (!position_set_) {
        position_     = new_position;
        position_set_ = true;
        ESP_LOGI(kTag, "Rotary initial position: 0x%04X", position_);
        return;
    }

    int delta = static_cast<int>(new_position) - static_cast<int>(position_);

    if (delta > 32768)
        delta -= 65536;
    else if (delta < -32768)
        delta += 65536;

    if (delta != 0) {
        if (config::kDebugKeys) {
            ESP_LOGI(kTag, "Rotary: pos=0x%04X delta=%d", new_position, delta);
        }
        InputEvent event;
        event.type  = InputEvent::Type::Rotary;
        event.delta = delta;
        Emit(event);
        position_ = new_position;
    }
}

}  // namespace idrive
