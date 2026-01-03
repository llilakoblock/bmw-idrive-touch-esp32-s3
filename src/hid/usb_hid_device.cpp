// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT

#include "hid/usb_hid_device.h"

#include <cstring>

#include "class/hid/hid_device.h"
#include "config/config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"

namespace idrive {

namespace {

const char* kTag = "USB_HID";

// Report IDs for different HID functions.
enum ReportId {
    kReportIdKeyboard = 1,
    kReportIdMouse = 2,
    kReportIdConsumer = 3,
};

// Combined HID report descriptor for keyboard, mouse, and consumer controls.
const uint8_t kHidReportDescriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(kReportIdKeyboard)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(kReportIdMouse)),
    TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(kReportIdConsumer)),
};

// USB configuration descriptor.
const uint8_t kHidConfigurationDescriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN, 0,
                          100),
    TUD_HID_DESCRIPTOR(0, 0, false, sizeof(kHidReportDescriptor), 0x81, 16, 10),
};

// Global instance for TinyUSB callbacks.
UsbHidDevice* g_usb_hid_instance = nullptr;

// USB device task.
void UsbDeviceTask(void* arg) {
    (void)arg;
    while (true) {
        tud_task();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

}  // namespace

// =============================================================================
// Global Instance Access
// =============================================================================

UsbHidDevice& GetUsbHidDevice() {
    static UsbHidDevice instance;
    return instance;
}

// =============================================================================
// TinyUSB Callbacks
// =============================================================================

extern "C" {

void tud_mount_cb(void) {
    ESP_LOGI(kTag, "USB mounted");
    if (g_usb_hid_instance) {
        g_usb_hid_instance->OnMount();
    }
}

void tud_umount_cb(void) {
    ESP_LOGI(kTag, "USB unmounted");
    if (g_usb_hid_instance) {
        g_usb_hid_instance->OnUnmount();
    }
}

void tud_suspend_cb(bool remote_wakeup_en) {
    (void)remote_wakeup_en;
    ESP_LOGI(kTag, "USB suspended");
}

void tud_resume_cb(void) {
    ESP_LOGI(kTag, "USB resumed");
}

uint8_t const* tud_hid_descriptor_report_cb(uint8_t itf) {
    (void)itf;
    return kHidReportDescriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t* buffer,
                               uint16_t reqlen) {
    (void)itf;
    (void)report_type;
    (void)reqlen;
    (void)report_id;
    (void)buffer;
    return 0;
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const* buffer, uint16_t bufsize) {
    (void)itf;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
}

}  // extern "C"

// =============================================================================
// UsbHidDevice Implementation
// =============================================================================

bool UsbHidDevice::Init() {
    ESP_LOGI(kTag, "Initializing USB HID device");

    mutex_ = xSemaphoreCreateMutex();
    if (!mutex_) {
        ESP_LOGE(kTag, "Failed to create mutex");
        return false;
    }

    g_usb_hid_instance = this;

    // USB device descriptor.
    static tusb_desc_device_t descriptor = {
        .bLength = sizeof(descriptor),
        .bDescriptorType = TUSB_DESC_DEVICE,
        .bcdUSB = 0x0200,
        .bDeviceClass = 0,
        .bDeviceSubClass = 0,
        .bDeviceProtocol = 0,
        .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
        .idVendor = config::kUsbVendorId,
        .idProduct = config::kUsbProductId,
        .bcdDevice = 0x0100,
        .iManufacturer = 0x01,
        .iProduct = 0x02,
        .iSerialNumber = 0x03,
        .bNumConfigurations = 0x01,
    };

    static const char kLanguageDescriptor[] = {0x09, 0x04};
    static const char* kStringDescriptor[] = {
        kLanguageDescriptor,
        config::kUsbManufacturer,
        config::kUsbProduct,
        config::kUsbSerialNumber,
        "HID Interface",
    };

    tinyusb_config_t tusb_cfg = {
        .device_descriptor = &descriptor,
        .string_descriptor = kStringDescriptor,
        .string_descriptor_count =
            sizeof(kStringDescriptor) / sizeof(kStringDescriptor[0]),
        .external_phy = false,
        .configuration_descriptor = kHidConfigurationDescriptor,
    };

    esp_err_t err = tinyusb_driver_install(&tusb_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "TinyUSB driver install failed: %s",
                 esp_err_to_name(err));
        return false;
    }

    xTaskCreate(UsbDeviceTask, "TinyUSB", 4096, nullptr, 5, nullptr);

    initialized_ = true;
    ESP_LOGI(kTag, "USB HID initialized");
    return true;
}

