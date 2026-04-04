// Copyright 2024 BMW iDrive ESP32-S3 Project
// SPDX-License-Identifier: MIT

#include "idrive/idrive_controller.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "ota/ota_trigger.h"
#include "utils/utils.h"

namespace idrive {

namespace {
const char        *kTag                 = "IDRIVE";
constexpr uint32_t kInitRetryIntervalMs = 5000;
constexpr uint32_t kOptionResetHoldTimeMs  = 5000;  // 5 seconds
}  // namespace

IDriveController::IDriveController(CanBus &can, UsbHidDevice &hid, const Config &config)
    : can_(can), hid_(hid), config_(config)
{}

void IDriveController::MarkActivity()
{
    last_activity_time_ = utils::GetMillis();
}

void IDriveController::EnterSleepMode()
{
    ESP_LOGI(kTag, "No activity for %lu ms - entering inactivity sleep",
             config_.inactivity_timeout_ms);

    // Send light OFF once, but do not change the logical light_enabled_ setting.
    uint8_t off_data[2] = {0xFE, 0x00};
    can_.Send(can_id::kLight, off_data, 2);

    ready_                        = false;
    rotary_init_done_             = false;
    touchpad_init_done_           = false;
    touchpad_active_              = false;
    light_init_done_              = false;
    rotary_position_set_          = false;
    controller_awake_             = false;
    startup_commands_sent_        = false;
    sleep_mode_active_            = true;
    cooldown_start_time_          = 0;
    last_poll_time_               = 0;
    last_light_time_              = 0;
    last_reinit_time_             = 0;
    last_touchpad_init_time_      = 0;
    last_activity_time_           = 0;
    touchpad_init_ignore_counter_ = 0;

    // Reset 0x25B parser state.
    alt_rotary_position_         = 0;
    alt_rotary_position_set_     = false;
    alt_joystick_active_         = false;
    last_alt_button_id_          = 0;
    last_alt_joystick_id_        = 0;
    alt_menu_pressed_            = false;
    alt_back_pressed_            = false;

    // Reset OPTION long-hold state.
    option_held_                = false;
    option_long_hold_triggered_ = false;
    if (option_reset_timer_ != nullptr) {
        esp_timer_stop(option_reset_timer_);
    }
}

void IDriveController::InitOptionResetTimer()
{
    if (option_reset_timer_ != nullptr) {
        return;
    }

    esp_timer_create_args_t timer_args = {};
    timer_args.callback = &IDriveController::OptionResetTimerCallback;
    timer_args.arg = this;
    timer_args.dispatch_method = ESP_TIMER_TASK;
    timer_args.name = "option_reset";

    esp_err_t err = esp_timer_create(&timer_args, &option_reset_timer_);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "Failed to create OPTION reset timer: %s", esp_err_to_name(err));
        option_reset_timer_ = nullptr;
        return;
    }

    ESP_LOGI(kTag, "OPTION long-hold reset timer initialized (%lu ms)",
             kOptionResetHoldTimeMs);
}

void IDriveController::HandleOptionResetState(bool pressed)
{
    if (pressed) {
        if (option_held_) {
            return;
        }

        option_held_ = true;
        option_long_hold_triggered_ = false;

        if (option_reset_timer_ != nullptr) {
            esp_timer_stop(option_reset_timer_);
            esp_timer_start_once(option_reset_timer_,
                                 static_cast<uint64_t>(kOptionResetHoldTimeMs) * 1000ULL);
            ESP_LOGI(kTag, "OPTION held - reset armed (%lu ms)", kOptionResetHoldTimeMs);
        }
    } else {
        if (!option_held_) {
            return;
        }

        option_held_ = false;

        if (option_reset_timer_ != nullptr) {
            esp_timer_stop(option_reset_timer_);
        }

        if (!option_long_hold_triggered_) {
            ESP_LOGI(kTag, "OPTION short press detected");
            SendOptionShortPress();
        } else {
            ESP_LOGI(kTag, "OPTION released after long hold");
        }
    }
}

void IDriveController::SendOptionShortPress()
{
    if (!ready_ || !hid_.IsConnected()) {
        return;
    }

    InputEvent press;
    press.type  = InputEvent::Type::Button;
    press.id    = protocol::kButtonOption;
    press.state = protocol::kInputPressed;
    DispatchEvent(press);

    InputEvent release = press;
    release.state = protocol::kInputReleased;
    DispatchEvent(release);
}

