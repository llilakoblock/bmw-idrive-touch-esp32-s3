# ESP32-S3 Performance Optimization Guide

## Overview

This document describes the task architecture, CPU core distribution, and performance optimization strategies for the BMW iDrive ESP32-S3 project.

## Hardware Specifications

### ESP32-S3 Dual-Core Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         ESP32-S3 SoC                            │
├─────────────────────────────────────────────────────────────────┤
│  Core 0 (PRO_CPU)          │  Core 1 (APP_CPU)                  │
│  - System tasks            │  - Application tasks               │
│  - WiFi/BT stack           │  - User code                       │
│  - 240 MHz Xtensa LX7      │  - 240 MHz Xtensa LX7              │
└─────────────────────────────────────────────────────────────────┘
```

### Memory Layout

| Region | Size | Usage |
|--------|------|-------|
| IRAM | 32KB | Fast code (ISRs, hot paths) |
| DRAM | 512KB | Heap, stack, BSS, data |
| Flash | 8MB | Application code, OTA partitions |

## Task Distribution Strategy

### Recommended Core Assignment

```
┌─────────────────────────────────────────────────────────────────┐
│                         CORE 0                                  │
│  Purpose: System & Communication Tasks                          │
├─────────────────────────────────────────────────────────────────┤
│  • WiFi Task (OTA mode only)        Priority: 23                │
│  • HTTP Server (OTA mode only)      Priority: 5                 │
│  • TinyUSB Task                     Priority: 5                 │
│  • ESP Timer                        Priority: 22                │
│  • IDLE0                            Priority: 0                 │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                         CORE 1                                  │
│  Purpose: Real-time Application Tasks                           │
├─────────────────────────────────────────────────────────────────┤
│  • CAN Task (event-driven)          Priority: 10                │
│  • Main Controller Task             Priority: 5                 │
│  • IDLE1                            Priority: 0                 │
└─────────────────────────────────────────────────────────────────┘
```

### Task Creation Guidelines

**Always use `xTaskCreatePinnedToCore()` instead of `xTaskCreate()`:**

```cpp
// BAD - task can migrate between cores
xTaskCreate(TaskFunc, "Task", 4096, nullptr, 5, &handle);

// GOOD - task pinned to specific core
xTaskCreatePinnedToCore(
    TaskFunc,       // Function
    "Task",         // Name
    4096,           // Stack size
    nullptr,        // Parameter
    5,              // Priority
    &handle,        // Handle
    1               // Core ID (0 or 1)
);
```

### Priority Guidelines

| Priority Range | Usage |
|---------------|-------|
| 20-25 | System critical (timers, WiFi) |
| 10-15 | High priority application (CAN RX) |
| 5-9 | Normal application (USB, main loop) |
| 1-4 | Low priority background |
| 0 | IDLE only |

## Event-Driven vs Polling Architecture

### Polling Architecture (Current)

```
Main Loop (every 10ms):
┌─────────────────────────────────────┐
│  while (true) {                     │
│      can.ProcessAlerts();  // poll  │
│      controller.Update();           │
│      vTaskDelay(10ms);     // wait  │
│  }                                  │
└─────────────────────────────────────┘

Latency: 0-10ms (average 5ms)
CPU Usage: Constant polling overhead
```

### Event-Driven Architecture (Recommended)

```
ISR → Notification → Task:
┌─────────────────────────────────────┐
│  ISR: xTaskNotifyFromISR()          │
│           ↓                         │
│  Task: ulTaskNotifyTake() // blocks │
│           ↓                         │
│  Process immediately                │
└─────────────────────────────────────┘

Latency: ~50-100µs
CPU Usage: Sleep when idle
```

### Implementation Pattern

```cpp
// Task that waits for events
void EventDrivenTask(void* arg) {
    while (running) {
        // Block until notification (or timeout for keepalive)
        uint32_t notification = ulTaskNotifyTake(
            pdTRUE,                  // Clear on exit
            pdMS_TO_TICKS(100)       // Timeout for periodic tasks
        );

        if (notification > 0) {
            ProcessEvents();  // Handle immediately
        } else {
            PeriodicMaintenance();  // Keepalive, etc.
        }
    }
}

