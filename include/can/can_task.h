// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// Event-driven CAN task with proper core affinity.
// Uses FreeRTOS task notifications for low-latency message handling.

#pragma once

#include <cstdint>

#include "can/can_bus.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace idrive {

// =============================================================================
// CAN Task Configuration
// =============================================================================

namespace can_task_config {

constexpr BaseType_t kCoreId = 1;           // Run on APP_CPU (Core 1)
constexpr UBaseType_t kPriority = 10;       // High priority for real-time
constexpr uint32_t kStackSize = 4096;       // Stack size in bytes
constexpr uint32_t kTimeoutMs = 100;        // Timeout for periodic tasks

}  // namespace can_task_config

// =============================================================================
// CAN Task Class
// =============================================================================

class CanTask {
public:
    explicit CanTask(CanBus& can);
    ~CanTask();

    // Start the CAN task on specified core.
    bool Start(BaseType_t core_id = can_task_config::kCoreId,
               UBaseType_t priority = can_task_config::kPriority);

    // Stop the CAN task.
    void Stop();

    // Check if task is running.
    bool IsRunning() const { return running_; }

    // Get task handle (for notifications).
    TaskHandle_t GetHandle() const { return task_handle_; }

    // Notify task from ISR (static for ISR callback).
    static void IRAM_ATTR NotifyFromISR(BaseType_t* higher_priority_woken);

    // Get singleton instance for ISR access.
    static CanTask* GetInstance() { return instance_; }

private:
    static void TaskFunction(void* arg);
    void Run();

    CanBus& can_;
    TaskHandle_t task_handle_ = nullptr;
    volatile bool running_ = false;

    static CanTask* instance_;
};

}  // namespace idrive