bool UsbHidDevice::IsConnected() const {
    return connected_ && tud_ready();
}

void UsbHidDevice::OnMount() {
    connected_ = true;
}

void UsbHidDevice::OnUnmount() {
    connected_ = false;
}

// =============================================================================
// Keyboard Functions
// =============================================================================

void UsbHidDevice::KeyPress(uint8_t keycode) {
    if (!IsConnected()) return;

    if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE) {
        for (int i = 0; i < 6; ++i) {
            if (keyboard_report_.keycode[i] == 0) {
                keyboard_report_.keycode[i] = keycode;
                break;
            }
        }
        SendKeyboardReport();
        xSemaphoreGive(mutex_);
    }
}

void UsbHidDevice::KeyRelease(uint8_t keycode) {
    if (!IsConnected()) return;

    if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE) {
        for (int i = 0; i < 6; ++i) {
            if (keyboard_report_.keycode[i] == keycode) {
                keyboard_report_.keycode[i] = 0;
                break;
            }
        }
        SendKeyboardReport();
        xSemaphoreGive(mutex_);
    }
}

void UsbHidDevice::KeyPressAndRelease(uint8_t keycode) {
    KeyPress(keycode);
    vTaskDelay(pdMS_TO_TICKS(50));
    KeyRelease(keycode);
}

void UsbHidDevice::SendKeyboardReport() {
    tud_hid_n_report(0, kReportIdKeyboard, &keyboard_report_,
                     sizeof(keyboard_report_));
}

// =============================================================================
// Media Control Functions
// =============================================================================

void UsbHidDevice::MediaKeyPress(uint16_t keycode) {
    if (!IsConnected()) return;

    uint16_t usage = keycode;
    tud_hid_n_report(0, kReportIdConsumer, &usage, sizeof(usage));
}

void UsbHidDevice::MediaKeyRelease(uint16_t keycode) {
    (void)keycode;
    if (!IsConnected()) return;

    uint16_t usage = 0;
    tud_hid_n_report(0, kReportIdConsumer, &usage, sizeof(usage));
}

void UsbHidDevice::MediaKeyPressAndRelease(uint16_t keycode) {
    MediaKeyPress(keycode);
    vTaskDelay(pdMS_TO_TICKS(50));
    MediaKeyRelease(keycode);
}

// =============================================================================
// Mouse Functions
// =============================================================================

void UsbHidDevice::MouseMove(int8_t x, int8_t y) {
    if (!IsConnected()) return;

    if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE) {
        mouse_report_.x = x;
        mouse_report_.y = y;
        SendMouseReport();
        mouse_report_.x = 0;
        mouse_report_.y = 0;
        xSemaphoreGive(mutex_);
    }
}

void UsbHidDevice::MouseButtonPress(uint8_t button) {
    if (!IsConnected()) return;

    if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE) {
        mouse_report_.buttons |= button;
        SendMouseReport();
        xSemaphoreGive(mutex_);
    }
}

void UsbHidDevice::MouseButtonRelease(uint8_t button) {
    if (!IsConnected()) return;

    if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE) {
        mouse_report_.buttons &= ~button;
        SendMouseReport();
        xSemaphoreGive(mutex_);
    }
}

void UsbHidDevice::MouseClick(uint8_t button) {
    MouseButtonPress(button);
    vTaskDelay(pdMS_TO_TICKS(50));
    MouseButtonRelease(button);
}

void UsbHidDevice::MouseScroll(int8_t wheel) {
    if (!IsConnected()) return;

    if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE) {
        mouse_report_.wheel = wheel;
        SendMouseReport();
        mouse_report_.wheel = 0;
        xSemaphoreGive(mutex_);
    }
}

void UsbHidDevice::SendMouseReport() {
    tud_hid_n_report(0, kReportIdMouse, &mouse_report_, sizeof(mouse_report_));
}

}  // namespace idrive
