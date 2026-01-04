// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT

#include "ota/ota_trigger.h"

#include "esp_log.h"
#include "ota/ota_config.h"
#include "utils/utils.h"

namespace idrive::ota {

namespace {
const char* kTag = "OTA_TRIGGER";
}

void OtaTrigger::SetCallback(OtaTriggerCallback callback) {
    callback_ = std::move(callback);
}

void OtaTrigger::OnButtonEvent(uint8_t button_id, uint8_t state) {
    // Track Menu button (0x01).
    if (button_id == config::kTriggerButton1) {
        menu_held_ = (state == 0x01 || state == 0x02);  // Pressed or Held
    }
    // Track Back button (0x02).
    if (button_id == config::kTriggerButton2) {
        back_held_ = (state == 0x01 || state == 0x02);  // Pressed or Held
    }

    // Check if both buttons are held.
    bool both_held = menu_held_ && back_held_;

    if (both_held && !detecting_) {
        // Start timing.
        detecting_ = true;
        combo_start_time_ = utils::GetMillis();
        ESP_LOGI(kTag, "OTA trigger combo detected - hold for %lu ms",
                 config::kTriggerHoldTimeMs);
    } else if (!both_held && detecting_) {
        // Combo broken.
        detecting_ = false;
        ESP_LOGI(kTag, "OTA trigger combo released");
    }
}

void OtaTrigger::Update() {
    if (detecting_ && !triggered_) {
        uint32_t elapsed = utils::GetMillis() - combo_start_time_;
        if (elapsed >= config::kTriggerHoldTimeMs) {
            triggered_ = true;
            detecting_ = false;
            ESP_LOGI(kTag, "OTA trigger activated!");
            if (callback_) {
                callback_();
            }
        }
    }
}

}  // namespace idrive::ota
