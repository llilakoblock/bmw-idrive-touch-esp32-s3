// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// Rotary encoder input handler - volume control.

#pragma once

#include "input/input_handler.h"

namespace idrive {

class RotaryHandler : public InputHandler {
   public:
    using InputHandler::InputHandler;

    bool Handle(const InputEvent &event) override;

    void SetEnabled(bool enabled) { enabled_ = enabled; }
    bool IsEnabled() const { return enabled_; }

   private:
    bool enabled_ = true;
};

}  // namespace idrive
