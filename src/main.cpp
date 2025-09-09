#include <cstdio>

#include "driver/twai.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "idrive.h"
#include "settings.h"
#include "usb_hid_device.h"
#include "variables.h"

#define RX_PIN 4
#define TX_PIN 5

static const char *TAG = "MAIN";

// Helper to get current time in milliseconds (similar to Arduino millis())
static uint32_t getMillis()
{
    return (uint32_t) (esp_timer_get_time() / 1000ULL);
}

static void setupTWAI()
{
    // For 500 kbps
    twai_general_config_t g_config = {};
    g_config.mode                  = TWAI_MODE_NORMAL;
    g_config.tx_io                 = (gpio_num_t) TX_PIN;
    g_config.rx_io                 = (gpio_num_t) RX_PIN;
    g_config.clkout_io             = GPIO_NUM_NC;
    g_config.bus_off_io            = GPIO_NUM_NC;
    g_config.tx_queue_len          = 5;
    g_config.rx_queue_len          = 5;
    g_config.alerts_enabled        = TWAI_ALERT_NONE;
    g_config.clkout_divider        = 0;
    g_config.intr_flags            = ESP_INTR_FLAG_LEVEL1;

    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();

    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    esp_err_t err = twai_driver_install(&g_config, &t_config, &f_config);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "TWAI driver installed");
    } else {
        ESP_LOGE(TAG, "TWAI driver installation failed: %s", esp_err_to_name(err));
        return;
    }

    err = twai_start();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "TWAI driver started");
    } else {
        ESP_LOGE(TAG, "TWAI driver start failed: %s", esp_err_to_name(err));
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "ESP-IDF iDrive Project - Starting up...");

    // Initialize global variables
    previousMillis = getMillis();

    // Initialize USB HID device
    usb_hid_device_init();
    ESP_LOGI(TAG, "USB HID device initialized");

    // Small delay to let USB enumerate
    vTaskDelay(pdMS_TO_TICKS(1000));

    setupTWAI();

    iDriveInit();
    iDriveTouchpadInit();

    // Enter main loop
    while (true) {
        // If we haven't done so, check if controller can be marked ready
        if (!controllerReady && RotaryInitSuccess && TouchpadInitDone) {
            if (CoolDownMillis == 0)
                CoolDownMillis = getMillis();
            if (getMillis() - CoolDownMillis > controllerCoolDown) {
                controllerReady = true;
                ESP_LOGI(TAG, "iDrive controller ready!");
            }
        }

        // Receive any incoming CAN frames (non-blocking)
        twai_message_t message;
        esp_err_t      ret = twai_receive(&message, 0);  // timeout=0 => non-blocking
        if (ret == ESP_OK) {
            // decode - this is where button presses and touchpad events are handled
            decodeCanBus(message.identifier, message.data_length_code, message.data);
        }

        // Periodic tasks
        uint32_t now = getMillis();

        // Light init logic
        if (!LightInitDone) {
            if (previousMillis == 0)
                previousMillis = now;
            if (now - previousMillis > iDriveInitLightTime) {
                LightInitDone = true;
            } else {
                do_iDriveLight();
            }
        }

        // iDrive Poll
        iDrivePoll(iDrivePollTime);

        // iDrive Light keepalive
        iDriveLight(iDriveLightTime);

        // Tiny delay so we don't hog the CPU
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}