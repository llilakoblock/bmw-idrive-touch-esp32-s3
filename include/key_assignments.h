#ifndef KEYASSIGNMENTS_H
#define KEYASSIGNMENTS_H

#define HID_KEY_A 0x04
#define HID_KEY_B 0x05
#define HID_KEY_C 0x06
#define HID_KEY_D 0x07
#define HID_KEY_E 0x08
#define HID_KEY_F 0x09
#define HID_KEY_M 0x10
#define HID_KEY_N 0x11
#define HID_KEY_O 0x12
#define HID_KEY_R 0x15
#define HID_KEY_S 0x16
#define HID_KEY_T 0x17
#define HID_KEY_U 0x18
#define HID_KEY_DD 0x07  // 'd' repeated from above
#define HID_KEY_L 0x0F
#define HID_KEY_ENTER 0x28
#define HID_KEY_ESC 0x29
#define HID_KEY_BSPACE 0x2A
#define HID_KEY_LEFT_ARROW 0x50
#define HID_KEY_RIGHT_ARROW 0x4F
#define HID_KEY_UP_ARROW 0x52
#define HID_KEY_DOWN_ARROW 0x51
#define HID_KEY_VOLUP 0x80    // or Consumer usage
#define HID_KEY_VOLDOWN 0x81  // or Consumer usage

/*=========================================================================*/
/*           Here are the symbolic names used by decodeCanBus              */
/*=========================================================================*/

// For iDrive Buttons
#define KEY_MENU_KB HID_KEY_M
#define KEY_BACK_KB HID_KEY_B
#define KEY_OPTION_KB HID_KEY_O
#define KEY_RADIO_KB HID_KEY_R
#define KEY_CD_KB HID_KEY_C
#define KEY_NAV_KB HID_KEY_N
#define KEY_TEL_KB HID_KEY_T

// Joystick movements
#define KEY_CENTER_KB HID_KEY_S
#define KEY_UP_KB HID_KEY_U
#define KEY_DOWN_KB HID_KEY_D  // or HID_KEY_DD
#define KEY_LEFT_KB HID_KEY_L
#define KEY_RIGHT_KB HID_KEY_R  // But conflict with 'r'?

// Rotations
#define KEY_ROTATE_PLUS_KB HID_KEY_VOLUP
#define KEY_ROTATE_MINUS_KB HID_KEY_VOLDOWN

// For mouse buttons, define if needed:
#define MOUSE_BTN_LEFT 0x01
#define MOUSE_BTN_RIGHT 0x02
#define MOUSE_BTN_MIDDLE 0x04

#endif
