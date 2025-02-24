#include "idrive.h"

#include <stdint.h>
#include <string.h>  // for memset if needed

#include "settings.h"
#include "variables.h"
#include "driver/twai.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "IDRIVE";

// Helper function to send a CAN frame
static void sendCANFrame(uint32_t canId, bool extended, uint8_t length,
                         uint8_t *data) {
  twai_message_t message;
  message.identifier = canId;
  message.extd = extended ? 1 : 0;
  message.data_length_code = length;
  for (int i = 0; i < length; i++) {
    message.data[i] = data[i];
  }

  esp_err_t ret = twai_transmit(&message, pdMS_TO_TICKS(50));
  if (ret != ESP_OK) {
    ESP_LOGW(TAG, "CAN transmit failed: %s", esp_err_to_name(ret));
  }
}

/*=========================================================================*/
/* iDrive Functions                                                        */
/*=========================================================================*/

void iDriveInit() {
  // Example: ID 273, Data: 1D E1 00 F0 FF 7F DE 04
  uint8_t buf[8] = {0x1D, 0xE1, 0x00, 0xF0, 0xFF, 0x7F, 0xDE, 0x04};
  sendCANFrame(MSG_OUT_ROTARY_INIT, false, 8, buf);

  RotaryInitPositionSet = false;
  ESP_LOGI(TAG, "Sent iDriveInit frame");
}

void iDriveTouchpadInit() {
  // Example: ID BF, Data: 21 00 00 00 11 00 00 00
  uint8_t buf[8] = {0x21, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00};
  sendCANFrame(MSG_IN_TOUCH, false, 8, buf);
  ESP_LOGI(TAG, "Sent iDriveTouchpadInit frame");
}

void do_iDriveLight() {
  // ID 202 => 2 FD 0 => on, 2 FE 0 => off
  uint8_t buf[2];
  buf[0] = (iDriveLightOn) ? 0xFD : 0xFE;
  buf[1] = 0x00;
  sendCANFrame(MSG_OUT_LIGHT, false, 2, buf);
}

void iDriveLight(unsigned long milliseconds) {
  static uint32_t lastLight = 0;
  uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);

  if (now - lastLight >= milliseconds) {
    lastLight = now;
    do_iDriveLight();
  }
}

void do_iDrivePoll() {
  // ID 501 => 1 0 0 0 0 0 0 0
  uint8_t buf[8] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  sendCANFrame(MSG_OUT_POLL, false, 8, buf);
}

void iDrivePoll(unsigned long milliseconds) {
  static uint32_t lastPing = 0;
  uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);

  if (now - lastPing >= milliseconds) {
    lastPing = now;
#if defined(SERIAL_DEBUG) && defined(DEBUG_CanResponse)
    ESP_LOGI(TAG, "iDrive Polling");
#endif
    do_iDrivePoll();
  }
}

/*=========================================================================*/
/*  decodeCanBus() - print frames, do minimal logic                        */
/*=========================================================================*/

bool isvalueinarray(int val, int *arr, int size) {
  for (int i = 0; i < size; i++) {
    if (arr[i] == val) return true;
  }
  return false;
}

void decodeCanBus(unsigned long canId, uint8_t len, uint8_t *buf) {
  // Print the frame data
  ESP_LOGI(TAG, "CAN ID: 0x%03lX, DLC:%d, Data:", canId, len);
  for (int i = 0; i < len; i++) {
    printf("%02X ", buf[i]);
  }
  printf("\n");

  if (canId == MSG_IN_ROTARY_INIT) {
    ESP_LOGI(TAG, "Rotary Init Success");
    RotaryInitSuccess = true;
  }

  // Example of checking iDrive status
  if (canId == MSG_IN_STATUS) {
    // if buf[4] == 0x06 => lost init
    if (buf[4] == MSG_STATUS_NO_INIT) {
      ESP_LOGW(TAG, "iDrive lost init");
      RotaryInitSuccess = false;
      LightInitDone = false;
      previousMillis = 0;
      CoolDownMillis = 0;
      TouchpadInitIgnoreCounter = 0;
    }
  }
}
