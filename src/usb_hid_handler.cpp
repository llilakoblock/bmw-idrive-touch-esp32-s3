#include "usb_hid_handler.h"
#include "driver/usb_serial_jtag.h"  // Not strictly necessary if using the new built-in USB
#include "esp_log.h"
#include "usb/usb_host.h"  // for tud_* APIs

static const char* TAG = "USBHID";

// Forward declare any needed callbacks if you want advanced usage
// e.g. set_report, get_report, etc.
// For a simple setup, we can rely on default TinyUSB handlers.

void init_usb_hid() {
  ESP_LOGI(TAG, "Initializing TinyUSB...");
  // Minimal default config
  tinyusb_config_t tusb_cfg = {};
  tusb_cfg.device_descriptor = NULL;  // use default
  tusb_cfg.string_descriptor = NULL;  // use default
  // Or use: tinyusb_config_t tusb_cfg = TINYUSB_CONFIG_DEFAULT();

  // Install TinyUSB driver
  esp_err_t ret = tinyusb_driver_install(&tusb_cfg);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to install TinyUSB driver: %s", esp_err_to_name(ret));
  } else {
    ESP_LOGI(TAG, "TinyUSB driver installed");
  }
}

// Called periodically in your main loop or a dedicated task
void usb_hid_task(void) {
  // TinyUSB device task
  tud_task();
}

// Send a relative mouse movement
void usb_hid_move_mouse(int8_t dx, int8_t dy, uint8_t buttons) {
  // buttons is a bitmask: MOUSE_BTN_LEFT=0x01, MOUSE_BTN_RIGHT=0x02, etc.
  // For scroll: use the 4th and 5th params
  // Example: tud_hid_mouse_report(report_id, buttons, dx, dy, vertical_scroll,
  // horizontal_scroll);
  tud_hid_mouse_report(0, buttons, dx, dy, 0, 0);
}

// Type out a small text by pressing and releasing each character
// A real solution might require more robust layering, but here's a simple
// approach
void usb_hid_type_text(const char* text) {
  // The ASCII -> HID scancode would normally use a big lookup table.
  // For brevity, let's do a simple subset of letters:
  // (TinyUSB wants up to 6 keys pressed at once in the array)
  while (*text) {
    uint8_t c = *text;
    uint8_t hid_code = 0;

    if (c >= 'a' && c <= 'z') {
      hid_code = (c - 'a') + 0x04;  // 'a' is 0x04 in HID
    } else if (c >= 'A' && c <= 'Z') {
      // For uppercase, set SHIFT as well. For brevity not shown here.
      hid_code = (c - 'A') + 0x04;
    } else if (c == ' ') {
      hid_code = 0x2C;  // HID code for space
    } else if (c == '\n') {
      hid_code = 0x28;  // Enter
    }
    // Add more cases if needed

    if (hid_code) {
      // Press the key
      uint8_t keycode[6] = {0};
      keycode[0] = hid_code;
      tud_hid_keyboard_report(0, 0 /* no modifier */, keycode);
      vTaskDelay(pdMS_TO_TICKS(10));

      // Release the key
      keycode[0] = 0;
      tud_hid_keyboard_report(0, 0, keycode);
      vTaskDelay(pdMS_TO_TICKS(10));
    }
    text++;
  }
}
