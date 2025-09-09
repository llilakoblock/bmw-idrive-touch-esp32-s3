#include "usb_hid_device.h"

#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

// ESP-IDF TinyUSB includes
#include "tinyusb.h"
#include "tusb.h"

static const char *TAG = "USB_HID";


// Mutex for thread safety
static SemaphoreHandle_t hid_mutex = NULL;

// Connection state
static bool usb_connected = false;

// Report IDs
#define REPORT_ID_KEYBOARD 1
#define REPORT_ID_MOUSE    2
#define REPORT_ID_CONSUMER 3

// HID Report Descriptor for composite device (Keyboard + Mouse + Consumer Control)
static const uint8_t hid_report_descriptor[] = {
    // Keyboard
    0x05, 0x01,                // Usage Page (Generic Desktop)
    0x09, 0x06,                // Usage (Keyboard)
    0xA1, 0x01,                // Collection (Application)
    0x85, REPORT_ID_KEYBOARD,  // Report ID (1)
    0x05, 0x07,                // Usage Page (Keyboard)
    0x19, 0xE0,                // Usage Minimum (Keyboard Left Control)
    0x29, 0xE7,                // Usage Maximum (Keyboard Right GUI)
    0x15, 0x00,                // Logical Minimum (0)
    0x25, 0x01,                // Logical Maximum (1)
    0x75, 0x01,                // Report Size (1)
    0x95, 0x08,                // Report Count (8)
    0x81, 0x02,                // Input (Data,Var,Abs)
    0x95, 0x01,                // Report Count (1)
    0x75, 0x08,                // Report Size (8)
    0x81, 0x01,                // Input (Const,Array,Abs)
    0x95, 0x06,                // Report Count (6)
    0x75, 0x08,                // Report Size (8)
    0x15, 0x00,                // Logical Minimum (0)
    0x25, 0xFF,                // Logical Maximum (255)
    0x05, 0x07,                // Usage Page (Keyboard)
    0x19, 0x00,                // Usage Minimum (0)
    0x29, 0xFF,                // Usage Maximum (255)
    0x81, 0x00,                // Input (Data,Array,Abs)
    0xC0,                      // End Collection

    // Mouse
    0x05, 0x01,             // Usage Page (Generic Desktop)
    0x09, 0x02,             // Usage (Mouse)
    0xA1, 0x01,             // Collection (Application)
    0x85, REPORT_ID_MOUSE,  // Report ID (2)
    0x09, 0x01,             // Usage (Pointer)
    0xA1, 0x00,             // Collection (Physical)
    0x05, 0x09,             // Usage Page (Button)
    0x19, 0x01,             // Usage Minimum (Button 1)
    0x29, 0x05,             // Usage Maximum (Button 5)
    0x15, 0x00,             // Logical Minimum (0)
    0x25, 0x01,             // Logical Maximum (1)
    0x95, 0x05,             // Report Count (5)
    0x75, 0x01,             // Report Size (1)
    0x81, 0x02,             // Input (Data,Var,Abs)
    0x95, 0x01,             // Report Count (1)
    0x75, 0x03,             // Report Size (3)
    0x81, 0x01,             // Input (Const,Array,Abs)
    0x05, 0x01,             // Usage Page (Generic Desktop)
    0x09, 0x30,             // Usage (X)
    0x09, 0x31,             // Usage (Y)
    0x09, 0x38,             // Usage (Wheel)
    0x15, 0x81,             // Logical Minimum (-127)
    0x25, 0x7F,             // Logical Maximum (127)
    0x75, 0x08,             // Report Size (8)
    0x95, 0x03,             // Report Count (3)
    0x81, 0x06,             // Input (Data,Var,Rel)
    0xC0,                   // End Collection
    0xC0,                   // End Collection

    // Consumer Control (Media Keys)
    0x05, 0x0C,                // Usage Page (Consumer)
    0x09, 0x01,                // Usage (Consumer Control)
    0xA1, 0x01,                // Collection (Application)
    0x85, REPORT_ID_CONSUMER,  // Report ID (3)
    0x15, 0x00,                // Logical Minimum (0)
    0x25, 0x01,                // Logical Maximum (1)
    0x75, 0x01,                // Report Size (1)
    0x95, 0x10,                // Report Count (16)
    0x09, 0xB5,                // Usage (Scan Next Track)
    0x09, 0xB6,                // Usage (Scan Previous Track)
    0x09, 0xB7,                // Usage (Stop)
    0x09, 0xB8,                // Usage (Eject)
    0x09, 0xCD,                // Usage (Play/Pause)
    0x09, 0xE2,                // Usage (Mute)
    0x09, 0xE9,                // Usage (Volume Increment)
    0x09, 0xEA,                // Usage (Volume Decrement)
    0x0A, 0x23, 0x02,          // Usage (AC Home)
    0x0A, 0x24, 0x02,          // Usage (AC Back)
    0x0A, 0x21, 0x02,          // Usage (AC Search)
    0x09, 0x40,                // Usage (Menu)
    0x09, 0x30,                // Usage (Power)
    0x09, 0x32,                // Usage (Sleep)
    0x09, 0x34,                // Usage (Stand-by)
    0x09, 0x65,                // Usage (App Control Menu)
    0x81, 0x02,                // Input (Data,Var,Abs)
    0xC0                       // End Collection
};

