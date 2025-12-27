// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// iDrive controller CAN bus communication and input processing.
// Handles button presses, rotary encoder, joystick, and touchpad inputs.

#include "idrive.h"

#include <cmath>
#include <cstdint>
#include <cstring>

#include "driver/twai.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "key_assignments.h"
#include "settings.h"
#include "usb_hid_device.h"
#include "variables.h"

// =============================================================================
// Constants
// =============================================================================

namespace {

const char* kTag = "IDRIVE";

// CAN IDs to ignore in debug output.
int g_ignored_responses[] = {0x264, 0x267, 0x277, 0x567, 0x5E7, 0xBF};
const int kIgnoredResponsesSize = sizeof(g_ignored_responses) / sizeof(int);

}  // namespace

// =============================================================================
// Helper Functions
// =============================================================================

// Constrains a value to a specified range.
static int Constrain(int value, int min_val, int max_val)
{
    if (value < min_val) {
        return min_val;
    }
    if (value > max_val) {
        return max_val;
    }
    return value;
}

// Maps a value from one range to another (Arduino-style).
static int MapValue(int x, int in_min, int in_max, int out_min, int out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Returns current time in milliseconds.
static uint32_t GetMillis()
{
    return static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
}

// Sends a CAN frame with the specified parameters.
static void SendCanFrame(uint32_t can_id, bool extended, uint8_t length,
                         uint8_t* data)
{
    twai_message_t message   = {};
    message.identifier       = can_id;
    message.extd             = extended ? 1 : 0;
    message.data_length_code = length;

    for (int i = 0; i < length; i++) {
        message.data[i] = data[i];
    }

    esp_err_t ret = twai_transmit(&message, pdMS_TO_TICKS(50));
    if (ret != ESP_OK) {
        ESP_LOGW(kTag, "CAN transmit failed: %s", esp_err_to_name(ret));
    }
}

// =============================================================================
// Input Handlers
// =============================================================================

// Handles button press/release events.
static void HandleButton(uint8_t button, uint8_t state)
{
    if (!g_controller_ready || !UsbHidIsConnected()) {
        return;
    }

    // Map iDrive buttons to Android media keys.
    uint16_t media_key = 0;

    switch (button) {
        case kButtonMenu:
            media_key = HID_ANDROID_MENU;
            break;
        case kButtonBack:
            media_key = HID_ANDROID_BACK;
            break;
        case kButtonOption:
            media_key = HID_MEDIA_PLAY_PAUSE;
            break;
        case kButtonRadio:
            media_key = HID_MEDIA_PREV_TRACK;
            break;
        case kButtonCd:
            media_key = HID_MEDIA_NEXT_TRACK;
            break;
        case kButtonNav:
            media_key = HID_ANDROID_HOME;
            break;
        case kButtonTel:
            media_key = HID_ANDROID_SEARCH;
            break;
    }

    if (state == kInputPressed) {
        if (media_key != 0) {
            ESP_LOGI(kTag, "Button pressed: 0x%02X -> Media key: 0x%04X",
                     button, media_key);
            UsbHidMediaKeyPress(media_key);
        }
    } else if (state == kInputReleased) {
        if (media_key != 0) {
            ESP_LOGI(kTag, "Button released: 0x%02X", button);
            UsbHidMediaKeyRelease(media_key);
        }
    }
}

// Handles joystick direction events.
static void HandleJoystick(uint8_t direction, uint8_t state)
{
    if (!g_controller_ready || !UsbHidIsConnected()) {
        return;
    }

#ifdef IDRIVE_JOYSTICK_AS_MOUSE
    // Joystick as mouse movement.
    if (state == kInputPressed || state == kInputHeld) {
        int8_t x = 0;
        int8_t y = 0;

        if (direction & kStickUp) {
            y = -kJoystickMoveStep;
        }
        if (direction & kStickDown) {
            y = kJoystickMoveStep;
        }
        if (direction & kStickLeft) {
            x = -kJoystickMoveStep;
        }
        if (direction & kStickRight) {
            x = kJoystickMoveStep;
        }

        if (x != 0 || y != 0) {
            ESP_LOGI(kTag, "Joystick move: x=%d, y=%d", x, y);
            UsbHidMouseMove(x, y);
        }
    }

    // Center press as left click.
    if (direction == kStickCenter) {
        if (state == kInputPressed) {
            ESP_LOGI(kTag, "Joystick center pressed - left click");
            UsbHidMouseButtonPress(HID_MOUSE_BUTTON_LEFT);
        } else if (state == kInputReleased) {
            ESP_LOGI(kTag, "Joystick center released");
            UsbHidMouseButtonRelease(HID_MOUSE_BUTTON_LEFT);
        }
    }
#else
    // Joystick as arrow keys.
    uint8_t key = 0;

    if (direction & kStickUp) {
        key = HID_KEY_UP;
    } else if (direction & kStickDown) {
        key = HID_KEY_DOWN;
    } else if (direction & kStickLeft) {
        key = HID_KEY_LEFT;
    } else if (direction & kStickRight) {
        key = HID_KEY_RIGHT;
    } else if (direction == kStickCenter) {
        key = HID_KEY_ENTER;
    }

    if (key != 0) {
        if (state == kInputPressed) {
            ESP_LOGI(kTag, "Joystick arrow key pressed: 0x%02X", key);
            UsbHidKeyboardPress(key);
        } else if (state == kInputReleased) {
            ESP_LOGI(kTag, "Joystick arrow key released: 0x%02X", key);
            UsbHidKeyboardRelease(key);
        }
    }
#endif
}

// Handles rotary encoder rotation events.
static void HandleRotary(int8_t steps)
{
    if (!g_controller_ready || !UsbHidIsConnected() || g_rotary_disabled) {
        return;
    }

    // Use rotary for volume control.
    if (steps > 0) {
        for (int i = 0; i < steps; i++) {
            ESP_LOGI(kTag, "Rotary right - Volume up");
            UsbHidMediaKeyPressAndRelease(HID_MEDIA_VOLUME_UP);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    } else if (steps < 0) {
        for (int i = 0; i < -steps; i++) {
            ESP_LOGI(kTag, "Rotary left - Volume down");
            UsbHidMediaKeyPressAndRelease(HID_MEDIA_VOLUME_DOWN);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
}

// Handles touchpad touch events.
static void HandleTouchpad(int16_t x, int16_t y, uint8_t touch_type)
{
    if (!g_controller_ready || !UsbHidIsConnected()) {
        return;
    }

    static int16_t last_x = 0;
    static int16_t last_y = 0;
    static bool was_tracking = false;

    if (touch_type == kTouchFingerRemoved) {
        g_touching = false;
        was_tracking = false;
        ESP_LOGI(kTag, "Touchpad: finger removed");
        return;
    }

    g_touching = true;

    if (!was_tracking) {
        // First touch - save initial position.
        last_x = x;
        last_y = y;
        was_tracking = true;
        ESP_LOGI(kTag, "Touchpad: touch started at x=%d, y=%d", x, y);
        return;
    }

    // Calculate movement delta.
    int16_t delta_x = x - last_x;
    int16_t delta_y = y - last_y;

    // Apply threshold to avoid jitter.
    if (std::abs(delta_x) < kMinMouseTravel) {
        delta_x = 0;
    }
    if (std::abs(delta_y) < kMinMouseTravel) {
        delta_y = 0;
    }

    if (delta_x != 0 || delta_y != 0) {
        // Scale movement for better feel.
        int8_t mouse_x = Constrain(delta_x * kXMultiplier / 10, -127, 127);
        int8_t mouse_y = Constrain(delta_y * kYMultiplier / 10, -127, 127);

        ESP_LOGI(kTag, "Touchpad move: x=%d, y=%d (delta: %d, %d)",
                 mouse_x, mouse_y, delta_x, delta_y);
        UsbHidMouseMove(mouse_x, mouse_y);

        last_x = x;
        last_y = y;
    }
}

// =============================================================================
// Public Functions - iDrive Control
// =============================================================================

void IDriveInit()
{
    // Rotary encoder initialization frame.
    // ID 0x273, Data: 1D E1 00 F0 FF 7F DE 04
    uint8_t data[8] = {0x1D, 0xE1, 0x00, 0xF0, 0xFF, 0x7F, 0xDE, 0x04};
    SendCanFrame(kMsgOutRotaryInit, false, 8, data);

    g_rotary_init_position_set = false;
    ESP_LOGI(kTag, "Sent iDriveInit frame");
}

void IDriveTouchpadInit()
{
    // Touchpad initialization frame.
    // ID 0xBF, Data: 21 00 00 00 11 00 00 00
    uint8_t data[8] = {0x21, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00};
    SendCanFrame(kMsgInTouch, false, 8, data);
    ESP_LOGI(kTag, "Sent iDriveTouchpadInit frame");
}

void IDriveLightSend()
{
    // Light control frame.
    // ID 0x202, Data: 02 FD 00 (on) or 02 FE 00 (off)
    uint8_t data[3];
    data[0] = 0x02;
    data[1] = g_idrive_light_on ? 0xFD : 0xFE;
    data[2] = 0x00;
    SendCanFrame(kMsgOutLight, false, 3, data);
}

void IDriveLight(unsigned long interval_ms)
{
    static uint32_t last_light_time = 0;
    uint32_t now = GetMillis();

    if (now - last_light_time >= interval_ms) {
        last_light_time = now;
        IDriveLightSend();
    }
}

void IDrivePollSend()
{
    // Poll frame.
    // ID 0x501, Data: 01 00 00 00 00 00 00 00
    uint8_t data[8] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    SendCanFrame(kMsgOutPoll, false, 8, data);
}

void IDrivePoll(unsigned long interval_ms)
{
    static uint32_t last_poll_time = 0;
    uint32_t now = GetMillis();

    if (now - last_poll_time >= interval_ms) {
        last_poll_time = now;
#if defined(SERIAL_DEBUG) && defined(DEBUG_CAN_RESPONSE)
        ESP_LOGI(kTag, "iDrive Polling");
#endif
        IDrivePollSend();
    }
}

// =============================================================================
// Public Functions - CAN Message Processing
// =============================================================================

bool IsValueInArray(int value, int* array, int size)
{
    for (int i = 0; i < size; i++) {
        if (array[i] == value) {
            return true;
        }
    }
    return false;
}

void DecodeCanMessage(unsigned long can_id, uint8_t length, uint8_t* data)
{
    // Debug: log all incoming messages.
    ESP_LOGI(kTag, "RX <- ID: 0x%03lX, DLC:%d, Data: %02X %02X %02X %02X %02X %02X %02X %02X",
             can_id, length,
             length > 0 ? data[0] : 0, length > 1 ? data[1] : 0,
             length > 2 ? data[2] : 0, length > 3 ? data[3] : 0,
             length > 4 ? data[4] : 0, length > 5 ? data[5] : 0,
             length > 6 ? data[6] : 0, length > 7 ? data[7] : 0);

    // Log unknown CAN IDs.
    if (can_id != kMsgOutPoll && can_id != kMsgOutLight &&
        can_id != kMsgOutRotaryInit && can_id != 0x567 && can_id != 0x277 &&
        can_id != 0x5E7 && can_id != 0x267 && can_id != 0x264 &&
        can_id != 0xBF) {
        ESP_LOGW(kTag, "*** UNKNOWN CAN ID: 0x%03lX ***", can_id);
    }

    // Ignore our own transmitted messages (echo).
    if (can_id == kMsgOutPoll || can_id == kMsgOutLight ||
        can_id == kMsgOutRotaryInit) {
        return;
    }

#ifdef DEBUG_CAN_RESPONSE
    if (!IsValueInArray(can_id, g_ignored_responses, kIgnoredResponsesSize)) {
        ESP_LOGI(kTag, "CAN ID: 0x%03lX, DLC:%d, Data:", can_id, length);
        for (int i = 0; i < length; i++) {
            printf("%02X ", data[i]);
        }
        printf("\n");
    }
#endif

    // Process CAN messages by ID.
    switch (can_id) {
        case kMsgInInput: {
            // 0x267 - Buttons and joystick.
            if (length >= 6) {
                uint8_t state = data[3] & 0x0F;
                uint8_t input_type = data[4];
                uint8_t input = data[5];

                if (input_type == kInputTypeButton) {
                    HandleButton(input, state);
                } else if (input_type == kInputTypeStick) {
                    uint8_t direction = data[3] >> 4;
                    HandleJoystick(direction, state);
                } else if (input_type == kInputTypeCenter) {
                    HandleJoystick(kStickCenter, state);
                }
            }
            break;
        }

        case kMsgInRotary: {
            // 0x264 - Rotary encoder.
            ESP_LOGI(kTag, "*** ROTARY DATA ***");
            if (length >= 5 && g_rotary_init_position_set) {
                unsigned int new_position = data[3] + (data[4] * 0x100);
                int delta = static_cast<int>(new_position) -
                            static_cast<int>(g_rotary_position);

                // Handle wraparound.
                if (delta > 32768) {
                    delta -= 65536;
                } else if (delta < -32768) {
                    delta += 65536;
                }

                if (delta != 0) {
                    HandleRotary(delta);
                    g_rotary_position = new_position;
                }
            } else if (!g_rotary_init_position_set && length >= 5) {
                // Set initial position.
                uint8_t rotary_step_b = data[4];
                unsigned int new_pos = data[3] + (data[4] * 0x100);

                switch (rotary_step_b) {
                    case 0x7F:
                        g_rotary_position = new_pos + 1;
                        break;
                    case 0x80:
                        g_rotary_position = new_pos - 1;
                        break;
                    default:
                        g_rotary_position = new_pos;
                        break;
                }
                g_rotary_init_position_set = true;
                ESP_LOGI(kTag, "Rotary initial position: %u", g_rotary_position);
            }
            break;
        }

        case kMsgInTouch: {
            // 0xBF - Touchpad.
            ESP_LOGI(kTag, "*** TOUCHPAD MESSAGE RECEIVED ***");
            if (length >= 8) {
                uint8_t touch_type = data[4];

                // Ignore initial touchpad messages during initialization.
                if (g_touchpad_init_ignore_counter < kTouchpadInitIgnoreCount &&
                    g_rotary_init_success) {
                    g_touchpad_init_ignore_counter++;
                    ESP_LOGI(kTag, "Touchpad ignoring message %d/%d",
                             g_touchpad_init_ignore_counter,
                             kTouchpadInitIgnoreCount);
                    break;
                }

                if (touch_type == kTouchFingerRemoved) {
                    HandleTouchpad(0, 0, touch_type);
                } else if (touch_type == kTouchSingle ||
                           touch_type == kTouchMulti) {
                    // Extract coordinates.
                    int16_t x = static_cast<int8_t>(data[1]);
                    int16_t y = static_cast<int8_t>(data[3]);
                    uint8_t x_lr = data[2] & 0x0F;

                    ESP_LOGI(kTag, "Touch: X=%d, Y=%d, xLR=%d", x, y, x_lr);

                    // Convert coordinates.
                    if (x_lr == 0) {
                        // Left half: 0-255 -> -128 to 0.
                        x = MapValue(static_cast<uint8_t>(data[1]), 0, 255,
                                     -128, 0);
                    } else if (x_lr == 1) {
                        // Right half: 0-255 -> 0 to 127.
                        x = MapValue(static_cast<uint8_t>(data[1]), 0, 255,
                                     0, 127);
                    }

                    // Y coordinate mapping.
                    y = MapValue(static_cast<uint8_t>(data[3]), 0, 30,
                                 -128, 127);

                    HandleTouchpad(x, y, touch_type);
                }
            }
            break;
        }

        case kMsgInRotaryInit: {
            // 0x277 - Rotary initialization response.
            ESP_LOGI(kTag, "Rotary Init Success");
            g_rotary_init_success = true;
            break;
        }

        case kMsgInStatus: {
            // 0x5E7 - Status message.
            if (length >= 5) {
                ESP_LOGI(kTag, "Status message: data[4]=0x%02X", data[4]);
                if (data[4] == kStatusNoInit) {
                    // Lost initialization - reinitialize.
                    ESP_LOGW(kTag, "iDrive lost init - reinitializing");
                    g_rotary_init_success = false;
                    g_light_init_done = false;
                    g_touchpad_init_done = false;
                    g_previous_millis = 0;
                    g_cooldown_millis = 0;
                    g_touchpad_init_ignore_counter = 0;
                    g_controller_ready = false;
                    g_rotary_init_position_set = false;

                    IDriveInit();
                }
            }
            break;
        }
    }
}
