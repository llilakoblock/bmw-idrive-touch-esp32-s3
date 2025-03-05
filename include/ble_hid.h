#pragma once

#include <stdbool.h>
#include <stdint.h>

// Initialize BLE, set up a combined Keyboard+Mouse HID device
void bleHIDInit(void);

// Send a single key press/release (e.g. for iDrive buttons)
void bleKeyboardPress(uint8_t keycode, bool pressed);

// Type a string (like sending 'home' text)
void bleKeyboardTypeText(const char *text);

// Send mouse movement (delta X, delta Y, plus optional button states)
void bleMouseMove(int8_t deltaX, int8_t deltaY, uint8_t buttons = 0);
