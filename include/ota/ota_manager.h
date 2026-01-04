// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// OTA Manager - orchestrates WiFi AP and web server for firmware updates.

#pragma once

#include "ota/ota_trigger.h"
#include "ota/web_server.h"
#include "ota/wifi_ap.h"

namespace idrive::ota {

class OtaManager {
public:
    OtaManager();

    // Initialize OTA subsystem (sets up trigger detection).
    void Init();

    // Call regularly to update trigger detection.
    void Update();

    // Manually enter OTA mode.
    void EnterOtaMode();

    // Exit OTA mode and reboot.
    void ExitOtaMode();

    // Check if in OTA mode.
    bool IsOtaModeActive() const { return ota_mode_active_; }

    // Get trigger for integration with IDriveController.
    OtaTrigger& GetTrigger() { return trigger_; }

private:
    WifiAp wifi_;
    WebServer server_;
    OtaTrigger trigger_;

    bool ota_mode_active_ = false;

    void OnOtaTriggered();
    void OnOtaComplete(bool success);
};

}  // namespace idrive::ota
