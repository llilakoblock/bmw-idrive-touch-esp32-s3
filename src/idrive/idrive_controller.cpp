// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT

#include "idrive/idrive_controller.h"

#include "esp_log.h"

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

    // Set up CAN message callback.
    can_.SetCallback([this](const CanMessage &msg) { OnCanMessage(msg); });

    // Record start time.
    init_start_time_ = utils::GetMillis();

    // Send initial commands.
    SendRotaryInit();
    SendLightCommand();

    ESP_LOGI(kTag, "iDrive controller initialized, waiting for response...");
}

void IDriveController::Update()
{
    uint32_t now = utils::GetMillis();

    // Initialize touchpad after rotary is ready.
    if (rotary_init_done_ && !touchpad_init_done_) {
        ESP_LOGI(kTag, "Rotary init done, initializing touchpad");
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
            // Log only every 20th attempt (~1 sec) to reduce spam.
            static int retry_count = 0;
            if (++retry_count % 20 == 0) {
                ESP_LOGI(kTag, "Touchpad init retry #%d...", retry_count);
            }
        }
    }

    // Mark controller as ready after cooldown.
    if (!ready_ && rotary_init_done_ && touchpad_init_done_) {
        if (cooldown_start_time_ == 0) {
            cooldown_start_time_ = now;
        }
        if (now - cooldown_start_time_ > config::kControllerCooldownMs) {
            ready_ = true;
            ESP_LOGI(kTag, "iDrive controller ready!");
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
        // G-series ZBE4 needs continuous touchpad polling.
        // Always send - don't wait for touchpad_active_
        SendTouchpadInit();
    }

    // Periodic light keepalive.
    if (now - last_light_time_ >= config_.light_keepalive_ms) {
        last_light_time_ = now;
        SendLightCommand();
    }

    // Retry initialization if no response.
    if (!rotary_init_done_ && (now - last_reinit_time_ >= kInitRetryIntervalMs)) {
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
    // Log incoming messages for debugging.
    if (config::kDebugCan) {
        ESP_LOGI(kTag, "RX <- ID: 0x%03lX, DLC:%d, Data: %02X %02X %02X %02X %02X %02X %02X %02X",
                 msg.id, msg.length, msg.data[0], msg.data[1], msg.data[2], msg.data[3],
                 msg.data[4], msg.data[5], msg.data[6], msg.data[7]);
    }

    // Ignore our own transmitted messages (echo).
    if (msg.id == can_id::kRotaryInitCmd || msg.id == can_id::kLight || msg.id == can_id::kPoll) {
        return;
    }

    // Process by message ID.
    switch (msg.id) {
        case can_id::kInput:
            HandleInputMessage(msg);
            break;
        case can_id::kRotary:
            HandleRotaryMessage(msg);
            break;
        case can_id::kTouch:
            HandleTouchpadMessage(msg);
            break;
        case can_id::kRotaryInit:
            HandleRotaryInitResponse(msg);
            break;
        case can_id::kStatus:
            HandleStatusMessage(msg);
            break;
    }
}

void IDriveController::HandleInputMessage(const CanMessage &msg)
{
    if (msg.length < 6)
        return;

    uint8_t state      = msg.data[3] & 0x0F;
    uint8_t input_type = msg.data[4];
    uint8_t input      = msg.data[5];

    if (config::kDebugKeys) {
        ESP_LOGI(kTag, "Input: type=0x%02X, id=0x%02X, state=%d", input_type, input, state);
    }

    InputEvent event;

    if (input_type == protocol::kInputTypeButton) {
        // Forward button events to OTA trigger for combo detection.
        if (ota_trigger_) {
            ota_trigger_->OnButtonEvent(input, state);
        }
        event.type  = InputEvent::Type::Button;
        event.id    = input;
        event.state = state;
    } else if (input_type == protocol::kInputTypeStick) {
        event.type  = InputEvent::Type::Joystick;
        event.id    = msg.data[3] >> 4;  // Direction in upper nibble.
        event.state = state;
    } else if (input_type == protocol::kInputTypeCenter) {
        event.type  = InputEvent::Type::Joystick;
        event.id    = protocol::kStickCenter;
        event.state = state;
    } else {
        return;
    }

    DispatchEvent(event);
}

void IDriveController::HandleRotaryMessage(const CanMessage &msg)
{
    if (msg.length < 5)
        return;

    ESP_LOGD(kTag, "Rotary data received");

    if (!rotary_position_set_) {
        // Set initial position.
        uint8_t  rotary_step_b = msg.data[4];
        uint32_t new_pos       = msg.data[3] + (msg.data[4] * 0x100);

        switch (rotary_step_b) {
            case 0x7F:
                rotary_position_ = new_pos + 1;
                break;
            case 0x80:
                rotary_position_ = new_pos - 1;
                break;
            default:
                rotary_position_ = new_pos;
                break;
        }
        rotary_position_set_ = true;
        ESP_LOGI(kTag, "Rotary initial position: %lu", rotary_position_);
        return;
    }

    uint32_t new_position = msg.data[3] + (msg.data[4] * 0x100);
    int      delta        = static_cast<int>(new_position) - static_cast<int>(rotary_position_);

    // Handle wraparound.
    if (delta > 32768) {
        delta -= 65536;
    } else if (delta < -32768) {
        delta += 65536;
    }

    if (delta != 0) {
        InputEvent event;
        event.type  = InputEvent::Type::Rotary;
        event.delta = delta;
        DispatchEvent(event);
        rotary_position_ = new_position;
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

    // Only log when there's actual touch data (skip 0x11 = no finger)
    if (config::kDebugTouchpad && touch_type != protocol::kTouchFingerRemoved) {
        // Print raw CAN message for analysis
        ESP_LOGI(kTag, "Touch RAW: [%02X %02X %02X %02X %02X %02X %02X %02X]", msg.data[0],
                 msg.data[1], msg.data[2], msg.data[3], msg.data[4], msg.data[5], msg.data[6],
                 msg.data[7]);
    }

    // Ignore initial touchpad messages during initialization.
    if (touchpad_init_ignore_counter_ < config::kTouchpadInitIgnoreCount && rotary_init_done_) {
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

        // Finger 1 coordinates (0-511 range each)
        event.x = msg.data[1] + 256 * (msg.data[2] & 0x01);
        event.y = (static_cast<int16_t>(msg.data[3]) << 4) | (msg.data[2] >> 4);

        // Check for multi-touch (state 0x00 = two fingers)
        event.two_fingers = (touch_type == protocol::kTouchMulti);

        if (event.two_fingers) {
            // Finger 2 coordinates (0-511 range each)
            event.x2 = msg.data[5] + 256 * (msg.data[6] & 0x01);
            event.y2 = (static_cast<int16_t>(msg.data[7]) << 4) | (msg.data[6] >> 4);
        }

        DispatchEvent(event);
    }
}

void IDriveController::HandleRotaryInitResponse(const CanMessage &msg)
{
    (void) msg;
    ESP_LOGI(kTag, "Rotary Init Success");
    rotary_init_done_ = true;
}

void IDriveController::HandleStatusMessage(const CanMessage &msg)
{
    if (msg.length < 5)
        return;

    ESP_LOGD(kTag, "Status message: data[4]=0x%02X", msg.data[4]);

    if (msg.data[4] == protocol::kStatusNoInit) {
        // Lost initialization - reinitialize.
        ESP_LOGW(kTag, "iDrive lost init - reinitializing");
        ready_                        = false;
        rotary_init_done_             = false;
        light_init_done_              = false;
        touchpad_init_done_           = false;
        touchpad_active_              = false;
        rotary_position_set_          = false;
        cooldown_start_time_          = 0;
        touchpad_init_ignore_counter_ = 0;
        init_start_time_              = utils::GetMillis();

        SendRotaryInit();
    }
}

// =============================================================================
// CAN Commands
// =============================================================================

void IDriveController::SendRotaryInit()
{
    uint8_t data[8] = {0x1D, 0xE1, 0x00, 0xF0, 0xFF, 0x7F, 0xDE, 0x04};
    can_.Send(can_id::kRotaryInitCmd, data, 8);
    rotary_position_set_ = false;
    ESP_LOGI(kTag, "Sent rotary init frame");
}

void IDriveController::SendTouchpadInit()
{
    // G-series ZBE4 touchpad poll message.
    // byte[0] bit4 (0x10) must be SET for coordinates to update.
    // Cycling is NOT required - fixed 0x10 works perfectly.
    uint8_t data[8] = {0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    can_.Send(can_id::kTouchInitCmd, data, 8);
}

void IDriveController::SendLightCommand()
{
    // Light ON: 0xFD 0x00, Light OFF: 0xFE 0x00 (as per original code)
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
            break;  // Event was handled.
        }
    }
}

}  // namespace idrive
