// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// USB HID device implementation using TinyUSB.

#include "usb_hid_device.h"

#include <string.h>

#include "class/hid/hid_device.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "tinyusb.h"

// =============================================================================
// Constants
// =============================================================================

static const char* kTag = "USB_HID";

// Report IDs for different HID functions.
enum ReportId {
    kReportIdKeyboard = 1,
    kReportIdMouse    = 2,
    kReportIdConsumer = 3,
};

// =============================================================================
// HID Descriptors
// =============================================================================

// Combined HID report descriptor for keyboard, mouse, and consumer controls.
static const uint8_t kHidReportDescriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(kReportIdKeyboard)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(kReportIdMouse)),
    TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(kReportIdConsumer)),
};

// USB configuration descriptor.
static const uint8_t kHidConfigurationDescriptor[] = {
    // Configuration: number, interface count, string index, total length,
    // attribute, power in mA.
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN, 0,
                          100),

    // Interface: number, string index, protocol, report descriptor len,
    // EP In address, size & polling interval.
    TUD_HID_DESCRIPTOR(0, 0, false, sizeof(kHidReportDescriptor), 0x81, 16, 10),
};

// =============================================================================
// Static Variables
// =============================================================================

// Mutex for thread-safe HID report access.
static SemaphoreHandle_t s_hid_mutex = NULL;

// USB connection state.
static bool s_usb_connected = false;

// Current keyboard report state.
static hid_keyboard_report_t s_keyboard_report = {0};

// Current mouse report state.
static hid_mouse_report_t s_mouse_report = {0};

// =============================================================================
// TinyUSB Callbacks
// =============================================================================

void tud_mount_cb(void)
{
    ESP_LOGI(kTag, "USB mounted");
    s_usb_connected = true;
}

void tud_umount_cb(void)
{
    ESP_LOGI(kTag, "USB unmounted");
    s_usb_connected = false;
}

void tud_suspend_cb(bool remote_wakeup_en)
{
    (void)remote_wakeup_en;
    ESP_LOGI(kTag, "USB suspended");
}

void tud_resume_cb(void)
{
    ESP_LOGI(kTag, "USB resumed");
}

uint8_t const* tud_hid_descriptor_report_cb(uint8_t itf)
{
    (void)itf;
    return kHidReportDescriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t* buffer,
                               uint16_t reqlen)
{
    (void)itf;
    (void)report_type;
    (void)reqlen;

    switch (report_id) {
        case kReportIdKeyboard:
            memcpy(buffer, &s_keyboard_report, sizeof(s_keyboard_report));
            return sizeof(s_keyboard_report);

        case kReportIdMouse:
            memcpy(buffer, &s_mouse_report, sizeof(s_mouse_report));
            return sizeof(s_mouse_report);

        default:
            return 0;
    }
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const* buffer, uint16_t bufsize)
{
    (void)itf;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
}

// =============================================================================
// Internal Functions
// =============================================================================

