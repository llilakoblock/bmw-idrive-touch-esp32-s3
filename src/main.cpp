#include <cstdio>

#include "settings.h"
#include "variables.h"
#include "driver/twai.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "idrive.h"

#define RX_PIN 4
#define TX_PIN 5

static const char* TAG = "MAIN";

// Helper to get current time in milliseconds (similar to Arduino millis())
static uint32_t getMillis() {
  return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

static void setupTWAI() {
  // For 500 kbps
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
      (gpio_num_t)TX_PIN, (gpio_num_t)RX_PIN, TWAI_MODE_NORMAL);

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

  // // Reconfigure alerts to detect TX alerts and Bus-Off errors
  // uint32_t alerts_to_enable = TWAI_ALERT_TX_IDLE | TWAI_ALERT_TX_SUCCESS |
  //                             TWAI_ALERT_TX_FAILED | TWAI_ALERT_ERR_PASS |
  //                             TWAI_ALERT_BUS_ERROR;

  // if (twai_reconfigure_alerts(alerts_to_enable, NULL) == ESP_OK) {
  //   ESP_LOGI(TAG, "CAN Alerts reconfigured");
  // } else {
  //   ESP_LOGI(TAG, "Failed to reconfigure alerts");
  //   return;
  // }

  // twai_status_info_t status;
  // twai_get_status_info(&status);
  // ESP_LOGI(TAG, "TWAI state 0x%X", status.state);
}

extern "C" void app_main(void) {
  ESP_LOGI(TAG, "ESP-IDF iDrive Project - Starting up...");

  // Initialize global variables
  previousMillis = getMillis();

  setupTWAI();

  iDriveInit();
  iDriveTouchpadInit();

  // Enter main loop
  while (true) {
    // If we haven't done so, check if controller can be marked ready
    if (!controllerReady && RotaryInitSuccess && TouchpadInitDone) {
      if (CoolDownMillis == 0) CoolDownMillis = getMillis();
      if (getMillis() - CoolDownMillis > controllerCoolDown) {
        controllerReady = true;
      }
    }

    // Receive any incoming CAN frames (non-blocking)
    twai_message_t message;
    esp_err_t ret = twai_receive(&message, 0);  // timeout=0 => non-blocking
    if (ret == ESP_OK) {
      // decode
      decodeCanBus(message.identifier, message.data_length_code, message.data);
    }

    // Periodic tasks
    uint32_t now = getMillis();

    // Light init logic
    if (!LightInitDone) {
      if (previousMillis == 0) previousMillis = now;
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
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
