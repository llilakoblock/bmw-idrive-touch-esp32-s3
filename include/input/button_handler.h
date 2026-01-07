// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// Button input handler - maps iDrive buttons to media keys.

#pragma once

#include "input/input_handler.h"

namespace idrive {

class ButtonHandler : public InputHandler {
   public:
    using InputHandler::InputHandler;

    bool Handle(const InputEvent &event) override;
};

}  // namespace idrive