// Report structures
typedef struct {
    uint8_t modifier;
    uint8_t reserved;
    uint8_t keycode[6];
} keyboard_report_t;

typedef struct {
    uint8_t buttons;
    int8_t  x;
    int8_t  y;
    int8_t  wheel;
} mouse_report_t;

typedef struct {
    uint16_t usage_id;
} consumer_report_t;

// Current reports
static keyboard_report_t keyboard_report = {0};
static mouse_report_t    mouse_report    = {0};

// TinyUSB callbacks
void tud_mount_cb(void)
{
    ESP_LOGI(TAG, "USB mounted");
    usb_connected = true;
}

void tud_umount_cb(void)
{
    ESP_LOGI(TAG, "USB unmounted");
    usb_connected = false;
}

void tud_suspend_cb(bool remote_wakeup_en)
{
    (void) remote_wakeup_en;
    ESP_LOGI(TAG, "USB suspended");
    usb_connected = false;
}

void tud_resume_cb(void)
{
    ESP_LOGI(TAG, "USB resumed");
    usb_connected = true;
}

// Get HID report descriptor
uint8_t const *tud_hid_descriptor_report_cb(uint8_t itf)
{
    (void) itf;
    return hid_report_descriptor;
}

// HID Get Report callback
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t reqlen)
{
    (void) itf;
    (void) reqlen;
    if (report_type != HID_REPORT_TYPE_INPUT) {
        return 0;
    }

    switch (report_id) {
        case REPORT_ID_KEYBOARD:
            memcpy(buffer, &keyboard_report, sizeof(keyboard_report));
            return sizeof(keyboard_report);
        case REPORT_ID_MOUSE:
            memcpy(buffer, &mouse_report, sizeof(mouse_report));
            return sizeof(mouse_report);
        default:
            return 0;
    }
}

// HID Set Report callback (for LEDs, etc.)
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t bufsize)
{
    (void) itf;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) bufsize;
    // Handle LED indicators or other output reports if needed
}

// Send keyboard report
static void send_keyboard_report(void)
{
    if (usb_connected && tud_hid_ready()) {
        uint8_t report[9];
        report[0] = REPORT_ID_KEYBOARD;
        memcpy(&report[1], &keyboard_report, sizeof(keyboard_report));
        tud_hid_report(0, report, sizeof(report));
    }
}

// Send mouse report
static void send_mouse_report(void)
{
    if (usb_connected && tud_hid_ready()) {
        uint8_t report[5];
        report[0] = REPORT_ID_MOUSE;
        memcpy(&report[1], &mouse_report, sizeof(mouse_report));
        tud_hid_report(0, report, sizeof(report));
    }
}

