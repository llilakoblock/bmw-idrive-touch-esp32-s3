// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// Joystick input handler - mouse movement or arrow keys.

#pragma once

#include "input/input_handler.h"

namespace idrive {

class JoystickHandler : public InputHandler {
   public:
    JoystickHandler(UsbHidDevice &hid, bool as_mouse, int move_step = 30);

    bool Handle(const InputEvent &event) override;

    void SetAsMouse(bool as_mouse) { as_mouse_ = as_mouse; }
    bool IsMouse() const { return as_mouse_; }

   private:
    bool as_mouse_;
    int  move_step_;
};

}  // namespace idrive
