#ifndef SETTINGS_H
#define SETTINGS_H

/*=========================================================================*/
/*  Board & Feature Settings                                               */
/*=========================================================================*/

#define ANDROID  // Android mode enabled - uses Android-specific media keys

// iDrive Joystick can be used as arrow keys or as mouse
#define iDriveJoystickAsMouse  // Enable for mouse mode, comment out for arrow keys

#define MOUSE_V1  // Touchpad data format version
// #define MOUSE_V2

/*=========================================================================*/
/*  iDrive Timing & Behavior                                               */
/*=========================================================================*/

const long iDrivePollTime          = 500;    // ms - how often to poll iDrive
const long iDriveInitLightTime     = 1000;   // ms - light on during init
const long iDriveLightTime         = 10000;  // ms - light keepalive interval
const long controllerCoolDown      = 750;    // ms - wait before ready
const int  TouchpadInitIgnoreCount = 2;      // Ignore first N touchpad messages
const int  min_mouse_travel        = 5;      // Minimum movement threshold

#define JOYSTICK_MOVE_STEP 30  // Mouse movement step for joystick

// Android-specific button mappings:
// MENU button -> Android Menu
// BACK button -> Android Back
// OPTION button -> Play/Pause
// RADIO button -> Previous track
// CD button -> Next track
// NAV button -> Android Home
// TEL button -> Android Search
// Rotary knob -> Volume up/down
// Touchpad -> Mouse cursor
// Joystick center -> Mouse click (or Enter key)

/*=========================================================================*/
/*  USB Device Settings                                                    */
/*=========================================================================*/

// USB descriptors - these will appear in Android device manager
#define USB_VENDOR_ID    0x1234  // Change to your vendor ID if you have one
#define USB_PRODUCT_ID   0x5678  // Change to your product ID
#define USB_MANUFACTURER "BMW"
#define USB_PRODUCT      "iDrive Controller"
#define USB_SERIAL_NUM   "123456"

/*=========================================================================*/
/*  Mouse/Touchpad Sensitivity                                             */
/*=========================================================================*/

// V1 touchpad settings (8-bit signed coordinates)
const int8_t mouse_low_range    = -128;
const int8_t mouse_high_range   = 127;
const int8_t mouse_center_range = 0;
const float  powerValue         = 1.4;

// V2 touchpad settings (16-bit unsigned coordinates)
const int mouse_low_range_v2    = 0;
const int mouse_high_range_v2   = 510;
const int mouse_center_range_v2 = mouse_high_range_v2 / 2;

// Sensitivity multipliers - adjust these for your preference
const int x_multiplier = 15;  // Horizontal sensitivity (higher = faster)
const int y_multiplier = 15;  // Vertical sensitivity (higher = faster)

/*=========================================================================*/
/*  Debugging                                                              */
/*=========================================================================*/

#define SERIAL_DEBUG
#define DEBUG_CanResponse
// #define DEBUG_Keys
// #define DEBUG_TouchPad
// #define DEBUG_SpecificCanID
// #define DEBUG_ID 0xBF

// CAN IDs to ignore in debug output (reduces log spam)
// Defined in idrive.cpp
extern int ignored_responses[];

/*=========================================================================*/
/*  Light Control                                                          */
/*=========================================================================*/

// Which button turns off the backlight (set to 0 to disable)
#define LIGHT_OFF_BUTTON MSG_INPUT_BUTTON_OPTION

// Auto-dim settings
#define AUTO_DIM_ENABLED    1
#define AUTO_DIM_TIMEOUT    30000  // ms - dim after 30 seconds of inactivity
#define AUTO_DIM_BRIGHTNESS 50     // % - dimmed brightness level

#endif  // SETTINGS_H