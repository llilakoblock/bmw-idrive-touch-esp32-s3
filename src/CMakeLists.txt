idf_component_register(
    SRCS
        "main.cpp"
        "can/can_bus.cpp"
        "can/can_task.cpp"
        "hid/usb_hid_device.cpp"
        "idrive/idrive_controller.cpp"
        "input/button_handler.cpp"
        "input/joystick_handler.cpp"
        "input/rotary_handler.cpp"
        "input/touchpad_handler.cpp"
        "utils/utils.cpp"
        # OTA module
        "ota/ota_manager.cpp"
        "ota/ota_trigger.cpp"
        "ota/wifi_ap.cpp"
        "ota/web_server.cpp"
    INCLUDE_DIRS "../include"
    REQUIRES driver freertos esp_timer esp_wifi esp_http_server app_update nvs_flash esp_netif
)
