#include "ble_hid.h"

#include <string.h>

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_hidd.h"
#include "esp_log.h"
#include <esp_hidd_api.h>

static const char *TAG = "BLE_HID";

// Forward declarations
static void ble_gap_event_handler(esp_gap_ble_cb_event_t event,
                                  esp_ble_gap_cb_param_t *param);
static void ble_hidd_event_callback(esp_hidd_cb_event_t event,
                                    esp_hidd_cb_param_t *param);

// This handle is used to send keyboard/mouse reports
static esp_hidd_dev_t *s_hid_device = nullptr;

// Descriptor for standard 8-byte keyboard report
static void sendKeyboardReport(uint8_t modifier, uint8_t key1, uint8_t key2,
                               uint8_t key3, uint8_t key4, uint8_t key5,
                               uint8_t key6) {
  if (!s_hid_device) return;

  uint8_t report[8];
  memset(report, 0, sizeof(report));
  report[0] = modifier;  // e.g. shift, ctrl, etc.
  // report[1] is reserved
  report[2] = key1;
  report[3] = key2;
  report[4] = key3;
  report[5] = key4;
  report[6] = key5;
  report[7] = key6;

  esp_hidd_dev_input_set(s_hid_device, 0, report, sizeof(report));
}

// Simple mouse report: 3-button mask + X delta + Y delta + vertical wheel, etc.
// For a basic relative mouse, typically 5 bytes: [buttons, xDelta, yDelta,
// wheel, hWheel]
static void sendMouseReport(uint8_t buttons, int8_t dx, int8_t dy) {
  if (!s_hid_device) return;

  uint8_t report[5];
  memset(report, 0, sizeof(report));
  report[0] = buttons;  // e.g. bit0=left btn, bit1=right btn, bit2=middle
  report[1] = dx;       // delta X
  report[2] = dy;       // delta Y
  report[3] = 0;        // wheel
  report[4] = 0;        // hWheel

  esp_hidd_dev_input_set(s_hid_device, 1, report, sizeof(report));
}

// ======== Public functions to call from your iDrive code ========

void bleKeyboardPress(uint8_t keycode, bool pressed) {
  // This sends a single-key report. For simplicity, we only send key in
  // position [2] ignoring modifiers or multiple keys pressed at once. Note: If
  // 'pressed' is false, we send 0 to release.

  if (pressed) {
    sendKeyboardReport(0, keycode, 0, 0, 0, 0, 0);
  } else {
    // release
    sendKeyboardReport(0, 0, 0, 0, 0, 0, 0);
  }
}

void bleKeyboardTypeText(const char *text) {
  // This method quickly 'types' each character. In a real project, add small
  // delays between press and release, or keep track of multiple keys, etc.

  while (*text) {
    uint8_t c = *text++;
    // For ASCII letters, HID keycode offset is typically (c - 'a' + 4) for
    // lowercase, etc. You would need a robust map from ASCII to HID codes. For
    // this example, do a naive approach:
    if (c >= 'a' && c <= 'z') {
      uint8_t keycode = (c - 'a') + 0x04;  // 4 is HID_KEY_A
      bleKeyboardPress(keycode, true);
      bleKeyboardPress(keycode, false);
    } else if (c >= 'A' && c <= 'Z') {
      // do SHIFT + letter
      uint8_t keycode = (c - 'A') + 0x04;
      sendKeyboardReport(0x02, keycode, 0, 0, 0, 0, 0);  // Shift pressed
      sendKeyboardReport(0, 0, 0, 0, 0, 0, 0);           // release
    } else {
      // For “home” or punctuation, etc. you’d need a more thorough mapping
      // We'll just ignore in this minimal example.
    }
  }
}

void bleMouseMove(int8_t deltaX, int8_t deltaY, uint8_t buttons) {
  sendMouseReport(buttons, deltaX, deltaY);
}

// ======== HID setup code ========

// BLE HID callback
static void ble_hidd_event_callback(esp_hidd_cb_event_t event,
                                    esp_hidd_cb_param_t *param) {
  switch (event) {
    case ESP_HIDD_EVENT_REG_FINISH:
      if (param->init_finish.state == ESP_HIDD_STA_CON_SUCCESS) {
        ESP_LOGI(TAG, "ESP_HIDD_EVENT_REG_FINISH");
        s_hid_device = param->init_finish.hid_dev;
        // Set device into advertising mode
        esp_ble_gap_start_advertising(&param->init_finish.adv_params);
      }
      break;
    case ESP_HIDD_EVENT_DEINIT_FINISH:
      s_hid_device = nullptr;
      break;
    case ESP_HIDD_EVENT_BLE_CONNECT:
      ESP_LOGI(TAG, "ESP_HIDD_EVENT_BLE_CONNECT");
      break;
    case ESP_HIDD_EVENT_BLE_DISCONNECT:
      ESP_LOGI(TAG, "ESP_HIDD_EVENT_BLE_DISCONNECT");
      esp_ble_gap_start_advertising(&param->disconnect.adv_params);
      break;
    default:
      break;
  }
}

static void ble_gap_event_handler(esp_gap_ble_cb_event_t event,
                                  esp_ble_gap_cb_param_t *param) {
  switch (event) {
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
      if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(TAG, "Advertising start failed");
      }
      break;
    default:
      break;
  }
}

void bleHIDInit(void) {
  ESP_LOGI(TAG, "Initializing BLE HID...");

  // 1) Initialize the BT Controller
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  esp_bt_controller_init(&bt_cfg);
  esp_bt_controller_enable(ESP_BT_MODE_BLE);

  // 2) Initialize Bluedroid
  esp_bluedroid_init();
  esp_bluedroid_enable();

  // 3) Register the gap callback
  esp_ble_gap_register_callback(ble_gap_event_handler);

  // 4) Configure the HID device
  esp_hidd_app_param_t app_param = {
      .name = "ESP iDrive HID",
      .description = "iDrive BLE HID",
      .provider = "Espressif",
      .subclass = 0x8,  // Combo mouse+keyboard
      .desc_list = NULL,
      .desc_list_len = 0,
  };

  esp_hidd_qos_param_t both_qos = {.role = ESP_HIDD_QOS_ROLE_DEVICE,
                                   .sync_mode = 0,
                                   .event_mode = 0,
                                   .polling_interval = 0,
                                   .max_latency = 0,
                                   .report_attempts = 0,
                                   .report_timeout = 0};

  // 5) Register the HID device with the stack
  esp_hidd_dev_init(&app_param, &both_qos, &both_qos, ble_hidd_event_callback);

  ESP_LOGI(TAG, "BLE HID initialization done!");
}
