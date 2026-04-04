// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT

#include "input/rotary_handler.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "hid/hid_keycodes.h"

namespace idrive {

namespace {
const char *kTag = "ROTARY";
}

  // bool RotaryHandler::Handle(const InputEvent &event)
  // {
      // if (event.type != InputEvent::Type::Rotary) {
        //   return false;
    //   }
  //     if (!enabled_) {
       //    return true;  // Event consumed but not processed.
     //  }
    //   int16_t steps = event.delta;
    // Rotary encoder as mouse scroll wheel.
    // Volume/track controls are on steering wheel.
    //   if (steps != 0) {
        // Positive = scroll down, Negative = scroll up (natural scrolling)
      //     int8_t scroll = static_cast<int8_t>(-steps);  // Invert for natural feel
        //   ESP_LOGI(kTag, "Rotary scroll: %d steps -> wheel %d", steps, scroll);
        //   hid_.MouseScroll(scroll);
     //  }
    //   return true;
  // }

bool RotaryHandler::Handle(const InputEvent &event)
{
    if (event.type != InputEvent::Type::Rotary) {
        return false;
    }

    if (!enabled_) {
        return true;  // Event consumed but not processed.
    }

    int16_t steps = event.delta;

    // Rotary encoder as volume control.
    // Right turn (positive delta) = Volume Up
    // Left turn (negative delta)  = Volume Down
    if (steps > 0) {
        ESP_LOGI(kTag, "Rotary volume: %d step(s) -> Volume Up", steps);
        for (int16_t i = 0; i < steps; ++i) {
            hid_.MediaKeyPressAndRelease(hid::media::kVolumeUp);
        }
    } else if (steps < 0) {
        ESP_LOGI(kTag, "Rotary volume: %d step(s) -> Volume Down", steps);
        for (int16_t i = 0; i < -steps; ++i) {
            hid_.MediaKeyPressAndRelease(hid::media::kVolumeDown);
        }
    }

    return true;
}

}  // namespace idrive