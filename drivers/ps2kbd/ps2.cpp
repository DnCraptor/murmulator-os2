#include <pico.h>
#include "graphics.h"
#include "ps2.h"
#include "ps2kbd_mrmltr.h"
#include "sys_table.h"

static uint8_t led_status = 0b000;

extern "C" uint8_t get_leds_stat() {
    return led_status;
}

extern "C" void __in_hfa() init_pico_usb_drive() {
    gouta("[init_pico_usb_drive] unsupported in HID build\n");
}

extern "C" void __in_hfa() usb_driver(bool on) {
    gouta("[usb_driver] unsupported in HID build\n");
}

extern "C" bool __in_hfa() tud_msc_ejected() {
    gouta("[tud_msc_ejected] unsupported in HID build\n");
}

extern "C" void __in_hfa() set_tud_msc_ejected(bool v) {
    gouta("[set_tud_msc_ejected] unsupported in HID build\n");
}

extern "C" bool __in_hfa() set_usb_detached_handler(void* h) {}

extern "C" void  __in_hfa()pico_usb_drive_heartbeat() {}

extern "C" int16_t __in_hfa() keyboard_send(uint8_t data) {
    /// TODO:
}

static void __not_in_flash_func(process_kbd_report)(
    hid_keyboard_report_t const* report,
    hid_keyboard_report_t const* prev_report
) {
    goutf("TT\n");
}

Ps2Kbd_Mrmltr ps2kbd(
    pio1,
    KBD_CLOCK_PIN,
    process_kbd_report
);

extern "C" void __in_hfa() keyboard_init(void) {
    ps2kbd.init_gpio();
}

extern "C" void __in_hfa() vKeyboardTicker(void* pv) {
    while(1) {
        ps2kbd.tick();
        vTaskDelay(1);
    }
}
