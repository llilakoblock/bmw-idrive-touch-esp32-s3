#ifndef VARIABLES_H
#define VARIABLES_H

#include <cstddef>
#include <cstdint>
/*==========================================================================*/
/* Shared Global Variables (from original)                                  */
/*==========================================================================*/

// CAN settings
extern unsigned long previousMillis;
extern unsigned long CoolDownMillis;
extern int init_can_count;
extern const int max_can_init_attempts;
extern bool RotaryInitSuccess;
extern bool PollInit;
extern bool TouchpadInitDone;
extern bool LightInitDone;
extern bool RotaryInitPositionSet;
extern bool controllerReady;
extern bool touching;
extern bool rotaryDisabled;

// iDrive or user-specified states
extern bool iDriveLightOn;

// Rotary position for decoding
extern unsigned int rotaryposition;

// For advanced mouse handling
extern int PreviousX;
extern int PreviousY;
extern int ResultX;
extern int ResultY;
extern int TouchpadInitIgnoreCounter;

// mouse ranges
extern const int8_t mouse_low_range;
extern const int8_t mouse_high_range;
extern const int8_t mouse_center_range;
extern const float powerValue;

extern const int mouse_low_range_v2;
extern const int mouse_high_range_v2;
extern const int mouse_center_range_v2;
extern const int x_multiplier;
extern const int y_multiplier;

// Key state array
extern uint8_t stati[256];

#endif
