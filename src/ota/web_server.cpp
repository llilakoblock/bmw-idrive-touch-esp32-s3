// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT

#include "ota/web_server.h"

#include <algorithm>
#include <cstring>

#include "esp_log.h"
#include "esp_ota_ops.h"
#include "ota/ota_config.h"

namespace idrive::ota {

namespace {
const char* kTag = "WEB_SERVER";

// Embedded HTML page for OTA upload.
const char kOtaHtml[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>iDrive OTA Update</title>
    <style>
        * { box-sizing: border-box; font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; }
        body { background: #1a1a2e; color: #eee; margin: 0; padding: 20px; min-height: 100vh; }
        .container { max-width: 500px; margin: 0 auto; }
        h1 { color: #0078d4; text-align: center; margin-bottom: 30px; }
        .card { background: #16213e; border-radius: 12px; padding: 24px; margin-bottom: 20px; }
        .file-input { display: none; }
        .upload-area {
            border: 2px dashed #0078d4; border-radius: 8px; padding: 40px;
            text-align: center; cursor: pointer; transition: all 0.3s;
        }
        .upload-area:hover { background: rgba(0,120,212,0.1); }
        .btn {
            background: #0078d4; color: white; border: none; padding: 14px 28px;
            border-radius: 6px; cursor: pointer; font-size: 16px; width: 100%;
            margin-top: 16px; transition: background 0.3s;
        }
        .btn:hover { background: #006cbd; }
        .btn:disabled { background: #555; cursor: not-allowed; }
        .progress-container { display: none; margin-top: 20px; }
        .progress-bar { height: 8px; background: #333; border-radius: 4px; overflow: hidden; }
        .progress-fill { height: 100%; background: #0078d4; width: 0%; transition: width 0.3s; }
        .status { margin-top: 16px; text-align: center; padding: 12px; border-radius: 6px; }
        .status.success { background: rgba(0,200,83,0.2); color: #00c853; }
        .status.error { background: rgba(255,82,82,0.2); color: #ff5252; }
        .info { font-size: 14px; color: #888; text-align: center; margin-top: 20px; }
        .filename { margin-top: 12px; color: #0078d4; word-break: break-all; }
    </style>
</head>
<body>
    <div class="container">
        <h1>BMW iDrive OTA Update</h1>
        <div class="card">
            <div class="upload-area" id="dropZone" onclick="document.getElementById('fileInput').click()">
                <svg width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="#0078d4" stroke-width="2">
                    <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/>
                    <polyline points="17 8 12 3 7 8"/>
                    <line x1="12" y1="3" x2="12" y2="15"/>
                </svg>
                <p>Click or drag firmware.bin here</p>
                <div class="filename" id="fileName"></div>
            </div>
            <input type="file" id="fileInput" class="file-input" accept=".bin">
            <button class="btn" id="uploadBtn" disabled>Upload Firmware</button>
            <div class="progress-container" id="progressContainer">
                <div class="progress-bar"><div class="progress-fill" id="progressFill"></div></div>
                <p id="progressText" style="text-align:center;margin-top:8px;">0%</p>
            </div>
            <div class="status" id="status" style="display:none;"></div>
        </div>
        <p class="info">Connected to: iDrive-OTA<br>Hold Menu+Back to exit OTA mode</p>
    </div>
    <script>
        const dropZone = document.getElementById('dropZone');
        const fileInput = document.getElementById('fileInput');
        const uploadBtn = document.getElementById('uploadBtn');
        const progressContainer = document.getElementById('progressContainer');
        const progressFill = document.getElementById('progressFill');
        const progressText = document.getElementById('progressText');
        const status = document.getElementById('status');
        const fileName = document.getElementById('fileName');
        let selectedFile = null;

        ['dragenter', 'dragover', 'dragleave', 'drop'].forEach(e => {
            dropZone.addEventListener(e, ev => { ev.preventDefault(); ev.stopPropagation(); });
        });
        dropZone.addEventListener('dragenter', () => dropZone.classList.add('dragover'));
        dropZone.addEventListener('dragleave', () => dropZone.classList.remove('dragover'));
        dropZone.addEventListener('drop', e => {
            dropZone.classList.remove('dragover');
            const files = e.dataTransfer.files;
            if (files.length) handleFile(files[0]);
        });
        fileInput.addEventListener('change', () => { if (fileInput.files.length) handleFile(fileInput.files[0]); });

        function handleFile(file) {
            if (!file.name.endsWith('.bin')) {
                showStatus('Please select a .bin file', 'error');
                return;
            }
            selectedFile = file;
            fileName.textContent = file.name + ' (' + (file.size / 1024).toFixed(1) + ' KB)';
            uploadBtn.disabled = false;
            status.style.display = 'none';
        }

        uploadBtn.addEventListener('click', async () => {
            if (!selectedFile) return;
            uploadBtn.disabled = true;
            progressContainer.style.display = 'block';
            status.style.display = 'none';

            const xhr = new XMLHttpRequest();
            xhr.open('POST', '/upload', true);

            xhr.upload.onprogress = e => {
                if (e.lengthComputable) {
                    const pct = Math.round((e.loaded / e.total) * 100);
                    progressFill.style.width = pct + '%';
                    progressText.textContent = pct + '%';
                }
            };

            xhr.onload = () => {
                if (xhr.status === 200) {
                    showStatus('Upload successful! Rebooting in 3 seconds...', 'success');
                    setTimeout(() => fetch('/reboot', {method: 'POST'}), 3000);
                } else {
                    showStatus('Upload failed: ' + xhr.statusText, 'error');
                    uploadBtn.disabled = false;
                }
            };

            xhr.onerror = () => {
                showStatus('Upload failed: Network error', 'error');
                uploadBtn.disabled = false;
            };

            xhr.send(selectedFile);
        });

        function showStatus(msg, type) {
            status.textContent = msg;
            status.className = 'status ' + type;
            status.style.display = 'block';
        }
    </script>
</body>
</html>
)rawliteral";

}  // namespace

// Static member definition.
OtaCompleteCallback WebServer::ota_complete_callback_;

bool WebServer::Start() {
    if (server_) {
        return true;
    }

    ESP_LOGI(kTag, "Starting HTTP server...");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    config.max_uri_handlers = 4;

    esp_err_t ret = httpd_start(&server_, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(kTag, "Failed to start HTTP server: %s", esp_err_to_name(ret));
        return false;
    }

    // Register URI handlers.
    httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = HandleRoot,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(server_, &root);

    httpd_uri_t upload = {
        .uri = "/upload",
        .method = HTTP_POST,
        .handler = HandleUpload,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(server_, &upload);

    httpd_uri_t reboot = {
        .uri = "/reboot",
        .method = HTTP_POST,
        .handler = HandleReboot,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(server_, &reboot);

    ESP_LOGI(kTag, "HTTP server started on port %d", config.server_port);
    return true;
}

void WebServer::Stop() {
    if (server_) {
        httpd_stop(server_);
        server_ = nullptr;
        ESP_LOGI(kTag, "HTTP server stopped");
    }
}

void WebServer::SetOtaCompleteCallback(OtaCompleteCallback callback) {
    ota_complete_callback_ = std::move(callback);
}

esp_err_t WebServer::HandleRoot(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, kOtaHtml, std::strlen(kOtaHtml));
    return ESP_OK;
}

esp_err_t WebServer::HandleUpload(httpd_req_t* req) {
    ESP_LOGI(kTag, "OTA upload started, size: %d bytes", req->content_len);

    // Validate content length.
    if (req->content_len == 0 || req->content_len > config::kMaxFirmwareSize) {
        ESP_LOGE(kTag, "Invalid firmware size: %d", req->content_len);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid file size");
        return ESP_FAIL;
    }

    // Get update partition.
    const esp_partition_t* partition = esp_ota_get_next_update_partition(nullptr);
    if (!partition) {
        ESP_LOGE(kTag, "No OTA partition found");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                           "No OTA partition");
        return ESP_FAIL;
    }

    ESP_LOGI(kTag, "Writing to partition: %s", partition->label);

    // Begin OTA.
    esp_ota_handle_t ota_handle;
    esp_err_t err = esp_ota_begin(partition, req->content_len, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "esp_ota_begin failed: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                           "OTA begin failed");
        return ESP_FAIL;
    }

    // Receive and write chunks.
    char buf[config::kUploadBufferSize];
    int remaining = req->content_len;
    int total_received = 0;

    while (remaining > 0) {
        int to_read = std::min(remaining, static_cast<int>(sizeof(buf)));
        int received = httpd_req_recv(req, buf, to_read);

        if (received <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;  // Retry on timeout.
            }
            ESP_LOGE(kTag, "Receive error");
            esp_ota_abort(ota_handle);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                               "Receive error");
            return ESP_FAIL;
        }

        err = esp_ota_write(ota_handle, buf, received);
        if (err != ESP_OK) {
            ESP_LOGE(kTag, "esp_ota_write failed: %s", esp_err_to_name(err));
            esp_ota_abort(ota_handle);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                               "Flash write error");
            return ESP_FAIL;
        }

        remaining -= received;
        total_received += received;

        // Log progress every 10%.
        int pct = (total_received * 100) / req->content_len;
        static int last_pct = 0;
        if (pct / 10 > last_pct / 10) {
            ESP_LOGI(kTag, "OTA progress: %d%%", pct);
            last_pct = pct;
        }
    }

    // End OTA.
    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "esp_ota_end failed: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                           "OTA verification failed");
        return ESP_FAIL;
    }

    // Set boot partition.
    err = esp_ota_set_boot_partition(partition);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "esp_ota_set_boot_partition failed: %s",
                 esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                           "Set boot partition failed");
        return ESP_FAIL;
    }

    ESP_LOGI(kTag, "OTA complete: %d bytes written to %s",
             total_received, partition->label);

    // Send success response.
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"success\":true}");

    // Notify callback.
    if (ota_complete_callback_) {
        ota_complete_callback_(true);
    }

    return ESP_OK;
}

esp_err_t WebServer::HandleReboot(httpd_req_t* req) {
    ESP_LOGI(kTag, "Reboot requested");
    httpd_resp_sendstr(req, "Rebooting...");

    // Delay to allow response to be sent.
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();

    return ESP_OK;
}

}  // namespace idrive::ota
