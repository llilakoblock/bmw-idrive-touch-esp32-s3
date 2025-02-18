#ifndef SETTINGS_H
#define SETTINGS_H

/*=========================================================================*/
/*  Board & Feature Settings                                               */
/*=========================================================================*/

#define USE_BLEKEYBOARDCODE  // (Kept for historical references, not actually
                             // using BLE)
#define ANDROID  // Or comment out if you want desktop keys vs. Android keys

// iDrive Joystick can be used as arrow keys or as mouse
// #define iDriveJoystickAsMouse

// MOUSE method version
#define MOUSE_V1
// #define MOUSE_V2

/*=========================================================================*/
/*  iDrive Timing & Behavior                                               */
/*=========================================================================*/

const long iDrivePollTime = 500;        // ms
const long iDriveInitLightTime = 1000;  // ms
const long iDriveLightTime = 10000;     // ms
const long controllerCoolDown = 750;    // ms
const int TouchpadInitIgnoreCount = 2;
const int min_mouse_travel = 10;

#define JOYSTICK_MOVE_STEP 50  // Not used in the original code?

#define LIGHT_OFF_BUTTON \
  'o'  // Key that toggles iDriveLightOn (was OPTION button in original code)

/*=========================================================================*/
/*  Debugging                                                              */
/*=========================================================================*/

// Uncomment or comment as you like
#define SERIAL_DEBUG
#define DEBUG_CanResponse
// #define DEBUG_Keys
// #define DEBUG_TouchPad
// #define DEBUG_SpecificCanID
// #define DEBUG_ID 0xBF

/*=========================================================================*/
/*  CAN Response IDs to ignore (in original code).                          */
/*  But we will log everything as requested. If you want to skip some, you  */
/*  can use this array.                                                    */
/*=========================================================================*/
static int ignored_responses[] = {0x264, 0x267, 0x277, 0x567, 0x5E7, 0xBF};

#endif  // SETTINGS_H
