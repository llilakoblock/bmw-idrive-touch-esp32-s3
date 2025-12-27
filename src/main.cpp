// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// Main application entry point for ESP32-S3 iDrive controller adapter.
// Initializes CAN bus (TWAI), USB HID, and runs the main control loop.

#include <cstdio>

#include "driver/twai.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "idrive.h"
#include "settings.h"
#include "usb_hid_device.h"
#include "variables.h"

// =============================================================================
// Pin Configuration
// =============================================================================

namespace {

constexpr gpio_num_t kCanRxPin = GPIO_NUM_4;
constexpr gpio_num_t kCanTxPin = GPIO_NUM_5;

const char* kTag = "MAIN";

// Retry interval for initialization (milliseconds).
constexpr uint32_t kInitRetryIntervalMs = 5000;

}  // namespace

// =============================================================================
// Helper Functions
// =============================================================================

// Returns current time in milliseconds since boot.
static uint32_t GetMillis()
{
    return static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
}

// Initializes the TWAI (CAN) driver at 500 kbps.
static void SetupTwai()
{
    twai_general_config_t general_config = {};
    general_config.mode                  = TWAI_MODE_NORMAL;
    general_config.tx_io                 = kCanTxPin;
    general_config.rx_io                 = kCanRxPin;
    general_config.clkout_io             = GPIO_NUM_NC;
    general_config.bus_off_io            = GPIO_NUM_NC;
    general_config.tx_queue_len          = 10;
    general_config.rx_queue_len          = 10;
    general_config.alerts_enabled        = TWAI_ALERT_ALL;
    general_config.clkout_divider        = 0;
    general_config.intr_flags            = ESP_INTR_FLAG_LEVEL1;

    twai_timing_config_t timing_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t filter_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    esp_err_t err = twai_driver_install(&general_config, &timing_config,
                                        &filter_config);
    if (err == ESP_OK) {
        ESP_LOGI(kTag, "TWAI driver installed");
    } else {
        ESP_LOGE(kTag, "TWAI driver installation failed: %s",
                 esp_err_to_name(err));
        return;
    }

    err = twai_start();
    if (err == ESP_OK) {
        ESP_LOGI(kTag, "TWAI driver started");
    } else {
        ESP_LOGE(kTag, "TWAI driver start failed: %s", esp_err_to_name(err));
    }
}

// Handles CAN bus alerts and recovers from errors.
static void HandleCanAlerts()
{
    uint32_t alerts;
    if (twai_read_alerts(&alerts, 0) != ESP_OK || alerts == 0) {
        return;
    }

    if (alerts & TWAI_ALERT_ERR_PASS) {
        ESP_LOGW(kTag, "CAN: Error passive state");
    }

    if (alerts & TWAI_ALERT_BUS_OFF) {
        ESP_LOGE(kTag, "CAN: Bus off state - attempting recovery");
        twai_initiate_recovery();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    if (alerts & TWAI_ALERT_TX_FAILED) {
        ESP_LOGW(kTag, "CAN: TX failed");
    }

    if (alerts & TWAI_ALERT_RX_QUEUE_FULL) {
        ESP_LOGW(kTag, "CAN: RX queue full");
    }
}

// =============================================================================
// Main Application
// =============================================================================

extern "C" void app_main(void)
{
    ESP_LOGI(kTag, "ESP-IDF iDrive Controller - Starting...");

    // Subscribe to watchdog.
    esp_task_wdt_add(NULL);

    // Initialize timestamp.
    g_previous_millis = GetMillis();

    // Initialize USB HID device.
    UsbHidDeviceInit();
    ESP_LOGI(kTag, "USB HID device initialized");

    // Allow USB enumeration to complete.
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Initialize CAN bus.
    SetupTwai();

    // Wait for bus stabilization.
    vTaskDelay(pdMS_TO_TICKS(500));

    // Initialize rotary encoder first.
    IDriveInit();

    // Enable backlight.
    IDriveLightSend();

    ESP_LOGI(kTag, "Entering main loop...");

    // Track touchpad initialization state.
    bool touchpad_init_sent = false;
    uint32_t last_reinit_time = 0;

    // Main loop.
    while (true) {
        // Reset watchdog.
        esp_task_wdt_reset();

        // Handle CAN bus errors.
        HandleCanAlerts();

        // Process incoming CAN messages.
        twai_message_t message;
        while (twai_receive(&message, 0) == ESP_OK) {
            DecodeCanMessage(message.identifier, message.data_length_code,
                             message.data);
        }

        // Initialize touchpad after rotary encoder is ready.
        if (g_rotary_init_success && !touchpad_init_sent) {
            ESP_LOGI(kTag, "Rotary init done, initializing touchpad");
            vTaskDelay(pdMS_TO_TICKS(100));
            IDriveTouchpadInit();
            g_touchpad_init_done = true;
            touchpad_init_sent = true;
        }

        // Mark controller as ready after cooldown.
        if (!g_controller_ready && g_rotary_init_success && g_touchpad_init_done) {
            if (g_cooldown_millis == 0) {
                g_cooldown_millis = GetMillis();
            }
            if (GetMillis() - g_cooldown_millis > kControllerCooldownMs) {
                g_controller_ready = true;
                ESP_LOGI(kTag, "iDrive controller ready!");
                ESP_LOGI(kTag, "TouchpadInitDone=%d, RotaryInitSuccess=%d",
                         g_touchpad_init_done, g_rotary_init_success);
            }
        }

        // Track light initialization completion.
        uint32_t now = GetMillis();
        if (!g_light_init_done) {
            if (now - g_previous_millis > kLightInitDurationMs) {
                g_light_init_done = true;
                ESP_LOGI(kTag, "Light init done");
            }
        }

        // Periodic polling.
        IDrivePoll(kPollIntervalMs);

        // Periodic light keepalive.
        IDriveLight(kLightKeepaliveIntervalMs);

        // Retry initialization if no response.
        if (!g_rotary_init_success && (now - last_reinit_time >= kInitRetryIntervalMs)) {
            last_reinit_time = now;
            ESP_LOGW(kTag, "No init response - retrying...");
            IDriveInit();
            touchpad_init_sent = false;
        }

        // Yield to other tasks.
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
