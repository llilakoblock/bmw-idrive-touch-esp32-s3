idf_component_register(
    SRCS
        "main.cpp"
        "can/can_bus.cpp"
        "hid/usb_hid_device.cpp"
        "idrive/idrive_controller.cpp"
        "input/button_handler.cpp"
        "input/joystick_handler.cpp"
        "input/rotary_handler.cpp"
        "input/touchpad_handler.cpp"
        "utils/utils.cpp"
    INCLUDE_DIRS "../include"
    REQUIRES driver freertos esp_timer
)
