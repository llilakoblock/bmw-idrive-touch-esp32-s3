// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// iDrive ZBE4 protocol (G-series, original revision).
// CAN IDs: 0x267 (buttons/joystick), 0x264 (rotary), 0x277 (init response).
// Tested with: part number 65826829079.

#pragma once

#include "idrive/controller_protocol.h"

namespace idrive {

class ZBE4Protocol : public ControllerProtocol {
public:
    const char *Name()              const override { return "ZBE4"; }
    uint32_t    DetectionId()       const override { return 0x277; }
    bool        HandlesId(uint32_t id) const override;
    void        OnDetected()              override;
    bool        IsReady()           const override { return init_done_; }
    void        Reset()                   override;
    void        OnMessage(const CanMessage &msg) override;

private:
    bool     init_done_       = false;
    uint32_t rotary_position_ = 0;
    bool     position_set_    = false;

    void HandleInputMessage(const CanMessage &msg);
    void HandleRotaryMessage(const CanMessage &msg);
};

}  // namespace idrive
