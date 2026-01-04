// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT

#include "ota/wifi_ap.h"

#include <cstring>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "ota/ota_config.h"

namespace idrive::ota {

namespace {
const char* kTag = "WIFI_AP";
}

bool WifiAp::Start() {
    if (running_) {
        return true;
    }

    ESP_LOGI(kTag, "Starting WiFi AP...");

    // Initialize NVS (required for WiFi).
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize network interface.
    if (!initialized_) {
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        esp_netif_create_default_wifi_ap();
        initialized_ = true;
    }

    // Initialize WiFi with default config.
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Configure AP.
    wifi_config_t wifi_config = {};
    std::strncpy(reinterpret_cast<char*>(wifi_config.ap.ssid),
                 config::kApSsid, sizeof(wifi_config.ap.ssid));
    std::strncpy(reinterpret_cast<char*>(wifi_config.ap.password),
                 config::kApPassword, sizeof(wifi_config.ap.password));
    wifi_config.ap.ssid_len = std::strlen(config::kApSsid);
    wifi_config.ap.channel = config::kApChannel;
    wifi_config.ap.max_connection = config::kApMaxConnections;
    wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    running_ = true;
    ESP_LOGI(kTag, "WiFi AP started: SSID='%s', Password='%s'",
             config::kApSsid, config::kApPassword);
    ESP_LOGI(kTag, "Connect to http://%s", GetIpAddress());

    return true;
}

void WifiAp::Stop() {
    if (!running_) {
        return;
    }

    ESP_LOGI(kTag, "Stopping WiFi AP...");
    esp_wifi_stop();
    esp_wifi_deinit();
    running_ = false;
}

}  // namespace idrive::ota
