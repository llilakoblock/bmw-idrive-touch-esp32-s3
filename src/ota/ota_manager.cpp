// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT

#include "ota/ota_manager.h"

#include "esp_log.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ota/ota_config.h"

namespace idrive::ota {

namespace {
const char* kTag = "OTA_MANAGER";
}

OtaManager::OtaManager() = default;

void OtaManager::Init() {
    ESP_LOGI(kTag, "========================================");
    ESP_LOGI(kTag, "OTA Manager initialized");
    ESP_LOGI(kTag, "Hold Menu+Back for 3 seconds to enter OTA mode");
    ESP_LOGI(kTag, "========================================");

    // Set up trigger callback.
    trigger_.SetCallback([this]() { OnOtaTriggered(); });

    // Mark current firmware as valid (prevents rollback).
    esp_ota_mark_app_valid_cancel_rollback();
}

void OtaManager::Update() {
    trigger_.Update();
}

void OtaManager::OnOtaTriggered() {
    ESP_LOGI(kTag, "========================================");
    ESP_LOGI(kTag, "OTA MODE TRIGGERED!");
    ESP_LOGI(kTag, "========================================");

    EnterOtaMode();
}

void OtaManager::EnterOtaMode() {
    if (ota_mode_active_) {
        return;
    }

    ota_mode_active_ = true;

    // Start WiFi AP.
    ESP_LOGI(kTag, "Starting WiFi AP...");
    if (!wifi_.Start()) {
        ESP_LOGE(kTag, "Failed to start WiFi AP");
        ota_mode_active_ = false;
        return;
    }

    // Start web server.
    server_.SetOtaCompleteCallback([this](bool success) {
        OnOtaComplete(success);
    });

    if (!server_.Start()) {
        ESP_LOGE(kTag, "Failed to start web server");
        wifi_.Stop();
        ota_mode_active_ = false;
        return;
    }

    ESP_LOGI(kTag, "========================================");
    ESP_LOGI(kTag, "OTA mode active!");
    ESP_LOGI(kTag, "Connect to WiFi: %s", config::kApSsid);
    ESP_LOGI(kTag, "Password: %s", config::kApPassword);
    ESP_LOGI(kTag, "Open: http://%s", wifi_.GetIpAddress());
    ESP_LOGI(kTag, "========================================");
}

void OtaManager::ExitOtaMode() {
    if (!ota_mode_active_) {
        return;
    }

    ESP_LOGI(kTag, "Exiting OTA mode...");
    server_.Stop();
    wifi_.Stop();
    ota_mode_active_ = false;
}

void OtaManager::OnOtaComplete(bool success) {
    if (success) {
        ESP_LOGI(kTag, "========================================");
        ESP_LOGI(kTag, "OTA SUCCESSFUL!");
        ESP_LOGI(kTag, "Rebooting to new firmware...");
        ESP_LOGI(kTag, "========================================");

        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    } else {
        ESP_LOGE(kTag, "OTA failed - staying in OTA mode");
    }
}

}  // namespace idrive::ota