void IDriveController::OptionResetTimerCallback(void *arg)
{
    auto *self = static_cast<IDriveController *>(arg);
    if (self == nullptr) {
        return;
    }

    if (self->option_held_) {
        self->option_long_hold_triggered_ = true;
        ESP_LOGW(kTag, "OPTION held for %lu ms - restarting ESP32",
                 kOptionResetHoldTimeMs);
        esp_restart();
    }
}

void IDriveController::Init()
{
    ESP_LOGI(kTag, "Initializing iDrive controller");

    InitOptionResetTimer();

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

    // Do NOT send startup commands yet.
    // Wait until the controller shows first life signs on the CAN bus.
    ESP_LOGI(kTag, "iDrive controller initialized, waiting for controller wake...");

    // PART OF THE OLD CODE
    // Record start time.
    // init_start_time_ = utils::GetMillis();
    // Send initial commands.
    // SendRotaryInit();
    // SendLightCommand();
    // ESP_LOGI(kTag, "iDrive controller initialized, waiting for response...");
}

void IDriveController::Update()
{
    uint32_t now = utils::GetMillis();

    if (sleep_mode_active_) {
        return;
    }

    // Stay quiet until the controller wakes and sends something first.
    if (!controller_awake_) {
        return;
    }

    // Send startup commands once, after wake is detected.
    if (!startup_commands_sent_) {
        init_start_time_ = now;
        SendRotaryInit();
        SendLightCommand();
        startup_commands_sent_ = true;
        ESP_LOGI(kTag, "Controller awake - sent startup commands");
        return;
    }

    // Sleep mode after period of inactivity.
    if (config_.inactivity_timeout_ms > 0 &&
        last_activity_time_ != 0 &&
        now - last_activity_time_ >= config_.inactivity_timeout_ms) {
        EnterSleepMode();
        return;
    }

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
    // if (config::kDebugCan) {
   //  if (config::kDebugCan && msg.id != can_id::kTouch) {
    if (config::kDebugCan &&
    (msg.id == 0x25B ||
     msg.id == can_id::kInput ||
     msg.id == can_id::kRotary ||
     msg.id == can_id::kRotaryInit)) {
        ESP_LOGI(kTag, "RX <- ID: 0x%03lX, DLC:%d, Data: %02X %02X %02X %02X %02X %02X %02X %02X",
                 msg.id, msg.length, msg.data[0], msg.data[1], msg.data[2], msg.data[3],
                 msg.data[4], msg.data[5], msg.data[6], msg.data[7]);
    }

    // Ignore our own transmitted messages (echo).
    if (msg.id == can_id::kRotaryInitCmd || msg.id == can_id::kLight || msg.id == can_id::kPoll) {
        return;
    }

    // While in inactivity sleep, ignore everything except a real wake/input frame.
    if (sleep_mode_active_) {
        if (msg.id == can_id::kInputAlt ||
            msg.id == can_id::kInput ||
            msg.id == can_id::kRotary) {
            sleep_mode_active_ = false;
            controller_awake_  = true;
            MarkActivity();
            ESP_LOGI(kTag, "Wake detected after inactivity sleep (ID 0x%03lX)", msg.id);
        }
        return;
    }
    
    // First real incoming frame means the controller is awake.
    if (!controller_awake_) {
        controller_awake_ = true;
        MarkActivity();
        ESP_LOGI(kTag, "Controller wake detected from CAN activity (ID 0x%03lX)", msg.id);
    }

    // Process by message ID.
    switch (msg.id) {
        case can_id::kInput:
            HandleInputMessage(msg);
            break;
        case can_id::kInputAlt:
            HandleInputAltMessage(msg);
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

        if (input == protocol::kButtonOption) {
            MarkActivity();
            bool pressed =
                (state == protocol::kInputPressed || state == protocol::kInputHeld);
            HandleOptionResetState(pressed);
            return;  // Do not dispatch raw OPTION press/release immediately.
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

    MarkActivity();
    DispatchEvent(event);
}

void IDriveController::HandleInputAltMessage(const CanMessage &msg)
{
    if (msg.length < 8)
        return;

    if (!rotary_init_done_) {
            rotary_init_done_ = true;
            ESP_LOGI(kTag, "0x25B activity detected - marking rotary init done");
    }

    uint16_t new_position =
        static_cast<uint16_t>(msg.data[1]) |
        (static_cast<uint16_t>(msg.data[2]) << 8);

    // MENU/BACK are encoded as bitfields in byte 4 on the 0x25B path.
    // From logs:
    // 0x04 = MENU press
    // 0x20 = BACK press
    // 0x24 = MENU + BACK
    // 0x44 / 0x48 = held-state variants
    bool menu_now = (msg.data[4] & 0x0C) != 0;
    bool back_now = (msg.data[4] & 0x60) != 0;

    auto handle_alt_button_edge = [this](bool now, bool &previous, uint8_t button_id) {
        if (now == previous) {
            return;
        }

        previous = now;
        uint8_t state = now ? protocol::kInputPressed : protocol::kInputReleased;

        if (config::kDebugKeys) {
            ESP_LOGI(kTag, "ALT button %s: id=0x%02X",
                     now ? "press" : "release", button_id);
        }

        if (ota_trigger_) {
            ota_trigger_->OnButtonEvent(button_id, state);
        }

        MarkActivity();

        InputEvent event;
        event.type  = InputEvent::Type::Button;
        event.id    = button_id;
        event.state = state;
        DispatchEvent(event);
    };

    handle_alt_button_edge(menu_now, alt_menu_pressed_, protocol::kButtonMenu);
    handle_alt_button_edge(back_now, alt_back_pressed_, protocol::kButtonBack);

    // "Idle" non-rotary frame for your controller:
    // 00 00 00 C0 F8 in bytes 3..7
    bool idle_controls =
        (msg.data[3] == 0x00 &&
         msg.data[4] == 0x00 &&
         msg.data[5] == 0x00 &&
         msg.data[6] == 0xC0 &&
         msg.data[7] == 0xF8);

    // MENU/BACK are fully handled above, including combo state for OTA.
    if (menu_now || back_now) {
        return;
    }

    // OPTION held-state frame on this controller.
    // We already armed the timer on initial press, so this should not be logged
    // as an unknown control frame.
    if (option_held_ &&
        msg.data[3] == 0x00 &&
        msg.data[4] == 0x00 &&
        msg.data[5] == 0x02 &&
        msg.data[6] == 0xC0 &&
        msg.data[7] == 0xF8) {
        return;
    }

    // -------------------------------------------------------------------------
    // Press frames: joystick or buttons
    // -------------------------------------------------------------------------
    if (!idle_controls) {
        // =========================
        // Joystick family in byte 3
        // =========================
        if (msg.data[3] != 0x00) {
            uint8_t joy_id = 0;
            bool known = true;

            switch (msg.data[3]) {
                case 0x01: joy_id = protocol::kStickCenter; break;
                case 0x10: joy_id = protocol::kStickUp;     break;
                case 0x70: joy_id = protocol::kStickDown;   break;
                case 0xA0: joy_id = protocol::kStickLeft;   break;
                case 0x40: joy_id = protocol::kStickRight;  break;
                default:
                    known = false;
                    break;
            }

            if (known) {
                last_alt_joystick_id_ = joy_id;
                alt_joystick_active_  = true;

                if (config::kDebugKeys) {
                    ESP_LOGI(kTag,
                             "ALT joystick press: raw=0x%02X -> joy=0x%02X",
                             msg.data[3], joy_id);
                }

                MarkActivity();

                InputEvent event;
                event.type  = InputEvent::Type::Joystick;
                event.id    = joy_id;
                event.state = protocol::kInputPressed;
                DispatchEvent(event);
                return;
            }
        }

        // =========================
        // Physical buttons
        // =========================
        uint8_t button_id = 0;
        bool known_button = true;

        // MENU/BACK are handled above as bitfields on byte 4,
        // because they can be pressed together for OTA.
        if (msg.data[5] == 0x01) {
            button_id = protocol::kButtonOption;   // OPTIONS
        } else if (msg.data[5] == 0x08) {
            button_id = protocol::kButtonTel;      // COM
        } else if (msg.data[6] == 0xC1) {
            button_id = protocol::kButtonCd;       // MEDIA
        } else if (msg.data[6] == 0xC8) {
            button_id = protocol::kButtonNav;      // NAV
        } else if (msg.data[7] == 0xF9) {
            button_id = protocol::kButtonMap;      // MAP
        } else {
            known_button = false;
        }

        if (known_button && button_id != 0) {
            last_alt_button_id_ = button_id;

            if (config::kDebugKeys) {
                ESP_LOGI(kTag,
                         "ALT button press: mapped=0x%02X raw=[%02X %02X %02X %02X %02X]",
                         button_id,
                         msg.data[3], msg.data[4], msg.data[5], msg.data[6], msg.data[7]);
            }

            if (ota_trigger_) {
                ota_trigger_->OnButtonEvent(button_id, protocol::kInputPressed);
            }

            MarkActivity();

            if (button_id == protocol::kButtonOption) {
                HandleOptionResetState(true);
                return;  // Do not dispatch raw OPTION press immediately.
            }

            InputEvent event;
            event.type  = InputEvent::Type::Button;
            event.id    = button_id;
            event.state = protocol::kInputPressed;
            DispatchEvent(event);
            return;
        }

        if (config::kDebugKeys) {
            ESP_LOGI(kTag,
                     "ALT unknown control frame: [%02X %02X %02X %02X %02X]",
                     msg.data[3], msg.data[4], msg.data[5], msg.data[6], msg.data[7]);
        }
        return;
    }

    // -------------------------------------------------------------------------
    // Release frames
    // -------------------------------------------------------------------------
    if (alt_joystick_active_) {
        if (config::kDebugKeys) {
            ESP_LOGI(kTag, "ALT joystick release: joy=0x%02X", last_alt_joystick_id_);
        }

        MarkActivity();

        InputEvent event;
        event.type  = InputEvent::Type::Joystick;
        event.id    = last_alt_joystick_id_;
        event.state = protocol::kInputReleased;
        DispatchEvent(event);

        alt_joystick_active_  = false;
        last_alt_joystick_id_ = 0;
        return;
    }

    if (last_alt_button_id_ != 0) {
        if (config::kDebugKeys) {
            ESP_LOGI(kTag, "ALT button release: id=0x%02X", last_alt_button_id_);
        }

        if (ota_trigger_) {
            ota_trigger_->OnButtonEvent(last_alt_button_id_, protocol::kInputReleased);
        }

        if (last_alt_button_id_ == protocol::kButtonOption) {
            HandleOptionResetState(false);
            last_alt_button_id_ = 0;
            return;  // Do not dispatch raw OPTION release.
        }

        MarkActivity();

        InputEvent event;
        event.type  = InputEvent::Type::Button;
        event.id    = last_alt_button_id_;
        event.state = protocol::kInputReleased;
        DispatchEvent(event);

        last_alt_button_id_ = 0;
        return;
    }

    // -------------------------------------------------------------------------
    // Rotary frames
    // -------------------------------------------------------------------------
    if (!alt_rotary_position_set_) {
        alt_rotary_position_     = new_position;
        alt_rotary_position_set_ = true;

        ESP_LOGI(kTag, "ALT rotary initial position: %u", alt_rotary_position_);
        return;
    }

    int delta = static_cast<int>(new_position) - static_cast<int>(alt_rotary_position_);

    if (delta > 32768) {
        delta -= 65536;
    } else if (delta < -32768) {
        delta += 65536;
    }

    if (delta != 0) {
        if (config::kDebugKeys) {
            ESP_LOGI(kTag, "ALT rotary: pos=0x%04X delta=%d", new_position, delta);
        }

        MarkActivity();

        InputEvent event;
        event.type  = InputEvent::Type::Rotary;
        event.delta = delta;
        DispatchEvent(event);

        alt_rotary_position_ = new_position;
    }
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
        MarkActivity();
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

        MarkActivity();        
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

        // Reset 0x25B parser state.
        alt_rotary_position_      = 0;
        alt_rotary_position_set_  = false;
        alt_joystick_active_      = false;
        last_alt_button_id_       = 0;
        last_alt_joystick_id_     = 0;
        alt_menu_pressed_         = false;
        alt_back_pressed_         = false;
        option_held_              = false;
        option_long_hold_triggered_ = false;

        if (option_reset_timer_ != nullptr) {
            esp_timer_stop(option_reset_timer_);
        }        

        // Stay in the already-awake state; do not go back to cold-boot wake waiting.
        startup_commands_sent_    = true;        

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
    //  if (!hid_.IsConnected()) { - If initialization makes problem, this helps to skip init.
        return;
    }

    for (auto &handler : handlers_) {
        if (handler->Handle(event)) {
            break;  // Event was handled.
        }
    }
}

}  // namespace idrive
