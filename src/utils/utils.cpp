// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT

#include "utils/utils.h"

#include "esp_timer.h"

namespace idrive::utils {

uint32_t GetMillis()
{
    return static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
}

int MapValue(int x, int in_min, int in_max, int out_min, int out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

}  // namespace idrive::utils
