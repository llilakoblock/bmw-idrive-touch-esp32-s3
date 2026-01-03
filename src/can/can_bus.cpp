// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT

#include "can/can_bus.h"

#include "driver/twai.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace idrive {

namespace {
const char* kTag = "CAN_BUS";
}

CanBus::CanBus(gpio_num_t rx_pin, gpio_num_t tx_pin)
    : rx_pin_(rx_pin), tx_pin_(tx_pin) {}

bool CanBus::Init(uint32_t baudrate) {
    ESP_LOGI(kTag, "Initializing CAN bus at %lu bps", baudrate);

    twai_general_config_t general_config = {};
    general_config.mode = TWAI_MODE_NORMAL;
    general_config.tx_io = tx_pin_;
    general_config.rx_io = rx_pin_;
    general_config.clkout_io = GPIO_NUM_NC;
    general_config.bus_off_io = GPIO_NUM_NC;
    general_config.tx_queue_len = 10;
    general_config.rx_queue_len = 10;
    general_config.alerts_enabled = TWAI_ALERT_ALL;
    general_config.clkout_divider = 0;
    general_config.intr_flags = ESP_INTR_FLAG_LEVEL1;

    // Configure timing based on baudrate.
    twai_timing_config_t timing_config;
    switch (baudrate) {
        case 500000:
            timing_config = TWAI_TIMING_CONFIG_500KBITS();
            break;
        case 250000:
            timing_config = TWAI_TIMING_CONFIG_250KBITS();
            break;
        case 125000:
            timing_config = TWAI_TIMING_CONFIG_125KBITS();
            break;
        case 1000000:
            timing_config = TWAI_TIMING_CONFIG_1MBITS();
            break;
        default:
            ESP_LOGE(kTag, "Unsupported baudrate: %lu", baudrate);
            return false;
    }

    twai_filter_config_t filter_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    esp_err_t err = twai_driver_install(&general_config, &timing_config,
                                        &filter_config);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "TWAI driver installation failed: %s",
                 esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(kTag, "TWAI driver installed");

    err = twai_start();
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "TWAI driver start failed: %s", esp_err_to_name(err));
        twai_driver_uninstall();
        return false;
    }

    ESP_LOGI(kTag, "TWAI driver started");
    initialized_ = true;
    return true;
}

bool CanBus::Send(uint32_t id, const uint8_t* data, uint8_t length,
                  bool extended) {
    if (!initialized_) {
        ESP_LOGW(kTag, "CAN bus not initialized");
        return false;
    }

    twai_message_t message = {};
    message.identifier = id;
    message.extd = extended ? 1 : 0;
    message.data_length_code = length;

    for (int i = 0; i < length && i < 8; ++i) {
        message.data[i] = data[i];
    }

    esp_err_t ret = twai_transmit(&message, pdMS_TO_TICKS(50));
    if (ret != ESP_OK) {
        ESP_LOGW(kTag, "CAN transmit failed: %s", esp_err_to_name(ret));
        return false;
    }

    return true;
}

bool CanBus::Send(const CanMessage& message) {
    return Send(message.id, message.data, message.length, message.extended);
}

void CanBus::SetCallback(MessageCallback callback) {
    callback_ = std::move(callback);
}

void CanBus::ProcessAlerts() {
    if (!initialized_) return;

    uint32_t alerts;
    if (twai_read_alerts(&alerts, 0) == ESP_OK && alerts != 0) {
        HandleAlerts(alerts);
    }

    ReceiveMessages();
}

void CanBus::HandleAlerts(uint32_t alerts) {
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

void CanBus::ReceiveMessages() {
    twai_message_t twai_msg;

    while (twai_receive(&twai_msg, 0) == ESP_OK) {
        if (callback_) {
            CanMessage msg;
            msg.id = twai_msg.identifier;
            msg.length = twai_msg.data_length_code;
            msg.extended = twai_msg.extd;

            for (int i = 0; i < msg.length && i < 8; ++i) {
                msg.data[i] = twai_msg.data[i];
            }

            callback_(msg);
        }
    }
}

}  // namespace idrive
