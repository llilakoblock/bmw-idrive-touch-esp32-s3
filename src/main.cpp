// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// Main application entry point for ESP32-S3 iDrive controller adapter.
// Initializes CAN bus, USB HID, and runs the main control loop.

#include "can/can_bus.h"
#include "can/can_task.h"
#include "config/config.h"
#include "hid/usb_hid_device.h"
#include "idrive/idrive_controller.h"
#include "ota/ota_manager.h"

#include "esp_log.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {
const char* kTag = "MAIN";
}

extern "C" void app_main() {
    ESP_LOGI(kTag, "BMW iDrive Touch Adapter - Starting...");
    ESP_LOGI(kTag, "Modern C++17 Architecture with OTA Support");
    ESP_LOGI(kTag, "Main task running on core %d", xPortGetCoreID());

    // Subscribe to watchdog.
    esp_task_wdt_add(nullptr);

    // Create OTA manager (before other initializations).
    idrive::ota::OtaManager ota_manager;

    // Create CAN bus instance.
    idrive::CanBus can(GPIO_NUM_4, GPIO_NUM_5);

    // Create event-driven CAN task.
    idrive::CanTask can_task(can);

    // Get USB HID device instance.
    idrive::UsbHidDevice& hid = idrive::GetUsbHidDevice();

    // Configuration.
    idrive::Config config {
        .joystick_as_mouse  = false,  // Arrow keys mode (volume/tracks on steering wheel)
        .light_brightness   = 255,
        .poll_interval_ms   = idrive::config::kPollIntervalMs,
        .light_keepalive_ms = idrive::config::kLightKeepaliveMs,
        .min_mouse_travel   = idrive::config::kMinMouseTravel,
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

    // Start event-driven CAN task on Core 1 with high priority.
    if (!can_task.Start()) {
        ESP_LOGE(kTag, "Failed to start CAN task");
        return;
    }

    // Wait for bus stabilization.
    vTaskDelay(pdMS_TO_TICKS(500));

    // Initialize iDrive controller.
    controller.Init();

    // Initialize OTA manager and connect trigger to controller.
    ota_manager.Init();
    controller.SetOtaTrigger(&ota_manager.GetTrigger());

    ESP_LOGI(kTag, "Entering main loop...");
    ESP_LOGI(kTag, "Task distribution: USB on Core 0, CAN on Core 1");

    // Main loop - CAN processing is now handled by dedicated task.
    while (true) {
        // Reset watchdog.
        esp_task_wdt_reset();

        // Check if we're in OTA mode.
        if (ota_manager.IsOtaModeActive()) {
            // In OTA mode, skip normal operation.
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // CAN processing is now event-driven in CAN task (Core 1).
        // Main loop only handles periodic controller updates.

        // Update controller state (handles timing, polling commands, etc.).
        controller.Update();

        // Update OTA trigger detection.
        ota_manager.Update();

        // Yield to other tasks - can be slower now since CAN is event-driven.
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
