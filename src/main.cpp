// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// Main application entry point for ESP32-S3 iDrive controller adapter.
// Initializes CAN bus, USB HID, and runs the main control loop.

#include "can/can_bus.h"
#include "config/config.h"
#include "hid/usb_hid_device.h"
#include "idrive/idrive_controller.h"

#include "esp_log.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {
const char* kTag = "MAIN";
}

extern "C" void app_main() {
    ESP_LOGI(kTag, "BMW iDrive Touch Adapter - Starting...");
    ESP_LOGI(kTag, "Modern C++17 Architecture");

    // Subscribe to watchdog.
    esp_task_wdt_add(nullptr);

    // Create CAN bus instance.
    idrive::CanBus can(GPIO_NUM_4, GPIO_NUM_5);

    // Get USB HID device instance.
    idrive::UsbHidDevice& hid = idrive::GetUsbHidDevice();

    // Configuration.
    idrive::Config config{
        .joystick_as_mouse = true,
        .light_brightness = 100,
        .poll_interval_ms = idrive::config::kPollIntervalMs,
        .light_keepalive_ms = idrive::config::kLightKeepaliveMs,
        .min_mouse_travel = idrive::config::kMinMouseTravel,
        .joystick_move_step = idrive::config::kJoystickMoveStep,
    };

    // Create iDrive controller.
    idrive::IDriveController controller(can, hid, config);

    // Initialize USB HID device.
    if (!hid.Init()) {
        ESP_LOGE(kTag, "Failed to initialize USB HID device");
        return;
    }
    ESP_LOGI(kTag, "USB HID device initialized");

    // Allow USB enumeration to complete.
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Initialize CAN bus.
    if (!can.Init(idrive::config::kCanBaudrate)) {
        ESP_LOGE(kTag, "Failed to initialize CAN bus");
        return;
    }
    ESP_LOGI(kTag, "CAN bus initialized at %lu bps", idrive::config::kCanBaudrate);

    // Wait for bus stabilization.
    vTaskDelay(pdMS_TO_TICKS(500));

    // Initialize iDrive controller.
    controller.Init();

    ESP_LOGI(kTag, "Entering main loop...");

    // Main loop.
    while (true) {
        // Reset watchdog.
        esp_task_wdt_reset();

        // Process CAN bus (receives messages and dispatches to controller).
        can.ProcessAlerts();

        // Update controller state (handles timing, polling, etc.).
        controller.Update();

        // Yield to other tasks.
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
