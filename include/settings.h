#ifndef SETTINGS_H
#define SETTINGS_H

/*=========================================================================*/
/*  Board & Feature Settings                                               */
/*=========================================================================*/

#define ANDROID  // Or comment out if you want desktop keys vs. Android keys

// iDrive Joystick can be used as arrow keys or as mouse
// #define iDriveJoystickAsMouse

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

#define JOYSTICK_MOVE_STEP 50  

#define LIGHT_OFF_BUTTON \
  'o'  
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

static int ignored_responses[] = {0x264, 0x267, 0x277, 0x567, 0x5E7, 0xBF};

#endif  // SETTINGS_H
