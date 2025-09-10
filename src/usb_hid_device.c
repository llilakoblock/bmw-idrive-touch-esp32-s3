#include "usb_hid_device.h"

#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

// ESP TinyUSB includes
#include "class/hid/hid_device.h"
#include "tinyusb.h"

static const char *TAG = "USB_HID";

// Mutex for thread safety
static SemaphoreHandle_t hid_mutex = NULL;

// Connection state
static bool usb_connected = false;

// Report IDs
enum { REPORT_ID_KEYBOARD = 1, REPORT_ID_MOUSE, REPORT_ID_CONSUMER };

// HID Report Descriptor
static const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(REPORT_ID_KEYBOARD)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(REPORT_ID_MOUSE)),
    TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(REPORT_ID_CONSUMER))};

// Current reports
static hid_keyboard_report_t keyboard_report = {0};
static hid_mouse_report_t    mouse_report    = {0};

// Configuration descriptor
static const uint8_t hid_configuration_descriptor[] = {
    // Configuration number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN, 0, 100),

    // Interface number, string index, protocol, report descriptor len, EP In address, size &
    // polling interval
    TUD_HID_DESCRIPTOR(0, 0, false, sizeof(hid_report_descriptor), 0x81, 16, 10),
};

// Invoked when device is mounted
void tud_mount_cb(void)
{
    ESP_LOGI(TAG, "USB mounted");
    usb_connected = true;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
    ESP_LOGI(TAG, "USB unmounted");
    usb_connected = false;
}

// Invoked when usb bus is suspended
void tud_suspend_cb(bool remote_wakeup_en)
{
    (void) remote_wakeup_en;
    ESP_LOGI(TAG, "USB suspended");
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
    ESP_LOGI(TAG, "USB resumed");
}

// Invoked to get HID report descriptor
uint8_t const *tud_hid_descriptor_report_cb(uint8_t itf)
{
    (void) itf;
    return hid_report_descriptor;
}

// Invoked when received GET_REPORT control request
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t reqlen)
{
    (void) itf;
    (void) report_type;
    (void) reqlen;

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

// Invoked when received SET_REPORT control request
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t bufsize)
{
    (void) itf;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) bufsize;
}

