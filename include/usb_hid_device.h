// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// USB HID device interface for keyboard, mouse, and media controls.

#ifndef BMW_IDRIVE_ESP32_USB_HID_DEVICE_H_
#define BMW_IDRIVE_ESP32_USB_HID_DEVICE_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Initialization
// =============================================================================

// Initializes the USB HID device with keyboard, mouse, and consumer controls.
void UsbHidDeviceInit(void);

// =============================================================================
// Keyboard Functions
// =============================================================================

// Sends a key press report for the specified keycode.
void UsbHidKeyboardPress(uint8_t keycode);

// Sends a key release report for the specified keycode.
void UsbHidKeyboardRelease(uint8_t keycode);

// Sends a key press followed by release after a short delay.
void UsbHidKeyboardPressAndRelease(uint8_t keycode);

// =============================================================================
// Media Control Functions (Consumer Page)
// =============================================================================

// Sends a media key press report.
void UsbHidMediaKeyPress(uint16_t keycode);

// Sends a media key release report.
void UsbHidMediaKeyRelease(uint16_t keycode);

// Sends a media key press followed by release after a short delay.
void UsbHidMediaKeyPressAndRelease(uint16_t keycode);

// =============================================================================
// Mouse Functions
// =============================================================================

// Sends a relative mouse movement report.
// Args:
//   x: Horizontal movement (-127 to 127).
//   y: Vertical movement (-127 to 127).
void UsbHidMouseMove(int8_t x, int8_t y);

// Sends a mouse button press report.
void UsbHidMouseButtonPress(uint8_t button);

// Sends a mouse button release report.
void UsbHidMouseButtonRelease(uint8_t button);

// Sends a mouse click (press + release) with delay.
void UsbHidMouseClick(uint8_t button);

// Sends a mouse scroll wheel report.
// Args:
//   wheel: Scroll amount (-127 to 127, positive = scroll up).
void UsbHidMouseScroll(int8_t wheel);

// =============================================================================
// Status Functions
// =============================================================================

// Returns true if USB device is connected and ready.
bool UsbHidIsConnected(void);

// =============================================================================
// Standard Keyboard Key Codes (USB HID Usage Table)
// =============================================================================

#define HID_KEY_A         0x04
#define HID_KEY_B         0x05
#define HID_KEY_C         0x06
#define HID_KEY_D         0x07
#define HID_KEY_E         0x08
#define HID_KEY_F         0x09
#define HID_KEY_G         0x0A
#define HID_KEY_H         0x0B
#define HID_KEY_I         0x0C
#define HID_KEY_J         0x0D
#define HID_KEY_K         0x0E
#define HID_KEY_L         0x0F
#define HID_KEY_M         0x10
#define HID_KEY_N         0x11
#define HID_KEY_O         0x12
#define HID_KEY_P         0x13
#define HID_KEY_Q         0x14
#define HID_KEY_R         0x15
#define HID_KEY_S         0x16
#define HID_KEY_T         0x17
#define HID_KEY_U         0x18
#define HID_KEY_V         0x19
#define HID_KEY_W         0x1A
#define HID_KEY_X         0x1B
#define HID_KEY_Y         0x1C
#define HID_KEY_Z         0x1D

#define HID_KEY_1         0x1E
#define HID_KEY_2         0x1F
#define HID_KEY_3         0x20
#define HID_KEY_4         0x21
#define HID_KEY_5         0x22
#define HID_KEY_6         0x23
#define HID_KEY_7         0x24
#define HID_KEY_8         0x25
#define HID_KEY_9         0x26
#define HID_KEY_0         0x27

#define HID_KEY_ENTER     0x28
#define HID_KEY_ESC       0x29
#define HID_KEY_BACKSPACE 0x2A
#define HID_KEY_TAB       0x2B
#define HID_KEY_SPACE     0x2C

#define HID_KEY_F1        0x3A
#define HID_KEY_F2        0x3B
#define HID_KEY_F3        0x3C
#define HID_KEY_F4        0x3D
#define HID_KEY_F5        0x3E
#define HID_KEY_F6        0x3F
#define HID_KEY_F7        0x40
#define HID_KEY_F8        0x41
#define HID_KEY_F9        0x42
#define HID_KEY_F10       0x43
#define HID_KEY_F11       0x44
#define HID_KEY_F12       0x45

#define HID_KEY_RIGHT     0x4F
#define HID_KEY_LEFT      0x50
#define HID_KEY_DOWN      0x51
#define HID_KEY_UP        0x52

// =============================================================================
// Media Control Key Codes (USB HID Consumer Page)
// =============================================================================

#define HID_MEDIA_PLAY_PAUSE   0x00CD
#define HID_MEDIA_STOP         0x00B7
#define HID_MEDIA_NEXT_TRACK   0x00B5
#define HID_MEDIA_PREV_TRACK   0x00B6
#define HID_MEDIA_VOLUME_UP    0x00E9
#define HID_MEDIA_VOLUME_DOWN  0x00EA
#define HID_MEDIA_MUTE         0x00E2
#define HID_MEDIA_BASS_BOOST   0x00E5
#define HID_MEDIA_LOUDNESS     0x00E7
#define HID_MEDIA_BASS_UP      0x0152
#define HID_MEDIA_BASS_DOWN    0x0153
#define HID_MEDIA_TREBLE_UP    0x0154
#define HID_MEDIA_TREBLE_DOWN  0x0155

// =============================================================================
// Android-Specific Media Key Codes
// =============================================================================

#define HID_ANDROID_BACK   0x0224  // AC Back
#define HID_ANDROID_HOME   0x0223  // AC Home
#define HID_ANDROID_MENU   0x0040  // Menu key
#define HID_ANDROID_SEARCH 0x0221  // AC Search

// =============================================================================
// Mouse Button Definitions
// =============================================================================

#define HID_MOUSE_BUTTON_LEFT   0x01
#define HID_MOUSE_BUTTON_RIGHT  0x02
#define HID_MOUSE_BUTTON_MIDDLE 0x04

// =============================================================================
// Legacy Function Names (for compatibility)
// =============================================================================

#define usb_hid_device_init           UsbHidDeviceInit
#define usb_hid_keyboard_press        UsbHidKeyboardPress
#define usb_hid_keyboard_release      UsbHidKeyboardRelease
#define usb_hid_keyboard_press_and_release UsbHidKeyboardPressAndRelease
#define usb_hid_media_key_press       UsbHidMediaKeyPress
#define usb_hid_media_key_release     UsbHidMediaKeyRelease
#define usb_hid_media_key_press_and_release UsbHidMediaKeyPressAndRelease
#define usb_hid_mouse_move            UsbHidMouseMove
#define usb_hid_mouse_button_press    UsbHidMouseButtonPress
#define usb_hid_mouse_button_release  UsbHidMouseButtonRelease
#define usb_hid_mouse_click           UsbHidMouseClick
#define usb_hid_mouse_scroll          UsbHidMouseScroll
#define usb_hid_is_connected          UsbHidIsConnected

#ifdef __cplusplus
}
#endif

#endif  // BMW_IDRIVE_ESP32_USB_HID_DEVICE_H_
