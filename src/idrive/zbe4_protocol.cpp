// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT

#include "idrive/zbe4_protocol.h"

#include "config/config.h"
#include "esp_log.h"

namespace idrive {

namespace {
const char *kTag = "ZBE4";
}

bool ZBE4Protocol::HandlesId(uint32_t id) const
{
    return id == can_id::kInput || id == can_id::kRotary;
}

void ZBE4Protocol::OnDetected()
{
    init_done_ = true;
    ESP_LOGI(kTag, "Init response received");
}

void ZBE4Protocol::Reset()
{
    init_done_    = false;
    position_set_ = false;
}

void ZBE4Protocol::OnMessage(const CanMessage &msg)
{
    switch (msg.id) {
        case can_id::kInput:
            HandleInputMessage(msg);
            break;
        case can_id::kRotary:
            HandleRotaryMessage(msg);
            break;
    }
}

void ZBE4Protocol::HandleInputMessage(const CanMessage &msg)
{
    if (msg.length < 6)
        return;

    uint8_t state      = msg.data[3] & 0x0F;
    uint8_t input_type = msg.data[4];
    uint8_t input      = msg.data[5];

    if (config::kDebugKeys) {
        ESP_LOGI(kTag, "Input: type=0x%02X id=0x%02X state=%d", input_type, input, state);
    }

    InputEvent event;

    if (input_type == protocol::kInputTypeButton) {
        event.type  = InputEvent::Type::Button;
        event.id    = input;
        event.state = state;
    } else if (input_type == protocol::kInputTypeStick) {
        event.type  = InputEvent::Type::Joystick;
        event.id    = msg.data[3] >> 4;
        event.state = state;
    } else if (input_type == protocol::kInputTypeCenter) {
        event.type  = InputEvent::Type::Joystick;
        event.id    = protocol::kStickCenter;
        event.state = state;
    } else {
        return;
    }

    Emit(event);
}

void ZBE4Protocol::HandleRotaryMessage(const CanMessage &msg)
{
    if (msg.length < 5)
        return;

    if (!position_set_) {
        uint8_t  step_b  = msg.data[4];
        uint32_t new_pos = msg.data[3] + (msg.data[4] * 0x100);

        switch (step_b) {
            case 0x7F: rotary_position_ = new_pos + 1; break;
            case 0x80: rotary_position_ = new_pos - 1; break;
            default:   rotary_position_ = new_pos;     break;
        }
        position_set_ = true;
        ESP_LOGI(kTag, "Rotary initial position: %lu", rotary_position_);
        return;
    }

    uint32_t new_position = msg.data[3] + (msg.data[4] * 0x100);
    int      delta        = static_cast<int>(new_position) - static_cast<int>(rotary_position_);

    if (delta > 32768)
        delta -= 65536;
    else if (delta < -32768)
        delta += 65536;

    if (delta != 0) {
        InputEvent event;
        event.type  = InputEvent::Type::Rotary;
        event.delta = delta;
        Emit(event);
        rotary_position_ = new_position;
    }
}

}  // namespace idrive