// ISR callback
void IRAM_ATTR AlertISR(void* arg) {
    BaseType_t yield = pdFALSE;
    vTaskNotifyGiveFromISR(task_handle, &yield);
    portYIELD_FROM_ISR(yield);
}
```

## CAN Bus Optimization

### TWAI Configuration

```cpp
twai_general_config_t config = {
    .mode = TWAI_MODE_NORMAL,
    .tx_io = GPIO_NUM_5,
    .rx_io = GPIO_NUM_4,
    .clkout_io = GPIO_NUM_NC,
    .bus_off_io = GPIO_NUM_NC,
    .tx_queue_len = 10,          // TX buffer depth
    .rx_queue_len = 20,          // RX buffer (increase for high traffic)
    .alerts_enabled = TWAI_ALERT_RX_DATA |
                      TWAI_ALERT_ERR_PASS |
                      TWAI_ALERT_BUS_OFF,
    .clkout_divider = 0,
    .intr_flags = ESP_INTR_FLAG_LEVEL1  // Low latency interrupt
};
```

### Alert-Based Reception

```cpp
// Configure alerts for event-driven RX
twai_reconfigure_alerts(
    TWAI_ALERT_RX_DATA,      // Notify on new message
    nullptr
);

// In ISR or alert handler
uint32_t alerts;
if (twai_read_alerts(&alerts, 0) == ESP_OK) {
    if (alerts & TWAI_ALERT_RX_DATA) {
        // Notify CAN task
        xTaskNotifyGiveFromISR(can_task_handle, &yield);
    }
}
```

## USB HID Optimization

### Task Placement

USB task should run on Core 0 (same as WiFi) to avoid cache contention with CAN processing:

```cpp
xTaskCreatePinnedToCore(
    UsbDeviceTask,
    "TinyUSB",
    4096,
    nullptr,
    5,
    &usb_task_handle,
    0  // Core 0
);
```

### Report Rate

Default HID polling interval is 10ms. For smoother input:
- Mouse: 8ms (125Hz) is sufficient
- Keyboard: 10ms is fine
- Adjust `bInterval` in HID descriptor if needed

## Latency Analysis

### Complete Data Flow Timing

```
Component                    Typical Latency
─────────────────────────────────────────────
CAN Frame Reception          250µs (8 bytes @ 500kbps)
TWAI ISR Processing          2-5µs
FreeRTOS Queue/Notify        1-2µs
Task Wake-up                 10-50µs
Application Processing       10-50µs
USB HID Report Queue         1-2µs
USB Poll Response            0-10ms (host dependent)
─────────────────────────────────────────────
Total (event-driven):        ~300µs + USB poll
Total (polling @ 10ms):      ~10ms + USB poll
```

## Memory Optimization

### Stack Size Guidelines

| Task Type | Recommended Stack |
|-----------|------------------|
| Simple ISR handler | 2048 bytes |
| CAN processing | 4096 bytes |
| USB task | 4096 bytes |
| WiFi/HTTP (OTA) | 8192 bytes |

### Heap Usage Monitoring

```cpp
// Check available heap
size_t free_heap = esp_get_free_heap_size();
size_t min_free = esp_get_minimum_free_heap_size();

ESP_LOGI(TAG, "Heap: %d free, %d minimum", free_heap, min_free);
```

## Power Optimization

### Light Sleep (when idle)

For battery-powered applications:

```cpp
// Enable automatic light sleep
esp_pm_config_esp32s3_t pm_config = {
    .max_freq_mhz = 240,
    .min_freq_mhz = 80,
    .light_sleep_enable = true
};
esp_pm_configure(&pm_config);
```

### Task Idle Behavior

Event-driven tasks naturally support power saving:
- `ulTaskNotifyTake()` with timeout allows CPU to sleep
- Polling with `vTaskDelay()` keeps CPU semi-active

## Debugging Performance

### Task Runtime Statistics

Enable in sdkconfig:
```
CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y
CONFIG_FREERTOS_USE_TRACE_FACILITY=y
```

```cpp
void PrintTaskStats() {
    char buffer[1024];
    vTaskGetRunTimeStats(buffer);
    printf("Task Stats:\n%s\n", buffer);
}
```

### CPU Load Monitoring

```cpp
// Get idle task runtime percentage
UBaseType_t idle_percentage = 100 - (non_idle_ticks * 100 / total_ticks);
```

## Checklist for New Tasks

1. [ ] Pin to specific core with `xTaskCreatePinnedToCore()`
2. [ ] Choose appropriate priority (see guidelines)
3. [ ] Use event-driven pattern where possible
4. [ ] Minimize stack size (start small, increase if needed)
5. [ ] Use `IRAM_ATTR` for ISR handlers
6. [ ] Protect shared resources with mutex/semaphore
7. [ ] Add watchdog feeding if long-running
