#include "idrive.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "driver/twai.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "key_assignments.h"
#include "settings.h"
#include "usb_hid_device.h"
#include "variables.h"

static const char *TAG = "IDRIVE";

// CAN IDs to ignore in debug output
int       ignored_responses[]    = {0x264, 0x267, 0x277, 0x567, 0x5E7, 0xBF};
const int ignored_responses_size = sizeof(ignored_responses) / sizeof(int);

// Helper to constrain value - moved before usage
static int constrain(int value, int min, int max)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

// Helper function to send a CAN frame
static void sendCANFrame(uint32_t canId, bool extended, uint8_t length, uint8_t *data)
{
    twai_message_t message;
    message.identifier       = canId;
    message.extd             = extended ? 1 : 0;
    message.data_length_code = length;
    for (int i = 0; i < length; i++) {
        message.data[i] = data[i];
    }

    esp_err_t ret = twai_transmit(&message, pdMS_TO_TICKS(50));
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "CAN transmit failed: %s", esp_err_to_name(ret));
    }
}

/*=========================================================================*/
/* iDrive Functions                                                        */
/*=========================================================================*/

void iDriveInit()
{
    // Example: ID 273, Data: 1D E1 00 F0 FF 7F DE 04
    uint8_t buf[8] = {0x1D, 0xE1, 0x00, 0xF0, 0xFF, 0x7F, 0xDE, 0x04};
    sendCANFrame(MSG_OUT_ROTARY_INIT, false, 8, buf);

    RotaryInitPositionSet = false;
    ESP_LOGI(TAG, "Sent iDriveInit frame");
}

void iDriveTouchpadInit()
{
    // Example: ID BF, Data: 21 00 00 00 11 00 00 00
    uint8_t buf[8] = {0x21, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00};
    sendCANFrame(MSG_IN_TOUCH, false, 8, buf);
    ESP_LOGI(TAG, "Sent iDriveTouchpadInit frame");
}

void do_iDriveLight()
{
    // ID 202 => 2 FD 0 => on, 2 FE 0 => off
    uint8_t buf[3];
    buf[0] = 0x02;
    buf[1] = (iDriveLightOn) ? 0xFD : 0xFE;
    buf[2] = 0x00;
    sendCANFrame(MSG_OUT_LIGHT, false, 3, buf);
}

void iDriveLight(unsigned long milliseconds)
{
    static uint32_t lastLight = 0;
    uint32_t        now       = (uint32_t) (esp_timer_get_time() / 1000ULL);

    if (now - lastLight >= milliseconds) {
        lastLight = now;
        do_iDriveLight();
    }
}

void do_iDrivePoll()
{
    // ID 501 => 1 0 0 0 0 0 0 0
    uint8_t buf[8] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    sendCANFrame(MSG_OUT_POLL, false, 8, buf);
}

void iDrivePoll(unsigned long milliseconds)
{
    static uint32_t lastPing = 0;
    uint32_t        now      = (uint32_t) (esp_timer_get_time() / 1000ULL);

    if (now - lastPing >= milliseconds) {
        lastPing = now;
#if defined(SERIAL_DEBUG) && defined(DEBUG_CanResponse)
        ESP_LOGI(TAG, "iDrive Polling");
#endif
        do_iDrivePoll();
    }
}

/*=========================================================================*/
/* Helper functions for button/joystick handling                          */
/*=========================================================================*/

static void handleButton(uint8_t button, uint8_t state)
{
    if (!controllerReady || !usb_hid_is_connected())
        return;

    // Map iDrive buttons to Android media keys
    uint16_t mediaKey    = 0;
    uint8_t  keyboardKey = 0;

    switch (button) {
        case MSG_INPUT_BUTTON_MENU:
            mediaKey = HID_ANDROID_MENU;  // Android Menu key
            break;
        case MSG_INPUT_BUTTON_BACK:
            mediaKey = HID_ANDROID_BACK;  // Android Back key
            break;
        case MSG_INPUT_BUTTON_OPTION:
            mediaKey = HID_MEDIA_PLAY_PAUSE;  // Play/Pause
            break;
        case MSG_INPUT_BUTTON_RADIO:
            mediaKey = HID_MEDIA_PREV_TRACK;  // Previous track
            break;
        case MSG_INPUT_BUTTON_CD:
            mediaKey = HID_MEDIA_NEXT_TRACK;  // Next track
            break;
        case MSG_INPUT_BUTTON_NAV:
            mediaKey = HID_ANDROID_HOME;  // Android Home
            break;
        case MSG_INPUT_BUTTON_TEL:
            mediaKey = HID_ANDROID_SEARCH;  // Android Search
            break;
    }

    if (state == MSG_INPUT_PRESSED) {
        if (mediaKey) {
            ESP_LOGI(TAG, "Button pressed: 0x%02X -> Media key: 0x%04X", button, mediaKey);
            usb_hid_media_key_press(mediaKey);
        } else if (keyboardKey) {
            ESP_LOGI(TAG, "Button pressed: 0x%02X -> Keyboard key: 0x%02X", button, keyboardKey);
            usb_hid_keyboard_press(keyboardKey);
        }
    } else if (state == MSG_INPUT_RELEASED) {
        if (mediaKey) {
            ESP_LOGI(TAG, "Button released: 0x%02X", button);
            usb_hid_media_key_release(mediaKey);
        } else if (keyboardKey) {
            ESP_LOGI(TAG, "Button released: 0x%02X", button);
            usb_hid_keyboard_release(keyboardKey);
        }
    }
}

