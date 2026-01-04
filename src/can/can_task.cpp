// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT

#include "can/can_task.h"

#include "driver/twai.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace idrive {

namespace {
const char* kTag = "CAN_TASK";
}

// Static instance for ISR access.
CanTask* CanTask::instance_ = nullptr;

CanTask::CanTask(CanBus& can) : can_(can) {
    instance_ = this;
}

CanTask::~CanTask() {
    Stop();
    if (instance_ == this) {
        instance_ = nullptr;
    }
}

bool CanTask::Start(BaseType_t core_id, UBaseType_t priority) {
    if (task_handle_) {
        ESP_LOGW(kTag, "CAN task already running");
        return true;
    }

    running_ = true;

    // Create task pinned to specific core for predictable performance.
    BaseType_t ret = xTaskCreatePinnedToCore(
        TaskFunction,
        "CAN_RX",
        can_task_config::kStackSize,
        this,
        priority,
        &task_handle_,
        core_id
    );

    if (ret != pdPASS) {
        ESP_LOGE(kTag, "Failed to create CAN task");
        running_ = false;
        return false;
    }

    ESP_LOGI(kTag, "CAN task started on core %d, priority %lu",
             static_cast<int>(core_id), static_cast<unsigned long>(priority));
    return true;
}

void CanTask::Stop() {
    if (!task_handle_) {
        return;
    }

    ESP_LOGI(kTag, "Stopping CAN task");
    running_ = false;

    // Wake up the task so it can exit.
    xTaskNotifyGive(task_handle_);

    // Wait for task to finish.
    vTaskDelay(pdMS_TO_TICKS(200));

    // Delete task if still running.
    if (task_handle_) {
        vTaskDelete(task_handle_);
        task_handle_ = nullptr;
    }
}

void IRAM_ATTR CanTask::NotifyFromISR(BaseType_t* higher_priority_woken) {
    if (instance_ && instance_->task_handle_) {
        vTaskNotifyGiveFromISR(instance_->task_handle_, higher_priority_woken);
    }
}

void CanTask::TaskFunction(void* arg) {
    auto* self = static_cast<CanTask*>(arg);
    self->Run();
}

void CanTask::Run() {
    ESP_LOGI(kTag, "CAN task running on core %d", xPortGetCoreID());

    while (running_) {
        // Block on TWAI alerts - this is the event-driven part.
        // twai_read_alerts() blocks until an alert is raised or timeout.
        uint32_t alerts = 0;
        esp_err_t ret = twai_read_alerts(
            &alerts,
            pdMS_TO_TICKS(can_task_config::kTimeoutMs)
        );

        if (!running_) {
            break;
        }

        if (ret == ESP_OK && alerts != 0) {
            // Alert received - process immediately.
            // TWAI_ALERT_RX_DATA means we have messages to read.
            can_.ProcessAlerts();
        } else if (ret == ESP_ERR_TIMEOUT) {
            // Timeout - still call ProcessAlerts for any pending messages
            // and to maintain keepalive if needed.
            can_.ProcessAlerts();
        }
    }

    ESP_LOGI(kTag, "CAN task exiting");
    task_handle_ = nullptr;
    vTaskDelete(nullptr);
}

}  // namespace idrive
