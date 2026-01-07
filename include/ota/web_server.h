// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// HTTP web server for OTA firmware upload.

#pragma once

#include <functional>

#include "esp_http_server.h"

namespace idrive::ota {

// Callback for OTA completion.
using OtaCompleteCallback = std::function<void(bool success)>;

class WebServer {
   public:
    // Start HTTP server.
    bool Start();

    // Stop HTTP server.
    void Stop();

    // Set callback for OTA completion.
    void SetOtaCompleteCallback(OtaCompleteCallback callback);

    // Check if server is running.
    bool IsRunning() const { return server_ != nullptr; }

   private:
    httpd_handle_t             server_ = nullptr;
    static OtaCompleteCallback ota_complete_callback_;

    // HTTP handlers (static for ESP-IDF C callback).
    static esp_err_t HandleRoot(httpd_req_t *req);
    static esp_err_t HandleUpload(httpd_req_t *req);
    static esp_err_t HandleReboot(httpd_req_t *req);
};

}  // namespace idrive::ota