// Send consumer report
static void send_consumer_report(uint16_t usage_id)
{
    if (usb_connected && tud_hid_ready()) {
        uint8_t report[3];
        report[0] = REPORT_ID_CONSUMER;
        report[1] = usage_id & 0xFF;
        report[2] = (usage_id >> 8) & 0xFF;
        tud_hid_report(0, report, sizeof(report));
    }
}

// USB task to handle TinyUSB events
static void usb_device_task(void *param)
{
    (void) param;
    while (1) {
        tud_task();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// Public functions implementation
void usb_hid_device_init(void)
{
    ESP_LOGI(TAG, "Initializing USB HID device");

    // Create mutex
    hid_mutex = xSemaphoreCreateMutex();

    // Initialize TinyUSB
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor        = NULL,  // Use default descriptor
        .string_descriptor        = NULL,
        .external_phy             = false,
        .configuration_descriptor = NULL,
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    // Create USB device task
    xTaskCreate(usb_device_task, "usb_device_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "USB HID device initialized");
}

void usb_hid_keyboard_press(uint8_t keycode)
{
    if (xSemaphoreTake(hid_mutex, portMAX_DELAY) == pdTRUE) {
        // Find empty slot in keycode array
        for (int i = 0; i < 6; i++) {
            if (keyboard_report.keycode[i] == 0) {
                keyboard_report.keycode[i] = keycode;
                break;
            }
        }
        send_keyboard_report();
        xSemaphoreGive(hid_mutex);
    }
}

void usb_hid_keyboard_release(uint8_t keycode)
{
    if (xSemaphoreTake(hid_mutex, portMAX_DELAY) == pdTRUE) {
        // Remove keycode from array
        for (int i = 0; i < 6; i++) {
            if (keyboard_report.keycode[i] == keycode) {
                keyboard_report.keycode[i] = 0;
                break;
            }
        }
        send_keyboard_report();
        xSemaphoreGive(hid_mutex);
    }
}

void usb_hid_keyboard_press_and_release(uint8_t keycode)
{
    usb_hid_keyboard_press(keycode);
    vTaskDelay(pdMS_TO_TICKS(50));
    usb_hid_keyboard_release(keycode);
}

void usb_hid_media_key_press(uint8_t keycode)
{
    send_consumer_report(keycode);
}

void usb_hid_media_key_release(uint8_t keycode)
{
    (void) keycode;
    send_consumer_report(0);  // Release all consumer keys
}

void usb_hid_media_key_press_and_release(uint8_t keycode)
{
    usb_hid_media_key_press(keycode);
    vTaskDelay(pdMS_TO_TICKS(50));
    usb_hid_media_key_release(keycode);
}

void usb_hid_mouse_move(int8_t x, int8_t y)
{
    if (xSemaphoreTake(hid_mutex, portMAX_DELAY) == pdTRUE) {
        mouse_report.x = x;
        mouse_report.y = y;
        send_mouse_report();
        mouse_report.x = 0;
        mouse_report.y = 0;
        xSemaphoreGive(hid_mutex);
    }
}

void usb_hid_mouse_button_press(uint8_t button)
{
    if (xSemaphoreTake(hid_mutex, portMAX_DELAY) == pdTRUE) {
        mouse_report.buttons |= button;
        send_mouse_report();
        xSemaphoreGive(hid_mutex);
    }
}

void usb_hid_mouse_button_release(uint8_t button)
{
    if (xSemaphoreTake(hid_mutex, portMAX_DELAY) == pdTRUE) {
        mouse_report.buttons &= ~button;
        send_mouse_report();
        xSemaphoreGive(hid_mutex);
    }
}

void usb_hid_mouse_click(uint8_t button)
{
    usb_hid_mouse_button_press(button);
    vTaskDelay(pdMS_TO_TICKS(50));
    usb_hid_mouse_button_release(button);
}

void usb_hid_mouse_scroll(int8_t wheel)
{
    if (xSemaphoreTake(hid_mutex, portMAX_DELAY) == pdTRUE) {
        mouse_report.wheel = wheel;
        send_mouse_report();
        mouse_report.wheel = 0;
        xSemaphoreGive(hid_mutex);
    }
}

bool usb_hid_is_connected(void)
{
    return usb_connected;
}