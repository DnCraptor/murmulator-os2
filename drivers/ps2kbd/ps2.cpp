#include <pico.h>
#include "graphics.h"
#include "ps2.h"
#include "ps2kbd_mrmltr.h"
#include "sys_table.h"

extern "C" bool handleScancode(uint32_t ps2scancode);

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
//    gouta("[tud_msc_ejected] unsupported in HID build\n");
    return true;
}

extern "C" void __in_hfa() set_tud_msc_ejected(bool v) {
//    gouta("[set_tud_msc_ejected] unsupported in HID build\n");
}

extern "C" bool __in_hfa() set_usb_detached_handler(void* h) {
    return true;
}

extern "C" void  __in_hfa()pico_usb_drive_heartbeat() {
    //
}

extern "C" int16_t __in_hfa() keyboard_send(uint8_t data) {
//
    return 0;
}

void keyboard_toggle_led(uint8_t led) {
    led_status ^= led;
//    keyboard_send(0xED);
//    busy_wait_ms(50);
//    keyboard_send(led_status);
}

static void __not_in_flash_func(process_kbd_report)(
    hid_keyboard_report_t* report,
    hid_keyboard_report_t* prev_report
) {
    uint32_t retval = 0;
//    gouta("process_kbd_report\n");
    for (uint8_t pkc: prev_report->keycode) {
        if (!pkc) continue;
        bool key_still_pressed = false;
        for (uint8_t kc: report->keycode) {
            if (kc == pkc) {
                key_still_pressed = true;
                break;
            }
        }
        if (!key_still_pressed) {
            
            handleScancode(0x80 | pkc); /// TODO: map HID 2 sc
        //    kbdExtraMapping((fabgl::VirtualKey)pressed_key[pkc], false);
        //    pressed_key[pkc] = 0;
        }
    }
    for (uint8_t kc: report->keycode) {
        if (!kc) continue;
     //   uint8_t* pk = pressed_key + kc;
        handleScancode(kc); /// TODO: map HID 2 sc
     //   fabgl::VirtualKey vk = (fabgl::VirtualKey)*pk;
     //   if (vk == fabgl::VirtualKey::VK_NONE) { // it was not yet pressed
     //       vk = map_key(kc);
     //       if (vk != fabgl::VirtualKey::VK_NONE) {
     //           *pk = (uint8_t)vk;
     //           kbdExtraMapping(vk, true);
     //       }
     //   }
    }

    switch (retval) {
        case 0x45:
            keyboard_toggle_led(PS2_LED_NUM_LOCK);
            break;
        case 0x46:
            keyboard_toggle_led(PS2_LED_SCROLL_LOCK);
            break;
        case 0x3A:
            keyboard_toggle_led(PS2_LED_CAPS_LOCK);
            break;
    }
}

Ps2Kbd_Mrmltr ps2kbd(
    pio1,
    KBD_CLOCK_PIN,
    process_kbd_report
);

extern "C" void __in_hfa() keyboard_init(void) {
    ps2kbd.init_gpio();
}

extern "C" void __in_hfa() vHID(void *pv) {
    while(1) {
        ps2kbd.tick();
        vTaskDelay(1);
    }
}
