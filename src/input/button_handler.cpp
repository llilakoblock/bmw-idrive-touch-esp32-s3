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

    // Map iDrive buttons to HID keys.
    // Native Android keys: Back, Home, AL Phone, AL Music Player
    // Key Mapper keys: NAV, OPTION, RADIO (obscure codes for custom remapping)
    uint16_t media_key = 0;

    switch (event.id) {
        case protocol::kButtonMenu:
            // MENU → Android Home (native)
            media_key = hid::android::kHome;
            break;
        case protocol::kButtonBack:
            // BACK → Android Back (native)
            media_key = hid::android::kBack;
            break;
        case protocol::kButtonOption:
            // OPTION → Key Mapper custom (for user remapping)
            media_key = hid::keymapper::kCustom2;
            break;
        case protocol::kButtonRadio:
            // RADIO → Key Mapper custom (for user remapping)
            media_key = hid::keymapper::kCustom3;
            break;
        case protocol::kButtonCd:
            // CD → AL Music Player (native Android)
            media_key = hid::android::kAlMusicPlayer;
            break;
        case protocol::kButtonNav:
            // NAV → Key Mapper custom (for user remapping to Maps/Waze)
            media_key = hid::keymapper::kCustom1;
            break;
        case protocol::kButtonTel:
            // TEL → AL Phone (native Android dialer)
            media_key = hid::android::kAlPhone;
            break;
        default:
            return false;
    }

    if (media_key == 0) {
        return false;
    }

    if (event.state == protocol::kInputPressed) {
        ESP_LOGI(kTag, "Button pressed: 0x%02X -> HID key: 0x%04X",
                 event.id, media_key);
        hid_.MediaKeyPress(media_key);
    } else if (event.state == protocol::kInputReleased) {
        ESP_LOGI(kTag, "Button released: 0x%02X", event.id);
        hid_.MediaKeyRelease(media_key);
    }

    return true;
}

}  // namespace idrive
