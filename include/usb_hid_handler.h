#ifndef USBHIDHANDLER_H
#define USBHIDHANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

void init_usb_hid();
void usb_hid_task(void);
void usb_hid_move_mouse(int8_t dx, int8_t dy, uint8_t buttons);
void usb_hid_type_text(const char* text);

#ifdef __cplusplus
}
#endif

#endif