// USB task
static void usb_device_task(void *arg)
{
    (void) arg;

    while (1) {
        tud_task();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// Initialize USB HID
void usb_hid_device_init(void)
{
    ESP_LOGI(TAG, "Initializing USB HID device");

    // Create mutex
    hid_mutex = xSemaphoreCreateMutex();

    // Configure USB descriptors
    static tusb_desc_device_t descriptor = {.bLength            = sizeof(descriptor),
                                            .bDescriptorType    = TUSB_DESC_DEVICE,
                                            .bcdUSB             = 0x0200,
                                            .bDeviceClass       = 0,
                                            .bDeviceSubClass    = 0,
                                            .bDeviceProtocol    = 0,
                                            .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
                                            .idVendor           = 0x303A,  // Espressif VID
                                            .idProduct          = 0x4002,
                                            .bcdDevice          = 0x0100,
                                            .iManufacturer      = 0x01,
                                            .iProduct           = 0x02,
                                            .iSerialNumber      = 0x03,
                                            .bNumConfigurations = 0x01};

    // Определите языковой дескриптор отдельно
    static const char language_descriptor[] = {0x09, 0x04};

    // Теперь используйте его в массиве
    static const char *string_descriptor[] = {
        language_descriptor,  // Language
        "BMW",                // Manufacturer
        "iDrive Controller",  // Product
        "123456",             // Serial
        "HID Interface",      // Interface
    };

    tinyusb_config_t tusb_cfg = {
        .device_descriptor        = &descriptor,
        .string_descriptor        = string_descriptor,
        .string_descriptor_count  = sizeof(string_descriptor) / sizeof(string_descriptor[0]),
        .external_phy             = false,
        .configuration_descriptor = hid_configuration_descriptor,
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    // Create USB task
    xTaskCreate(usb_device_task, "TinyUSB", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "USB HID initialized");
}

// Keyboard functions
void usb_hid_keyboard_press(uint8_t keycode)
{
    if (!usb_connected)
        return;

    if (xSemaphoreTake(hid_mutex, portMAX_DELAY) == pdTRUE) {
        // Find empty slot
        for (int i = 0; i < 6; i++) {
            if (keyboard_report.keycode[i] == 0) {
                keyboard_report.keycode[i] = keycode;
                break;
            }
        }

        tud_hid_n_report(0, REPORT_ID_KEYBOARD, &keyboard_report, sizeof(keyboard_report));
        xSemaphoreGive(hid_mutex);
    }
}

void usb_hid_keyboard_release(uint8_t keycode)
{
    if (!usb_connected)
        return;

    if (xSemaphoreTake(hid_mutex, portMAX_DELAY) == pdTRUE) {
        // Remove keycode
        for (int i = 0; i < 6; i++) {
            if (keyboard_report.keycode[i] == keycode) {
                keyboard_report.keycode[i] = 0;
                break;
            }
        }

        tud_hid_n_report(0, REPORT_ID_KEYBOARD, &keyboard_report, sizeof(keyboard_report));
        xSemaphoreGive(hid_mutex);
    }
}

void usb_hid_keyboard_press_and_release(uint8_t keycode)
{
    usb_hid_keyboard_press(keycode);
    vTaskDelay(pdMS_TO_TICKS(50));
    usb_hid_keyboard_release(keycode);
}

// Media key functions
void usb_hid_media_key_press(uint8_t keycode)
{
    if (!usb_connected)
        return;

    uint16_t usage = keycode;
    tud_hid_n_report(0, REPORT_ID_CONSUMER, &usage, 2);
}

void usb_hid_media_key_release(uint8_t keycode)
{
    if (!usb_connected)
        return;

    uint16_t usage = 0;
    tud_hid_n_report(0, REPORT_ID_CONSUMER, &usage, 2);
}

void usb_hid_media_key_press_and_release(uint8_t keycode)
{
    usb_hid_media_key_press(keycode);
    vTaskDelay(pdMS_TO_TICKS(50));
    usb_hid_media_key_release(keycode);
}

// Mouse functions
void usb_hid_mouse_move(int8_t x, int8_t y)
{
    if (!usb_connected)
        return;

    if (xSemaphoreTake(hid_mutex, portMAX_DELAY) == pdTRUE) {
        mouse_report.x = x;
        mouse_report.y = y;
        tud_hid_n_report(0, REPORT_ID_MOUSE, &mouse_report, sizeof(mouse_report));
        mouse_report.x = 0;
        mouse_report.y = 0;
        xSemaphoreGive(hid_mutex);
    }
}

void usb_hid_mouse_button_press(uint8_t button)
{
    if (!usb_connected)
        return;

    if (xSemaphoreTake(hid_mutex, portMAX_DELAY) == pdTRUE) {
        mouse_report.buttons |= button;
        tud_hid_n_report(0, REPORT_ID_MOUSE, &mouse_report, sizeof(mouse_report));
        xSemaphoreGive(hid_mutex);
    }
}

void usb_hid_mouse_button_release(uint8_t button)
{
    if (!usb_connected)
        return;

    if (xSemaphoreTake(hid_mutex, portMAX_DELAY) == pdTRUE) {
        mouse_report.buttons &= ~button;
        tud_hid_n_report(0, REPORT_ID_MOUSE, &mouse_report, sizeof(mouse_report));
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
    if (!usb_connected)
        return;

    if (xSemaphoreTake(hid_mutex, portMAX_DELAY) == pdTRUE) {
        mouse_report.wheel = wheel;
        tud_hid_n_report(0, REPORT_ID_MOUSE, &mouse_report, sizeof(mouse_report));
        mouse_report.wheel = 0;
        xSemaphoreGive(hid_mutex);
    }
}

bool usb_hid_is_connected(void)
{
    return usb_connected && tud_ready();
}