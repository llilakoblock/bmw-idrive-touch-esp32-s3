// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT

#include "input/button_handler.h"

#include "config/config.h"
#include "esp_log.h"
#include "hid/hid_keycodes.h"

namespace idrive {

namespace {
const char* kTag = "BUTTON";
}

bool ButtonHandler::Handle(const InputEvent& event) {
    if (event.type != InputEvent::Type::Button) {
        return false;
    }

    // Map iDrive buttons to Android media keys.
    uint16_t media_key = 0;

    switch (event.id) {
        case protocol::kButtonMenu:
            media_key = hid::android::kMenu;
            break;
        case protocol::kButtonBack:
            media_key = hid::android::kBack;
            break;
        case protocol::kButtonOption:
            media_key = hid::media::kPlayPause;
            break;
        case protocol::kButtonRadio:
            media_key = hid::media::kPrevTrack;
            break;
        case protocol::kButtonCd:
            media_key = hid::media::kNextTrack;
            break;
        case protocol::kButtonNav:
            media_key = hid::android::kHome;
            break;
        case protocol::kButtonTel:
            media_key = hid::android::kSearch;
            break;
        default:
            return false;
    }

    if (media_key == 0) {
        return false;
    }

    if (event.state == protocol::kInputPressed) {
        ESP_LOGI(kTag, "Button pressed: 0x%02X -> Media key: 0x%04X",
                 event.id, media_key);
        hid_.MediaKeyPress(media_key);
    } else if (event.state == protocol::kInputReleased) {
        ESP_LOGI(kTag, "Button released: 0x%02X", event.id);
        hid_.MediaKeyRelease(media_key);
    }

    return true;
}

}  // namespace idrive
