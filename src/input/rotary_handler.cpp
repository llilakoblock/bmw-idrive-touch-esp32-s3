// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT

#include "input/rotary_handler.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hid/hid_keycodes.h"

namespace idrive {

namespace {
const char* kTag = "ROTARY";
}

bool RotaryHandler::Handle(const InputEvent& event) {
    if (event.type != InputEvent::Type::Rotary) {
        return false;
    }

    if (!enabled_) {
        return true;  // Event consumed but not processed.
    }

    int16_t steps = event.delta;

    if (steps > 0) {
        for (int i = 0; i < steps; ++i) {
            ESP_LOGI(kTag, "Rotary right - Volume up");
            hid_.MediaKeyPressAndRelease(hid::media::kVolumeUp);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    } else if (steps < 0) {
        for (int i = 0; i < -steps; ++i) {
            ESP_LOGI(kTag, "Rotary left - Volume down");
            hid_.MediaKeyPressAndRelease(hid::media::kVolumeDown);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }

    return true;
}

}  // namespace idrive
