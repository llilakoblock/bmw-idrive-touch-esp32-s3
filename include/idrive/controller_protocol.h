// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT
//
// Abstract protocol interface for iDrive controller CAN variants.
// Each hardware revision implements this to decode CAN frames into InputEvents.
//
// To add support for a new revision:
//   1. Create a subclass of ControllerProtocol
//   2. Register it in IDriveController::Init()

#pragma once

#include <cstdint>
#include <functional>

#include "can/can_bus.h"
#include "input/input_handler.h"

namespace idrive {

class ControllerProtocol {
public:
    using EventCallback = std::function<void(const InputEvent &)>;

    virtual ~ControllerProtocol() = default;

    // Human-readable name for logging.
    virtual const char *Name() const = 0;

    // CAN ID whose first arrival on the bus triggers auto-detection of this protocol.
    // Return 0 to disable auto-detection (manual activation only).
    virtual uint32_t DetectionId() const = 0;

    // Returns true if this protocol should handle the given CAN ID.
    // Called after the protocol has been activated.
    virtual bool HandlesId(uint32_t id) const = 0;

    // Called once when this protocol is selected via auto-detection.
    // Use this to set the initial ready state.
    virtual void OnDetected() = 0;

    // Returns true when the protocol considers itself initialized and ready
    // for normal input processing.
    virtual bool IsReady() const = 0;

    // Reset to pre-init state. Called when a status message signals lost init.
    // The protocol will be re-detected on the next matching CAN frame.
    virtual void Reset() = 0;

    // Process an incoming CAN message. Use Emit() to produce InputEvents.
    virtual void OnMessage(const CanMessage &msg) = 0;

    void SetEventCallback(EventCallback cb) { callback_ = std::move(cb); }

protected:
    void Emit(const InputEvent &event)
    {
        if (callback_) callback_(event);
    }

private:
    EventCallback callback_;
};

}  // namespace idrive
