#include "ble_hid_dev.h"

#include <stdio.h>
#include <string.h>

#include "esp_check.h"
#include "esp_log.h"

// NimBLE headers
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/dis/ble_svc_dis.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "esp_hidd.h"

static const char *TAG = "BLE_HID_NIMBLE";

// -----------------------------
// A minimal HID Report Descriptor:
//  - Keyboard (Report ID=1): 8 bytes
//  - Mouse (Report ID=2): 3 bytes
// This is just an example, you can expand it to handle more keys/feature
// reports.
static const uint8_t s_hidReportMap[] = {
    0x05, 0x01,  // Usage Page (Generic Desktop)
    0x09, 0x06,  // Usage (Keyboard)
    0xA1, 0x01,  // Collection (Application)
    0x85, 0x01,  //   REPORT_ID (1)
    // Modifier byte
    0x05, 0x07,  //   Usage Page (Key Codes)
    0x19, 0xE0,  //   Usage Min (224)
    0x29, 0xE7,  //   Usage Max (231)
    0x15, 0x00,  //   Logical Min (0)
    0x25, 0x01,  //   Logical Max (1)
    0x75, 0x01,  //   Report Size (1)
    0x95, 0x08,  //   Report Count (8)
    0x81, 0x02,  //   Input (Data,Var,Abs)
    // Reserved byte
    0x95, 0x01,  //   Report Count (1)
    0x75, 0x08,  //   Report Size (8)
    0x81, 0x01,  //   Input (Const,Array,Abs)
    // Keycode array (6 bytes)
    0x95, 0x06,  //   Report Count (6)
    0x75, 0x08,  //   Report Size (8)
    0x15, 0x00,  //   Logical Min (0)
    0x25, 0x65,  //   Logical Max (101)
    0x05, 0x07,  //   Usage Page (Key Codes)
    0x19, 0x00,  //   Usage Min (0)
    0x29, 0x65,  //   Usage Max (101)
    0x81, 0x00,  //   Input (Data,Array,Abs)
    0xC0,        // End Collection

    // Mouse
    0x05, 0x01,  // Usage Page (Generic Desktop)
    0x09, 0x02,  // Usage (Mouse)
    0xA1, 0x01,  // Collection (Application)
    0x85, 0x02,  //   REPORT_ID (2)
    0x09, 0x01,  //   Usage (Pointer)
    0xA1, 0x00,  //   Collection (Physical)
    0x05, 0x09,  //     Usage Page (Buttons)
    0x19, 0x01,  //     Usage Min (Button 1)
    0x29, 0x03,  //     Usage Max (Button 3)
    0x15, 0x00,  //     Logical Min (0)
    0x25, 0x01,  //     Logical Max (1)
    0x95, 0x03,  //     Report Count (3)
    0x75, 0x01,  //     Report Size (1)
    0x81, 0x02,  //     Input (Data,Var,Abs)
    0x95, 0x01,  //     Report Count (1)
    0x75, 0x05,  //     Report Size (5) - padding
    0x81, 0x01,  //     Input (Cnst,Arr,Abs)
    // X, Y movement
    0x05, 0x01,  //     Usage Page (Generic Desktop)
    0x09, 0x30,  //     Usage (X)
    0x09, 0x31,  //     Usage (Y)
    0x15, 0x81,  //     Logical Min (-127)
    0x25, 0x7F,  //     Logical Max (127)
    0x75, 0x08,  //     Report Size (8)
    0x95, 0x02,  //     Report Count (2)
    0x81, 0x06,  //     Input (Data,Var,Rel)
    0xC0,        //   End Collection
    0xC0,        // End Collection
};

// -------------- BLE HID Service Declarations --------------

/// HID Service (0x1812)
static const uint16_t BLE_SVC_HID_UUID = 0x1812;
/// HID Information characteristic (0x2A4A)
static const uint16_t BLE_CHAR_HID_INFORMATION_UUID = 0x2A4A;
/// HID Report Map characteristic (0x2A4B)
static const uint16_t BLE_CHAR_REPORT_MAP_UUID = 0x2A4B;
/// HID Control Point characteristic (0x2A4C)
static const uint16_t BLE_CHAR_HID_CONTROL_POINT_UUID = 0x2A4C;
/// HID Report characteristic (0x2A4D)
static const uint16_t BLE_CHAR_REPORT_UUID = 0x2A4D;
// HID Protocol Mode characteristic (0x2A4E) - optional
static const uint16_t BLE_CHAR_PROTOCOL_MODE_UUID = 0x2A4E;

