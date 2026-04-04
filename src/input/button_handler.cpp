// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT

#include "input/button_handler.h"

#include "esp_log.h"

#include "config/config.h"
#include "hid/hid_keycodes.h"

namespace idrive {

namespace {
const char *kTag = "BUTTON";
}

bool ButtonHandler::Handle(const InputEvent &event)
{
    if (event.type != InputEvent::Type::Button) {
        return false;
    }

    uint16_t media_key = 0;
    uint8_t  keycode   = 0;

            // Native Android actions:
            // - MENU  -> Home
            // - BACK  -> Back
            // - MEDIA -> Music app
            //
            // Remappable buttons use keyboard F-keys:
            // - OPTIONS -> F1
            // - COM     -> F2
            // - NAV     -> F3
            // - MAP     -> F4

    switch (event.id) {
            case protocol::kButtonMenu:
                // Physical MENU -> Android Home
                media_key = hid::android::kHome;
                break;

            case protocol::kButtonBack:
                // Physical BACK -> Android Back
                media_key = hid::android::kBack;
                break;

            case protocol::kButtonCd:
                // Physical MEDIA -> Music app
                media_key = hid::android::kAlMusicPlayer;
                break;

            case protocol::kButtonOption:
                // Physical OPTIONS -> F1
                keycode = hid::key::kF1;
                break;

            case protocol::kButtonTel:
                // Physical COM -> F2
                keycode = hid::key::kF2;
                break;

            case protocol::kButtonNav:
                // Physical NAV -> F3
                keycode = hid::key::kF3;
                break;

            case protocol::kButtonMap:
                // Physical MAP -> F4
                keycode = hid::key::kF4;
                break;

            default:
                return false;
        }

        if (event.state == protocol::kInputPressed) {
            if (keycode != 0) {
                ESP_LOGI(kTag, "Button pressed: 0x%02X -> KEY 0x%02X", event.id, keycode);
                hid_.KeyPress(keycode);
            } else {
                ESP_LOGI(kTag, "Button pressed: 0x%02X -> HID key 0x%04X", event.id, media_key);
                hid_.MediaKeyPress(media_key);
            }
        } else if (event.state == protocol::kInputReleased) {
            if (keycode != 0) {
                ESP_LOGI(kTag, "Button released: 0x%02X -> KEY 0x%02X", event.id, keycode);
                hid_.KeyRelease(keycode);
            } else {
                ESP_LOGI(kTag, "Button released: 0x%02X", event.id);
                hid_.MediaKeyRelease(media_key);
            }
        }

        return true;
    }

}  // namespace idrive
