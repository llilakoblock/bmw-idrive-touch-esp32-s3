// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// iDrive ZBE4 revision -03 protocol.
// All input (buttons, joystick, rotary) multiplexed on CAN ID 0x25B.
// Tested with: part number 6582 6829079-03 (date 09.08.16).
//
// Frame format (8 bytes):
//   [0]    Frame counter (monotonically increasing)
//   [1]    Rotary position low byte (0xFF when idle)
//   [2]    Rotary position high byte (0x7F always)
//   [3]    Joystick direction (0x00=none, 0x01=center, 0x10=up, 0x70=down, 0xA0=left, 0x40=right)
//   [4]    MENU (bit2) / BACK (bit5) bitfield
//   [5]    OPTIONS (bit0) / COM (bit3) bitfield
//   [6]    Base 0xC0 + MEDIA (bit0) / NAV (bit3)
//   [7]    Base 0xF8 + MAP (bit0)
//
// Idle/release frame: bytes[3..7] = 00 00 00 C0 F8

#pragma once

#include "idrive/controller_protocol.h"

namespace idrive {

class ZBE4Rev03Protocol : public ControllerProtocol {
public:
    const char *Name()              const override { return "ZBE4-03"; }
    uint32_t    DetectionId()       const override { return 0x25B; }
    bool        HandlesId(uint32_t id) const override { return id == 0x25B; }
    void        OnDetected()              override;
    bool        IsReady()           const override { return init_done_; }
    void        Reset()                   override;
    void        OnMessage(const CanMessage &msg) override;

private:
    bool     init_done_       = false;
    uint32_t position_        = 0;
    bool     position_set_    = false;
    uint8_t  last_button_id_  = 0;
    uint8_t  last_joystick_id_= 0;
    bool     joystick_active_ = false;
    bool     menu_pressed_    = false;
    bool     back_pressed_    = false;

    void HandleButtonEdge(bool now, bool &previous, uint8_t button_id);
};

}  // namespace idrive