// We'll store our connection handle + attribute handles for sending
// notifications
static uint16_t s_conn_handle = BLE_HS_CONN_HANDLE_NONE;

// For simplicity, define two "report" handles for keyboard + mouse
static uint16_t s_hid_report_input_handle_1 = 0;  // Keyboard input
static uint16_t s_hid_report_input_handle_2 = 0;  // Mouse input

// HID Info
// bcdHID=0x0101, countryCode=0, flags=0 (normal)
static const uint8_t s_hidInfo[4] = {0x01, 0x01, 0x00, 0x00};

/// Callback when GATT access is performed (reading our characteristics, etc.).
static int ble_hid_gatt_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                                  struct ble_gatt_access_ctxt *ctxt,
                                  void *arg) {
  switch (ctxt->op) {
    case BLE_GATT_ACCESS_OP_READ_CHR:
      // Reading characteristic
      if (attr_handle == /* HID Info handle */ arg) {
        // Return the HID Info
        os_mbuf_append(ctxt->om, s_hidInfo, sizeof(s_hidInfo));
      } else if (attr_handle == /* HID Report Map handle */ arg) {
        // Return the HID Report Map
        os_mbuf_append(ctxt->om, s_hidReportMap, sizeof(s_hidReportMap));
      }
      // else if ... for other reads
      break;

    case BLE_GATT_ACCESS_OP_WRITE_CHR:
      // Typically for HID Control Point or output reports
      // You can parse if needed (e.g. LED states for keyboard)
      break;

    default:
      break;
  }
  return 0;  // success
}

// We define a few static references so we can pass them as 'arg' in the
// callback
static uint16_t s_hidInfoHandle = 0;
static uint16_t s_hidReportMapHandle = 0;
static uint16_t s_hidControlPointHandle = 0;

// The GATT definitions
static const struct ble_gatt_svc_def s_ble_gatt_services[] = {
    {/*** HID SERVICE ***/
     .type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = BLE_UUID16_DECLARE(BLE_SVC_HID_UUID),
     .characteristics =
         (struct ble_gatt_chr_def[]){
             {
                 // HID Information
                 .uuid = BLE_UUID16_DECLARE(BLE_CHAR_HID_INFORMATION_UUID),
                 .access_cb = ble_hid_gatt_access_cb,
                 .val_handle = &s_hidInfoHandle,
                 .flags = BLE_GATT_CHR_F_READ,
                 .arg = &s_hidInfoHandle,
             },
             {
                 // HID Report Map
                 .uuid = BLE_UUID16_DECLARE(BLE_CHAR_REPORT_MAP_UUID),
                 .access_cb = ble_hid_gatt_access_cb,
                 .val_handle = &s_hidReportMapHandle,
                 .flags = BLE_GATT_CHR_F_READ,
                 .arg = &s_hidReportMapHandle,
             },
             {
                 // HID Control Point
                 .uuid = BLE_UUID16_DECLARE(BLE_CHAR_HID_CONTROL_POINT_UUID),
                 .access_cb = ble_hid_gatt_access_cb,
                 .val_handle = &s_hidControlPointHandle,
                 .flags = BLE_GATT_CHR_F_WRITE_NO_RSP,
                 .arg = &s_hidControlPointHandle,
             },
             {
                 // Keyboard Input Report (ID=1)
                 .uuid = BLE_UUID16_DECLARE(BLE_CHAR_REPORT_UUID),
                 .access_cb = NULL,  // No read or write needed, we only send
                                     // notifications
                 .val_handle = &s_hid_report_input_handle_1,
                 .flags = BLE_GATT_CHR_F_NOTIFY,
             },
             {
                 // Mouse Input Report (ID=2)
                 .uuid = BLE_UUID16_DECLARE(BLE_CHAR_REPORT_UUID),
                 .access_cb = NULL,
                 .val_handle = &s_hid_report_input_handle_2,
                 .flags = BLE_GATT_CHR_F_NOTIFY,
             },
             {
                 0,  // no more characteristics
             }}},

    {
        0,  // no more services
    }};

// ------------------- NimBLE callbacks ---------------------

