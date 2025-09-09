#ifndef IDRIVE_H
#define IDRIVE_H

#include <stdint.h>

// Incoming
#define MSG_IN_INPUT 0x267
#define MSG_INPUT_BUTTON 0xC0
#define MSG_INPUT_BUTTON_MENU 0x01
#define MSG_INPUT_BUTTON_BACK 0x02
#define MSG_INPUT_BUTTON_OPTION 0x04
#define MSG_INPUT_BUTTON_RADIO 0x08
#define MSG_INPUT_BUTTON_CD 0x10
#define MSG_INPUT_BUTTON_NAV 0x20
#define MSG_INPUT_BUTTON_TEL 0x40

#define MSG_INPUT_CENTER 0xDE
#define MSG_INPUT_STICK 0xDD
#define MSG_INPUT_STICK_UP 0x01
#define MSG_INPUT_STICK_RIGHT 0x02
#define MSG_INPUT_STICK_DOWN 0x04
#define MSG_INPUT_STICK_LEFT 0x08
#define MSG_INPUT_STICK_CENTER 0x00

#define MSG_INPUT_RELEASED 0x00
#define MSG_INPUT_PRESSED 0x01
#define MSG_INPUT_HELD 0x02

#define MSG_IN_ROTARY 0x264
#define MSG_IN_ROTARY_INIT 0x277

#define MSG_IN_STATUS 0x5E7
#define MSG_STATUS_NO_INIT 0x06

#define MSG_IN_TOUCH 0xBF
#define FINGER_REMOVED 0x11
#define SINGLE_TOUCH 0x10
#define MULTI_TOUCH 0x00
#define TRIPLE_TOUCH 0x1F
#define QUAD_TOUCH 0x0F

#define MSG_IN_TOUCH_V2 0xFFFFFFBF  // for debugging only

// Outgoing
#define MSG_OUT_ROTARY_INIT 0x273
#define MSG_OUT_LIGHT 0x202
#define MSG_OUT_POLL 0x501  // or 0x563

// Functions
void iDriveInit();
void iDriveTouchpadInit();
void iDrivePoll(unsigned long milliseconds);
void iDriveLight(unsigned long milliseconds);
void decodeCanBus(unsigned long canId, uint8_t len, uint8_t buf[8]);

void do_iDrivePoll();
void do_iDriveLight();

bool isvalueinarray(int val, int *arr, int size);

#endif