static void handleJoystick(uint8_t direction, uint8_t state)
{
    if (!controllerReady || !usb_hid_is_connected())
        return;

#ifdef iDriveJoystickAsMouse
    // Joystick as mouse movement
    if (state == MSG_INPUT_PRESSED || state == MSG_INPUT_HELD) {
        int8_t x = 0, y = 0;

        if (direction & MSG_INPUT_STICK_UP)
            y = -JOYSTICK_MOVE_STEP;
        if (direction & MSG_INPUT_STICK_DOWN)
            y = JOYSTICK_MOVE_STEP;
        if (direction & MSG_INPUT_STICK_LEFT)
            x = -JOYSTICK_MOVE_STEP;
        if (direction & MSG_INPUT_STICK_RIGHT)
            x = JOYSTICK_MOVE_STEP;

        if (x != 0 || y != 0) {
            ESP_LOGI(TAG, "Joystick move: x=%d, y=%d", x, y);
            usb_hid_mouse_move(x, y);
        }
    }

    // Center press as left click
    if (direction == MSG_INPUT_STICK_CENTER) {
        if (state == MSG_INPUT_PRESSED) {
            ESP_LOGI(TAG, "Joystick center pressed - left click");
            usb_hid_mouse_button_press(HID_MOUSE_BUTTON_LEFT);
        } else if (state == MSG_INPUT_RELEASED) {
            ESP_LOGI(TAG, "Joystick center released");
            usb_hid_mouse_button_release(HID_MOUSE_BUTTON_LEFT);
        }
    }
#else
    // Joystick as arrow keys
    uint8_t key = 0;

    if (direction & MSG_INPUT_STICK_UP)
        key = HID_KEY_UP;
    else if (direction & MSG_INPUT_STICK_DOWN)
        key = HID_KEY_DOWN;
    else if (direction & MSG_INPUT_STICK_LEFT)
        key = HID_KEY_LEFT;
    else if (direction & MSG_INPUT_STICK_RIGHT)
        key = HID_KEY_RIGHT;
    else if (direction == MSG_INPUT_STICK_CENTER)
        key = HID_KEY_ENTER;

    if (key) {
        if (state == MSG_INPUT_PRESSED) {
            ESP_LOGI(TAG, "Joystick arrow key pressed: 0x%02X", key);
            usb_hid_keyboard_press(key);
        } else if (state == MSG_INPUT_RELEASED) {
            ESP_LOGI(TAG, "Joystick arrow key released: 0x%02X", key);
            usb_hid_keyboard_release(key);
        }
    }
#endif
}

