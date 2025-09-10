#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

// Board specific
#define CFG_TUSB_MCU OPT_MCU_ESP32S3

// RHPort number used for device can be defined by board.mk, default to port 0
#define BOARD_TUD_RHPORT 0

// RHPort max operational speed can defined by board.mk
#define BOARD_TUD_MAX_SPEED OPT_MODE_DEFAULT_SPEED

// Common configuration
#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE | OPT_MODE_DEFAULT_SPEED)
#define CFG_TUSB_OS           OPT_OS_FREERTOS

// Debug
#ifndef CFG_TUSB_DEBUG
    #define CFG_TUSB_DEBUG 0
#endif

// Memory section
#ifndef CFG_TUSB_MEM_SECTION
    #define CFG_TUSB_MEM_SECTION
#endif

#ifndef CFG_TUSB_MEM_ALIGN
    #define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))
#endif

// Device configuration
#define CFG_TUD_ENDPOINT0_SIZE 64

// Class drivers
#define CFG_TUD_HID    1
#define CFG_TUD_CDC    0
#define CFG_TUD_MSC    0
#define CFG_TUD_MIDI   0
#define CFG_TUD_VENDOR 0

// HID buffer size
#define CFG_TUD_HID_EP_BUFSIZE 64

#ifdef __cplusplus
}
#endif

#endif