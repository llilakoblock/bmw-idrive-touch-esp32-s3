// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// WiFi Access Point for OTA updates.

#pragma once

#include <cstdint>

namespace idrive::ota {

class WifiAp {
   public:
    // Start WiFi AP with configured credentials.
    bool Start();

    // Stop WiFi AP and deinitialize.
    void Stop();

    // Check if AP is running.
    bool IsRunning() const { return running_; }

    // Get the AP IP address.
    const char *GetIpAddress() const { return "192.168.4.1"; }

   private:
    bool running_     = false;
    bool initialized_ = false;
};

}  // namespace idrive::ota