static void handleRotary(int8_t steps)
{
    if (!controllerReady || !usb_hid_is_connected() || rotaryDisabled)
        return;

    // Use rotary for volume control
    if (steps > 0) {
        for (int i = 0; i < steps; i++) {
            ESP_LOGI(TAG, "Rotary right - Volume up");
            usb_hid_media_key_press_and_release(HID_MEDIA_VOLUME_UP);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    } else if (steps < 0) {
        for (int i = 0; i < -steps; i++) {
            ESP_LOGI(TAG, "Rotary left - Volume down");
            usb_hid_media_key_press_and_release(HID_MEDIA_VOLUME_DOWN);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
}

static void handleTouchpad(int16_t x, int16_t y, uint8_t touchType)
{
    if (!controllerReady || !usb_hid_is_connected() || !TouchpadInitDone)
        return;

    static int16_t lastX = 0, lastY = 0;
    static bool    wasTracking = false;

    if (touchType == FINGER_REMOVED) {
        touching    = false;
        wasTracking = false;
        ESP_LOGI(TAG, "Touchpad: finger removed");
        return;
    }

    touching = true;

    if (!wasTracking) {
        // First touch - just save position
        lastX       = x;
        lastY       = y;
        wasTracking = true;
        ESP_LOGI(TAG, "Touchpad: touch started at x=%d, y=%d", x, y);
        return;
    }

    // Calculate movement delta
    int16_t deltaX = x - lastX;
    int16_t deltaY = y - lastY;

    // Apply threshold to avoid jitter
    if (abs(deltaX) < min_mouse_travel)
        deltaX = 0;
    if (abs(deltaY) < min_mouse_travel)
        deltaY = 0;

    if (deltaX != 0 || deltaY != 0) {
        // Scale movement for better feel
        int8_t mouseX = constrain(deltaX * x_multiplier / 10, -127, 127);
        int8_t mouseY = constrain(deltaY * y_multiplier / 10, -127, 127);

        ESP_LOGI(TAG, "Touchpad move: x=%d, y=%d (delta: %d, %d)", mouseX, mouseY, deltaX, deltaY);
        usb_hid_mouse_move(mouseX, mouseY);

        lastX = x;
        lastY = y;
    }
}

/*=========================================================================*/
/*  decodeCanBus() - Process CAN messages and trigger USB HID events      */
/*=========================================================================*/

bool isvalueinarray(int val, int *arr, int size)
{
    for (int i = 0; i < size; i++) {
        if (arr[i] == val)
            return true;
    }
    return false;
}

void decodeCanBus(unsigned long canId, uint8_t len, uint8_t *buf)
{
#ifdef DEBUG_CanResponse
    if (!isvalueinarray(canId, ignored_responses, ignored_responses_size)) {
        ESP_LOGI(TAG, "CAN ID: 0x%03lX, DLC:%d, Data:", canId, len);
        for (int i = 0; i < len; i++) {
            printf("%02X ", buf[i]);
        }
        printf("\n");
    }
#endif

    // Process different CAN messages
    switch (canId) {
        case MSG_IN_INPUT: {  // 0x267 - Buttons and joystick
            if (len >= 3) {
                uint8_t type  = buf[0];
                uint8_t value = buf[1];
                uint8_t state = buf[2];

                if (type == MSG_INPUT_BUTTON) {
                    handleButton(value, state);
                } else if (type == MSG_INPUT_STICK || type == MSG_INPUT_CENTER) {
                    handleJoystick(value, state);
                }
            }
            break;
        }

        case MSG_IN_ROTARY: {  // 0x264 - Rotary encoder
            if (len >= 2 && RotaryInitPositionSet) {
                unsigned int newPosition = (buf[0] << 8) | buf[1];
                int          delta       = (int) newPosition - (int) rotaryposition;

                // Handle wraparound
                if (delta > 32768)
                    delta -= 65536;
                else if (delta < -32768)
                    delta += 65536;

                if (delta != 0) {
                    handleRotary(delta);
                    rotaryposition = newPosition;
                }
            } else if (!RotaryInitPositionSet && len >= 2) {
                // Set initial position
                rotaryposition        = (buf[0] << 8) | buf[1];
                RotaryInitPositionSet = true;
                ESP_LOGI(TAG, "Rotary initial position: %u", rotaryposition);
            }
            break;
        }

        case MSG_IN_TOUCH: {  // 0xBF - Touchpad
            if (len >= 8) {
                // Touchpad data format varies by version
                if (TouchpadInitIgnoreCounter < TouchpadInitIgnoreCount) {
                    TouchpadInitIgnoreCounter++;
                    TouchpadInitDone = true;
                    ESP_LOGI(TAG, "Touchpad init progress: %d/%d", TouchpadInitIgnoreCounter,
                             TouchpadInitIgnoreCount);
                    break;
                }

#ifdef MOUSE_V1
                // V1 format: touch type at buf[1], coordinates in specific positions
                uint8_t touchType = buf[1];

                if (touchType == SINGLE_TOUCH) {
                    int16_t x = (int8_t) buf[4];  // Signed 8-bit
                    int16_t y = (int8_t) buf[5];  // Signed 8-bit
                    handleTouchpad(x, y, touchType);
                } else if (touchType == FINGER_REMOVED) {
                    handleTouchpad(0, 0, touchType);
                }
#else
                // V2 format: 16-bit coordinates
                uint8_t touchType = buf[6];

                if (touchType != FINGER_REMOVED) {
                    int16_t x = (buf[2] << 8) | buf[3];
                    int16_t y = (buf[4] << 8) | buf[5];
                    handleTouchpad(x, y, touchType);
                } else {
                    handleTouchpad(0, 0, touchType);
                }
#endif
            }
            break;
        }

        case MSG_IN_ROTARY_INIT: {  // 0x277
            ESP_LOGI(TAG, "Rotary Init Success");
            RotaryInitSuccess = true;
            break;
        }

        case MSG_IN_STATUS: {  // 0x5E7
            if (buf[4] == MSG_STATUS_NO_INIT) {
                ESP_LOGW(TAG, "iDrive lost init - reinitializing");
                RotaryInitSuccess         = false;
                LightInitDone             = false;
                TouchpadInitDone          = false;
                previousMillis            = 0;
                CoolDownMillis            = 0;
                TouchpadInitIgnoreCounter = 0;
                controllerReady           = false;

                // Reinitialize
                iDriveInit();
                iDriveTouchpadInit();
            }
            break;
        }
    }
}