#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

// For an ESP32-S3 with built-in USB
#define CFG_TUSB_MCU OPT_MCU_ESP32S3

// Enable device stack
#define CFG_TUD_ENABLED 1

// Enable at most one or more HID function
#define CFG_TUD_HID 1
#define CFG_TUD_CDC 1  // optional if you want a CDC serial as well
#define CFG_TUD_MSC 0
#define CFG_TUD_MIDI 0
#define CFG_TUD_VENDOR 0

// Size of the HID buffer
#define CFG_TUD_HID_EP_BUFSIZE 64

#endif