// USB device task that runs TinyUSB stack.
static void UsbDeviceTask(void* arg)
{
    (void)arg;

    while (1) {
        tud_task();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// =============================================================================
// Public Functions
// =============================================================================

void UsbHidDeviceInit(void)
{
    ESP_LOGI(kTag, "Initializing USB HID device");

    // Create mutex for thread safety.
    s_hid_mutex = xSemaphoreCreateMutex();

    // USB device descriptor.
    static tusb_desc_device_t descriptor = {
        .bLength            = sizeof(descriptor),
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
        .bNumConfigurations = 0x01,
    };

    // Language descriptor for USB strings.
    static const char kLanguageDescriptor[] = {0x09, 0x04};

    // USB string descriptors.
    static const char* kStringDescriptor[] = {
        kLanguageDescriptor,   // Language
        "BMW",                 // Manufacturer
        "iDrive Controller",   // Product
        "123456",              // Serial
        "HID Interface",       // Interface
    };

    // TinyUSB configuration.
    tinyusb_config_t tusb_cfg = {
        .device_descriptor        = &descriptor,
        .string_descriptor        = kStringDescriptor,
        .string_descriptor_count  = sizeof(kStringDescriptor) /
                                    sizeof(kStringDescriptor[0]),
        .external_phy             = false,
        .configuration_descriptor = kHidConfigurationDescriptor,
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    // Create USB task.
    xTaskCreate(UsbDeviceTask, "TinyUSB", 4096, NULL, 5, NULL);

    ESP_LOGI(kTag, "USB HID initialized");
}

void UsbHidKeyboardPress(uint8_t keycode)
{
    if (!s_usb_connected) {
        return;
    }

    if (xSemaphoreTake(s_hid_mutex, portMAX_DELAY) == pdTRUE) {
        // Find empty slot in keycode array.
        for (int i = 0; i < 6; i++) {
            if (s_keyboard_report.keycode[i] == 0) {
                s_keyboard_report.keycode[i] = keycode;
                break;
            }
        }

        tud_hid_n_report(0, kReportIdKeyboard, &s_keyboard_report,
                         sizeof(s_keyboard_report));
        xSemaphoreGive(s_hid_mutex);
    }
}

void UsbHidKeyboardRelease(uint8_t keycode)
{
    if (!s_usb_connected) {
        return;
    }

    if (xSemaphoreTake(s_hid_mutex, portMAX_DELAY) == pdTRUE) {
        // Remove keycode from array.
        for (int i = 0; i < 6; i++) {
            if (s_keyboard_report.keycode[i] == keycode) {
                s_keyboard_report.keycode[i] = 0;
                break;
            }
        }

        tud_hid_n_report(0, kReportIdKeyboard, &s_keyboard_report,
                         sizeof(s_keyboard_report));
        xSemaphoreGive(s_hid_mutex);
    }
}

void UsbHidKeyboardPressAndRelease(uint8_t keycode)
{
    UsbHidKeyboardPress(keycode);
    vTaskDelay(pdMS_TO_TICKS(50));
    UsbHidKeyboardRelease(keycode);
}

void UsbHidMediaKeyPress(uint16_t keycode)
{
    if (!s_usb_connected) {
        return;
    }

    uint16_t usage = keycode;
    tud_hid_n_report(0, kReportIdConsumer, &usage, 2);
}

void UsbHidMediaKeyRelease(uint16_t keycode)
{
    (void)keycode;

    if (!s_usb_connected) {
        return;
    }

    uint16_t usage = 0;
    tud_hid_n_report(0, kReportIdConsumer, &usage, 2);
}

void UsbHidMediaKeyPressAndRelease(uint16_t keycode)
{
    UsbHidMediaKeyPress(keycode);
    vTaskDelay(pdMS_TO_TICKS(50));
    UsbHidMediaKeyRelease(keycode);
}

void UsbHidMouseMove(int8_t x, int8_t y)
{
    if (!s_usb_connected) {
        return;
    }

    if (xSemaphoreTake(s_hid_mutex, portMAX_DELAY) == pdTRUE) {
        s_mouse_report.x = x;
        s_mouse_report.y = y;
        tud_hid_n_report(0, kReportIdMouse, &s_mouse_report,
                         sizeof(s_mouse_report));
        s_mouse_report.x = 0;
        s_mouse_report.y = 0;
        xSemaphoreGive(s_hid_mutex);
    }
}

void UsbHidMouseButtonPress(uint8_t button)
{
    if (!s_usb_connected) {
        return;
    }

    if (xSemaphoreTake(s_hid_mutex, portMAX_DELAY) == pdTRUE) {
        s_mouse_report.buttons |= button;
        tud_hid_n_report(0, kReportIdMouse, &s_mouse_report,
                         sizeof(s_mouse_report));
        xSemaphoreGive(s_hid_mutex);
    }
}

void UsbHidMouseButtonRelease(uint8_t button)
{
    if (!s_usb_connected) {
        return;
    }

    if (xSemaphoreTake(s_hid_mutex, portMAX_DELAY) == pdTRUE) {
        s_mouse_report.buttons &= ~button;
        tud_hid_n_report(0, kReportIdMouse, &s_mouse_report,
                         sizeof(s_mouse_report));
        xSemaphoreGive(s_hid_mutex);
    }
}

void UsbHidMouseClick(uint8_t button)
{
    UsbHidMouseButtonPress(button);
    vTaskDelay(pdMS_TO_TICKS(50));
    UsbHidMouseButtonRelease(button);
}

void UsbHidMouseScroll(int8_t wheel)
{
    if (!s_usb_connected) {
        return;
    }

    if (xSemaphoreTake(s_hid_mutex, portMAX_DELAY) == pdTRUE) {
        s_mouse_report.wheel = wheel;
        tud_hid_n_report(0, kReportIdMouse, &s_mouse_report,
                         sizeof(s_mouse_report));
        s_mouse_report.wheel = 0;
        xSemaphoreGive(s_hid_mutex);
    }
}

bool UsbHidIsConnected(void)
{
    return s_usb_connected && tud_ready();
}
