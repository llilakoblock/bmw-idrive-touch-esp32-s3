// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT

#include "idrive/idrive_controller.h"

#include "esp_log.h"

#include "idrive/zbe4_protocol.h"
#include "idrive/zbe4_rev03_protocol.h"
#include "ota/ota_trigger.h"
#include "utils/utils.h"

namespace idrive {

namespace {
const char        *kTag                 = "IDRIVE";
constexpr uint32_t kInitRetryIntervalMs = 5000;
}  // namespace

IDriveController::IDriveController(CanBus &can, UsbHidDevice &hid, const Config &config)
    : can_(can), hid_(hid), config_(config)
{}

void IDriveController::Init()
{
    ESP_LOGI(kTag, "Initializing iDrive controller");

    // Create input handlers.
    auto button     = std::make_unique<ButtonHandler>(hid_);
    button_handler_ = button.get();
    handlers_.push_back(std::move(button));

    auto joystick     = std::make_unique<JoystickHandler>(hid_, config_.joystick_as_mouse,
                                                      config_.joystick_move_step);
    joystick_handler_ = joystick.get();
    handlers_.push_back(std::move(joystick));

    auto rotary     = std::make_unique<RotaryHandler>(hid_);
    rotary_handler_ = rotary.get();
    handlers_.push_back(std::move(rotary));

    auto touchpad     = std::make_unique<TouchpadHandler>(hid_, config_.min_mouse_travel,
                                                      config::kXMultiplier, config::kYMultiplier);
    touchpad_handler_ = touchpad.get();
    handlers_.push_back(std::move(touchpad));

    // Register known controller protocols.
    // Detection is first-come-first-served: whichever protocol's DetectionId()
    // appears first on the CAN bus wins.
    auto zbe4 = std::make_unique<ZBE4Protocol>();
    zbe4->SetEventCallback([this](const InputEvent &e) { OnProtocolEvent(e); });
    protocols_.push_back(std::move(zbe4));

    auto zbe4r03 = std::make_unique<ZBE4Rev03Protocol>();
    zbe4r03->SetEventCallback([this](const InputEvent &e) { OnProtocolEvent(e); });
    protocols_.push_back(std::move(zbe4r03));

    // Set up CAN message callback.
    can_.SetCallback([this](const CanMessage &msg) { OnCanMessage(msg); });

    // Record start time and send initial commands.
    init_start_time_ = utils::GetMillis();
    SendRotaryInit();
    SendLightCommand();

    ESP_LOGI(kTag, "Waiting for controller detection...");
}

void IDriveController::Update()
{
    uint32_t now = utils::GetMillis();

    // Initialize touchpad after protocol is ready.
    if (ProtocolReady() && !touchpad_init_done_) {
        ESP_LOGI(kTag, "Protocol ready (%s), initializing touchpad",
                 active_protocol_->Name());
        SendTouchpadInit();
        last_touchpad_init_time_ = now;
        touchpad_init_done_      = true;
    }

    // Keep sending touchpad init until we get a response on 0xBF.
    if (touchpad_init_done_ && !touchpad_active_) {
        constexpr uint32_t kTouchpadInitRetryMs = 50;
        if (now - last_touchpad_init_time_ >= kTouchpadInitRetryMs) {
            last_touchpad_init_time_ = now;
            SendTouchpadInit();
            static int retry_count = 0;
            if (++retry_count % 20 == 0) {
                ESP_LOGI(kTag, "Touchpad init retry #%d...", retry_count);
            }
        }
    }

    // Mark controller as ready after cooldown.
    if (!ready_ && ProtocolReady() && touchpad_init_done_) {
        if (cooldown_start_time_ == 0) {
            cooldown_start_time_ = now;
        }
        if (now - cooldown_start_time_ > config::kControllerCooldownMs) {
            ready_ = true;
            ESP_LOGI(kTag, "iDrive controller ready! (%s)", active_protocol_->Name());
        }
    }

    // Track light initialization completion.
    if (!light_init_done_) {
        if (now - init_start_time_ > config::kLightInitDurationMs) {
            light_init_done_ = true;
            ESP_LOGI(kTag, "Light init done");
        }
    }

    // Periodic polling.
    if (now - last_poll_time_ >= config_.poll_interval_ms) {
        last_poll_time_ = now;
        SendPollCommand();
        SendTouchpadInit();
    }

    // Periodic light keepalive.
    if (now - last_light_time_ >= config_.light_keepalive_ms) {
        last_light_time_ = now;
        SendLightCommand();
    }

    // Retry rotary init while no protocol has been detected yet.
    if (!active_protocol_ && (now - last_reinit_time_ >= kInitRetryIntervalMs)) {
        last_reinit_time_ = now;
        ESP_LOGW(kTag, "No init response - retrying...");
        SendRotaryInit();
    }
}

void IDriveController::SetLightBrightness(uint8_t brightness)
{
    config_.light_brightness = brightness;
    SendLightCommand();
}

void IDriveController::SetLightEnabled(bool enabled)
{
    light_enabled_ = enabled;
    SendLightCommand();
}

// =============================================================================
// CAN Message Handling
// =============================================================================

void IDriveController::OnCanMessage(const CanMessage &msg)
{
    if (config::kDebugCan) {
        ESP_LOGI(kTag, "RX <- ID: 0x%03lX, DLC:%d, Data: %02X %02X %02X %02X %02X %02X %02X %02X",
                 msg.id, msg.length, msg.data[0], msg.data[1], msg.data[2], msg.data[3],
                 msg.data[4], msg.data[5], msg.data[6], msg.data[7]);
    }

    // Ignore echo of our own transmitted messages.
    if (msg.id == can_id::kRotaryInitCmd || msg.id == can_id::kLight ||
        msg.id == can_id::kPoll || msg.id == can_id::kTouchInitCmd) {
        return;
    }

    // Touchpad and status are shared across all revisions.
    if (msg.id == can_id::kTouch) {
        HandleTouchpadMessage(msg);
        return;
    }
    if (msg.id == can_id::kStatus) {
        HandleStatusMessage(msg);
        return;
    }

    // Auto-detect protocol: first frame matching a DetectionId() wins.
    if (!active_protocol_) {
        for (auto &p : protocols_) {
            if (p->DetectionId() == msg.id) {
                active_protocol_ = p.get();
                active_protocol_->OnDetected();
                ESP_LOGI(kTag, "Detected: %s", active_protocol_->Name());
                break;
            }
        }
    }

    // Delegate to active protocol.
    if (active_protocol_ && active_protocol_->HandlesId(msg.id)) {
        active_protocol_->OnMessage(msg);
    }
}

void IDriveController::HandleTouchpadMessage(const CanMessage &msg)
{
    if (msg.length < 8)
        return;

    // Mark touchpad as active on first response.
    if (!touchpad_active_) {
        touchpad_active_ = true;
        ESP_LOGI(kTag, "Touchpad active! (received 0xBF response)");
    }

    uint8_t touch_type = msg.data[4];

    // Log all touch messages for debugging (including finger removed)
    if (config::kDebugTouchpad) {
        const char *type_str = "?";
        switch (touch_type) {
            case protocol::kTouchFingerRemoved: type_str = "REMOVED"; break;
            case protocol::kTouchSingle:        type_str = "SINGLE";  break;
            case protocol::kTouchMulti:         type_str = "MULTI";   break;
            case protocol::kTouchTriple:        type_str = "TRIPLE";  break;
            case protocol::kTouchQuad:          type_str = "QUAD";    break;
        }
        ESP_LOGD(kTag, "Touch: type=0x%02X (%s) [%02X %02X %02X %02X %02X %02X %02X %02X]",
                 touch_type, type_str, msg.data[0], msg.data[1], msg.data[2], msg.data[3],
                 msg.data[4], msg.data[5], msg.data[6], msg.data[7]);
    }

    // Ignore initial touchpad messages during initialization.
    if (touchpad_init_ignore_counter_ < config::kTouchpadInitIgnoreCount &&
        ProtocolReady()) {
        touchpad_init_ignore_counter_++;
        ESP_LOGI(kTag, "Touchpad ignoring message %d/%d", touchpad_init_ignore_counter_,
                 config::kTouchpadInitIgnoreCount);
        return;
    }

    InputEvent event;
    event.type  = InputEvent::Type::Touchpad;
    event.state = touch_type;

    if (touch_type == protocol::kTouchFingerRemoved) {
        DispatchEvent(event);
        return;
    }

    if (touch_type == protocol::kTouchSingle || touch_type == protocol::kTouchMulti ||
        touch_type == protocol::kTouchTriple || touch_type == protocol::kTouchQuad) {
        // G-series ZBE4 multi-touch protocol (both axes 9-bit, 0-511):
        // Byte 1: Finger 1 X low byte (0-255)
        // Byte 2: [high nibble = F1 Y low 4 bits] [low nibble = F1 X high bit]
        // Byte 3: Finger 1 Y high 5 bits (0-31)
        // Byte 4: Touch state
        // Byte 5: Finger 2 X low byte (0-255)
        // Byte 6: [high nibble = F2 Y low 4 bits] [low nibble = F2 X high bit]
        // Byte 7: Finger 2 Y high 5 bits (0-31)

        event.x = msg.data[1] + 256 * (msg.data[2] & 0x01);
        event.y = (static_cast<int16_t>(msg.data[3]) << 4) | (msg.data[2] >> 4);

        event.two_fingers = (touch_type == protocol::kTouchMulti);

        if (event.two_fingers) {
            event.x2 = msg.data[5] + 256 * (msg.data[6] & 0x01);
            event.y2 = (static_cast<int16_t>(msg.data[7]) << 4) | (msg.data[6] >> 4);
        }

        DispatchEvent(event);
    }
}

void IDriveController::HandleStatusMessage(const CanMessage &msg)
{
    if (msg.length < 5)
        return;

    ESP_LOGD(kTag, "Status message: data[4]=0x%02X", msg.data[4]);

    if (msg.data[4] == protocol::kStatusNoInit) {
        ESP_LOGW(kTag, "iDrive lost init - reinitializing");
        ready_                        = false;
        light_init_done_              = false;
        touchpad_init_done_           = false;
        touchpad_active_              = false;
        cooldown_start_time_          = 0;
        touchpad_init_ignore_counter_ = 0;
        last_reinit_time_             = utils::GetMillis();
        init_start_time_              = utils::GetMillis();

        // Reset protocol state and return to detection mode.
        // The next matching CAN frame will re-detect the same protocol.
        if (active_protocol_) {
            active_protocol_->Reset();
            active_protocol_ = nullptr;
        }

        SendRotaryInit();
    }
}

// =============================================================================
// Protocol Event Callback
// =============================================================================

void IDriveController::OnProtocolEvent(const InputEvent &event)
{
    // Notify OTA trigger for button combo detection.
    if (event.type == InputEvent::Type::Button && ota_trigger_) {
        ota_trigger_->OnButtonEvent(event.id, event.state);
    }
    DispatchEvent(event);
}

// =============================================================================
// CAN Commands
// =============================================================================

void IDriveController::SendRotaryInit()
{
    uint8_t data[8] = {0x1D, 0xE1, 0x00, 0xF0, 0xFF, 0x7F, 0xDE, 0x04};
    can_.Send(can_id::kRotaryInitCmd, data, 8);
    ESP_LOGI(kTag, "Sent rotary init frame");
}

void IDriveController::SendTouchpadInit()
{
    uint8_t data[8] = {0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    can_.Send(can_id::kTouchInitCmd, data, 8);
}

void IDriveController::SendLightCommand()
{
    uint8_t data[2];
    data[0] = light_enabled_ ? 0xFD : 0xFE;
    data[1] = 0x00;
    can_.Send(can_id::kLight, data, 2);
}

void IDriveController::SendPollCommand()
{
    uint8_t data[8] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    can_.Send(can_id::kPoll, data, 8);
}

// =============================================================================
// Event Dispatch
// =============================================================================

void IDriveController::DispatchEvent(const InputEvent &event)
{
    if (!ready_ || !hid_.IsConnected()) {
        return;
    }

    for (auto &handler : handlers_) {
        if (handler->Handle(event)) {
            break;
        }
    }
}

}  // namespace idrive
