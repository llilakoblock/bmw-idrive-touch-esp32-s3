// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// CAN bus communication class using ESP32 TWAI driver.

#pragma once

#include <cstdint>
#include <functional>

#include "driver/gpio.h"

namespace idrive {

// =============================================================================
// CAN Message Structure
// =============================================================================

struct CanMessage {
    uint32_t id = 0;
    uint8_t data[8] = {0};
    uint8_t length = 0;
    bool extended = false;
};

// =============================================================================
// CAN Bus Class
// =============================================================================

class CanBus {
public:
    using MessageCallback = std::function<void(const CanMessage&)>;

    // Constructor with configurable pins.
    CanBus(gpio_num_t rx_pin = GPIO_NUM_4, gpio_num_t tx_pin = GPIO_NUM_5);

    // Initialize the CAN bus at specified baudrate.
    bool Init(uint32_t baudrate = 500000);

    // Send a CAN message.
    bool Send(uint32_t id, const uint8_t* data, uint8_t length,
              bool extended = false);

    // Send a CAN message using CanMessage struct.
    bool Send(const CanMessage& message);

    // Set callback for received messages.
    void SetCallback(MessageCallback callback);

    // Process CAN bus alerts and receive messages.
    // Call this regularly in the main loop.
    void ProcessAlerts();

    // Check if CAN bus is initialized.
    bool IsInitialized() const { return initialized_; }

private:
    gpio_num_t rx_pin_;
    gpio_num_t tx_pin_;
    MessageCallback callback_;
    bool initialized_ = false;

    void HandleAlerts(uint32_t alerts);
    void ReceiveMessages();
};

}  // namespace idrive
