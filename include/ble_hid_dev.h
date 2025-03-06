#ifndef BLE_HID_NIMBLE_H
#define BLE_HID_NIMBLE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize our custom BLE HID GATT server (Keyboard + Mouse)
 * using the NimBLE stack. Sets up all NimBLE structures,
 * starts advertising, etc.
 */
void ble_hid_nimble_init(void);

/**
 * Send a keyboard report specifying which keys are pressed.
 * Typically 8 bytes: [Modifier][Reserved][6 * KeyCodes].
 *
 * @param modifier A bitmask of modifier keys (e.g. 0x02 for Left Shift)
 * @param keycodes Array of up to 6 concurrent key presses
 */
void ble_hid_send_keyboard_input(uint8_t modifier, const uint8_t keycodes[6]);

/**
 * Send a mouse report (buttons, dx, dy).
 * For a typical report: [buttons][dx][dy].
 *
 * @param buttons Bitmask (1=Left, 2=Right, 4=Middle, etc.)
 * @param dx     Signed 8-bit horizontal movement
 * @param dy     Signed 8-bit vertical movement
 */
void ble_hid_send_mouse_input(uint8_t buttons, int8_t dx, int8_t dy);

#ifdef __cplusplus
}
#endif

#endif /* BLE_HID_NIMBLE_H */
