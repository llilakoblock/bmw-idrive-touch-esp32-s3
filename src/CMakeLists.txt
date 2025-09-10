idf_component_register(
    SRCS "main.cpp" "idrive.cpp" "variables.cpp" "usb_hid_device.c"
    INCLUDE_DIRS "../include"
    REQUIRES driver freertos esp_timer
)