// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// OTA trigger detection - detects Menu+Back button hold combo.

#pragma once

#include <cstdint>
#include <functional>

namespace idrive::ota {

// Callback when OTA mode is triggered.
using OtaTriggerCallback = std::function<void()>;

class OtaTrigger {
   public:
    // Set callback for when OTA mode is triggered.
    void SetCallback(OtaTriggerCallback callback);

    // Call this when button events occur (from IDriveController).
    void OnButtonEvent(uint8_t button_id, uint8_t state);

    // Call regularly to check timing.
    void Update();

    // Check if currently detecting combo.
    bool IsDetecting() const { return detecting_; }

   private:
    OtaTriggerCallback callback_;

    bool     menu_held_        = false;
    bool     back_held_        = false;
    bool     detecting_        = false;
    bool     triggered_        = false;
    uint32_t combo_start_time_ = 0;
};

}  // namespace idrive::ota
