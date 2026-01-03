// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// Utility functions.

#pragma once

#include <cstdint>
#include <cstddef>

namespace idrive::utils {

// Returns current time in milliseconds since boot.
uint32_t GetMillis();

// Constrains a value to a specified range [min_val, max_val].
template <typename T>
constexpr T Constrain(T value, T min_val, T max_val) {
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

// Maps a value from one range to another (Arduino-style linear interpolation).
int MapValue(int x, int in_min, int in_max, int out_min, int out_max);

// Checks if a value exists in an array.
template <typename T, size_t N>
constexpr bool IsInArray(T value, const T (&array)[N]) {
    for (size_t i = 0; i < N; ++i) {
        if (array[i] == value) return true;
    }
    return false;
}

}  // namespace idrive::utils