/// When a client (PC/phone) connects to us:
static int ble_hid_gap_event_cb(struct ble_gap_event *event, void *arg) {
  switch (event->type) {
    case BLE_GAP_EVENT_CONNECT: {
      if (event->connect.status == 0) {
        ESP_LOGI(TAG, "BLE HID: Client connected, handle=%d",
                 event->connect.conn_handle);
        s_conn_handle = event->connect.conn_handle;
      } else {
        ESP_LOGI(TAG, "BLE HID: Connection failed, status=%d",
                 event->connect.status);
        s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
        // Re-start advertising
        ble_gap_adv_start(/* ... */);
      }
      break;
    }
    case BLE_GAP_EVENT_DISCONNECT: {
      ESP_LOGI(TAG, "BLE HID: Client disconnected");
      s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
      // Re-start advertising
      ble_gap_adv_start(/* ... */);
      break;
    }
    default:
      break;
  }
  return 0;
}

/// Start NimBLE advertising with typical parameters for a HID device
static void ble_hid_start_advertising(void) {
  struct ble_gap_adv_params adv_params;
  memset(&adv_params, 0, sizeof(adv_params));
  adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;  // undirected connectable
  adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;  // general discovery

  // Example advertisement data
  static uint8_t adv_data[] = {
      0x02,
      BLE_HS_ADV_TYPE_FLAGS,
      BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP,
      0x03,
      BLE_HS_ADV_TYPE_TX_PWR_LEVEL,
      0x00,  // TX power
      0x07,
      BLE_HS_ADV_TYPE_COMPLETE_NAME,
      'E',
      'S',
      'P',
      '-',
      'H',
      'I',
      'D',
  };

  ble_gap_adv_set_data(adv_data, sizeof(adv_data));
  ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params,
                    ble_hid_gap_event_cb, NULL);
  ESP_LOGI(TAG, "Started NimBLE advertising as 'ESP-HID'");
}

/// NimBLE host task
static void ble_hid_host_task(void *param) {
  ESP_LOGI(TAG, "NimBLE host task started");
  nimble_port_run();
  nimble_port_freertos_deinit();
}

// ---------------- Initialization ----------------
void ble_hid_nimble_init(void) {
  ESP_ERROR_CHECK(nimble_port_init());
  ESP_ERROR_CHECK(ble_hs_util_ensure_addr(0));

  // Register GATT services
  ble_gatts_count_cfg(s_ble_gatt_services);
  ble_gatts_add_svcs(s_ble_gatt_services);

  // Device Info Service (optional)
  ble_svc_dis_init();
  // GAP service for device name
  ble_svc_gap_init();
  ble_svc_gap_device_name_set("ESP-HID");

  // Start the GATT server
  nimble_port_freertos_init(ble_hid_host_task);

  // Once the server is up, we can start advertising
  ble_hid_start_advertising();
}

// ---------------- Sending Reports ----------------

void ble_hid_send_keyboard_input(uint8_t modifier, const uint8_t keycodes[6]) {
  if (s_conn_handle == BLE_HS_CONN_HANDLE_NONE) {
    return;  // not connected
  }
  // Our keyboard report is 8 bytes: [Modifier][Reserved=0][6 x KeyCodes]
  uint8_t report[8];
  memset(report, 0, sizeof(report));
  report[0] = modifier;
  if (keycodes) {
    memcpy(&report[2], keycodes, 6);
  }
  // We must prepend the Report ID if that’s how you interpret the descriptor
  // i.e. [ReportID=1][the 8 bytes above], but we used a separate characteristic
  // for each ID in the minimal example. Actually, we declared a single 2A4D
  // repeated. Let's do a simplified approach (no ID in the data).

  // We'll just notify the "keyboard input" characteristic handle:
  struct os_mbuf *om = ble_hs_mbuf_from_flat(report, sizeof(report));
  ble_gatts_chr_updated(
      s_hid_report_input_handle_1);  // ensures CCCD is validated
  ble_gatts_notify_custom(s_conn_handle, s_hid_report_input_handle_1, om);
}

void ble_hid_send_mouse_input(uint8_t buttons, int8_t dx, int8_t dy) {
  if (s_conn_handle == BLE_HS_CONN_HANDLE_NONE) {
    return;
  }
  // For the mouse, 3 bytes: [buttons][dx][dy]
  uint8_t report[3];
  report[0] = buttons;
  report[1] = (uint8_t)dx;
  report[2] = (uint8_t)dy;

  struct os_mbuf *om = ble_hs_mbuf_from_flat(report, sizeof(report));
  ble_gatts_chr_updated(s_hid_report_input_handle_2);
  ble_gatts_notify_custom(s_conn_handle, s_hid_report_input_handle_2, om);
}
