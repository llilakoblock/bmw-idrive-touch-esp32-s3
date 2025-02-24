#include "variables.h"
#include <cstddef>

// default states
unsigned long previousMillis = 0;
unsigned long CoolDownMillis = 0;
int init_can_count = 0;
const int max_can_init_attempts = 15;
bool RotaryInitSuccess = false;
bool PollInit = false;
bool TouchpadInitDone = false;
bool LightInitDone = false;
bool RotaryInitPositionSet = false;
bool controllerReady = false;
bool touching = false;
bool rotaryDisabled = false;

bool iDriveLightOn = true;

unsigned int rotaryposition = 0;

int PreviousX = 0;
int PreviousY = 0;
int ResultX = 0;
int ResultY = 0;
int TouchpadInitIgnoreCounter = 0;

const int8_t mouse_low_range = -128;
const int8_t mouse_high_range = 127;
const int8_t mouse_center_range = 0;
const float powerValue = 1.4;

const int mouse_low_range_v2 = 0;
const int mouse_high_range_v2 = 510;
const int mouse_center_range_v2 = mouse_high_range_v2 / 2;
const int x_multiplier = 2;
const int y_multiplier = 12;

// Key states
uint8_t stati[256] = {0};
