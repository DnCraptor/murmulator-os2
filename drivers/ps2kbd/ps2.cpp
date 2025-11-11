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

static const uint16_t hid2scancode[256] = {
    0, // 0 HID_KEY_NONE
    0, // 1
    0, // 2
    0, // 3
    0x1E, // 4 HID_KEY_A
    0x30, // 5 HID_KEY_B
    0x2E, // 6 HID_KEY_C
    0x20, // 7 HID_KEY_D
    0x12, // 8 HID_KEY_E
    0x21, // 9 HID_KEY_F
    0x22, // A HID_KEY_G
    0x23, // B HID_KEY_H
    0x17, // C HID_KEY_I
    0x24, // D HID_KEY_J
    0x25, // E HID_KEY_K
    0x26, // F HID_KEY_L
    0x32, // 10 HID_KEY_M
    0x31, // 11 HID_KEY_N
    0x18, // 12 HID_KEY_O
    0x19, // 13 HID_KEY_P
    0x10,
    0x13,
    0x1F,
    0x14,
    0x16,
    0x2F,
    0x11,
    0x2D,
    0x15,
    0x2C, // 1D HID_KEY_Z
    2, // 1
    3, // 2
    4,
    5,
    6,
    7,
    8,
    9,
    10,
    11,   // HID_KEY_0                         0x27
    0x1C, // HID_KEY_ENTER                     0x28
    1,    // HID_KEY_ESCAPE                    0x29
    0x0E, // HID_KEY_BACKSPACE                 0x2A
    0x0F, // HID_KEY_TAB                       0x2B
    0x39, // HID_KEY_SPACE                     0x2C
    0x0C, // HID_KEY_MINUS                     0x2D
    0x0D, // HID_KEY_EQUAL                     0x2E
    0x1A, // HID_KEY_BRACKET_LEFT              0x2F
    0x1B, // HID_KEY_BRACKET_RIGHT             0x30
    0x2B, // HID_KEY_BACKSLASH                 0x31
    0,    // HID_KEY_EUROPE_1                  0x32
    0x27, // HID_KEY_SEMICOLON                 0x33
    0x28, // HID_KEY_APOSTROPHE                0x34
    0x29, // HID_KEY_GRAVE                     0x35
    0x33, // HID_KEY_COMMA                     0x36
    0x34, // HID_KEY_PERIOD                    0x37
    0x35, // HID_KEY_SLASH                     0x38
    0x3A, // HID_KEY_CAPS_LOCK                 0x39
    0x3B, // HID_KEY_F1                        0x3A
    0x3C,
    0x3D,
    0x3E,
    0x3F,
    0x40,
    0x41,
    0x42,
    0x43,
    0x44,
    0x57,
    0x58,   // HID_KEY_F12                       0x45
    0xE037, // HID_KEY_PRINT_SCREEN              0x46 , TODO: ensure E0,2A,E0,37
    0x46,   // HID_KEY_SCROLL_LOCK               0x47
    0x9DC5, // HID_KEY_PAUSE                     0x48 , TODO: ensure E1,1D,45, E1,9D,C5
    0xE052, // HID_KEY_INSERT                    0x49
    0xE047, // HID_KEY_HOME                      0x4A
    0xE049, // HID_KEY_PAGE_UP                   0x4B
    0xE053, // HID_KEY_DELETE                    0x4C
    0xE04F, // HID_KEY_END                       0x4D
    0xE051, // HID_KEY_PAGE_DOWN                 0x4E
    0xE04D, // HID_KEY_ARROW_RIGHT               0x4F
    0xE04B, // HID_KEY_ARROW_LEFT                0x50
    0xE050, // HID_KEY_ARROW_DOWN                0x51
    0xE048, // HID_KEY_ARROW_UP                  0x52
    0x45, // HID_KEY_NUM_LOCK                  0x53
    0xE035, // HID_KEY_KEYPAD_DIVIDE             0x54
    0x0037, // HID_KEY_KEYPAD_MULTIPLY           0x55
    0x004A, // HID_KEY_KEYPAD_SUBTRACT           0x56
    0x004E, // HID_KEY_KEYPAD_ADD                0x57
    0xE01C, // HID_KEY_KEYPAD_ENTER              0x58
    0x4F, // HID_KEY_KEYPAD_1                  0x59
    0x50, // HID_KEY_KEYPAD_2                  0x5A
    0x51, // HID_KEY_KEYPAD_3                  0x5B
    0x4B,
    0x4C,
    0x4D,
    0x47,
    0x48,
    0x49,
    0x52, // HID_KEY_KEYPAD_0                  0x62
    0x53, // HID_KEY_KEYPAD_DECIMAL            0x63
    0, // HID_KEY_EUROPE_2                  0x64
    0xE05D, // HID_KEY_APPLICATION               0x65 (Menu)
    0, // HID_KEY_POWER                     0x66
    0, // HID_KEY_KEYPAD_EQUAL              0x67
};
/*
#define HID_KEY_F13                       0x68
#define HID_KEY_F14                       0x69
#define HID_KEY_F15                       0x6A
#define HID_KEY_F16                       0x6B
#define HID_KEY_F17                       0x6C
#define HID_KEY_F18                       0x6D
#define HID_KEY_F19                       0x6E
#define HID_KEY_F20                       0x6F
#define HID_KEY_F21                       0x70
#define HID_KEY_F22                       0x71
#define HID_KEY_F23                       0x72
#define HID_KEY_F24                       0x73
#define HID_KEY_EXECUTE                   0x74
#define HID_KEY_HELP                      0x75
#define HID_KEY_MENU                      0x76
#define HID_KEY_SELECT                    0x77
#define HID_KEY_STOP                      0x78
#define HID_KEY_AGAIN                     0x79
#define HID_KEY_UNDO                      0x7A
#define HID_KEY_CUT                       0x7B
#define HID_KEY_COPY                      0x7C
#define HID_KEY_PASTE                     0x7D
#define HID_KEY_FIND                      0x7E
#define HID_KEY_MUTE                      0x7F
#define HID_KEY_VOLUME_UP                 0x80
#define HID_KEY_VOLUME_DOWN               0x81
#define HID_KEY_LOCKING_CAPS_LOCK         0x82
#define HID_KEY_LOCKING_NUM_LOCK          0x83
#define HID_KEY_LOCKING_SCROLL_LOCK       0x84
#define HID_KEY_KEYPAD_COMMA              0x85
#define HID_KEY_KEYPAD_EQUAL_SIGN         0x86
#define HID_KEY_KANJI1                    0x87
#define HID_KEY_KANJI2                    0x88
#define HID_KEY_KANJI3                    0x89
#define HID_KEY_KANJI4                    0x8A
#define HID_KEY_KANJI5                    0x8B
#define HID_KEY_KANJI6                    0x8C
#define HID_KEY_KANJI7                    0x8D
#define HID_KEY_KANJI8                    0x8E
#define HID_KEY_KANJI9                    0x8F
#define HID_KEY_LANG1                     0x90
#define HID_KEY_LANG2                     0x91
#define HID_KEY_LANG3                     0x92
#define HID_KEY_LANG4                     0x93
#define HID_KEY_LANG5                     0x94
#define HID_KEY_LANG6                     0x95
#define HID_KEY_LANG7                     0x96
#define HID_KEY_LANG8                     0x97
#define HID_KEY_LANG9                     0x98
#define HID_KEY_ALTERNATE_ERASE           0x99
#define HID_KEY_SYSREQ_ATTENTION          0x9A
#define HID_KEY_CANCEL                    0x9B
#define HID_KEY_CLEAR                     0x9C
#define HID_KEY_PRIOR                     0x9D
#define HID_KEY_RETURN                    0x9E
#define HID_KEY_SEPARATOR                 0x9F
#define HID_KEY_OUT                       0xA0
#define HID_KEY_OPER                      0xA1
#define HID_KEY_CLEAR_AGAIN               0xA2
#define HID_KEY_CRSEL_PROPS               0xA3
#define HID_KEY_EXSEL                     0xA4
// RESERVED					                      0xA5-DF
#define HID_KEY_CONTROL_LEFT              0xE0
#define HID_KEY_SHIFT_LEFT                0xE1
#define HID_KEY_ALT_LEFT                  0xE2
#define HID_KEY_GUI_LEFT                  0xE3
#define HID_KEY_CONTROL_RIGHT             0xE4
#define HID_KEY_SHIFT_RIGHT               0xE5
#define HID_KEY_ALT_RIGHT                 0xE6
#define HID_KEY_GUI_RIGHT                 0xE7
*/

typedef struct mod2key_s {
    hid_keyboard_modifier_bm_t mod;
    uint16_t scancode;
} mod2key_t;

static const mod2key_t mod2key[] = {
    { KEYBOARD_MODIFIER_LEFTALT,    0x38 },
    { KEYBOARD_MODIFIER_RIGHTALT,   0xE038 },
    { KEYBOARD_MODIFIER_LEFTCTRL,   0x1D },
    { KEYBOARD_MODIFIER_RIGHTCTRL,  0xE01D },
    { KEYBOARD_MODIFIER_RIGHTSHIFT, 0x36 },
    { KEYBOARD_MODIFIER_LEFTSHIFT,  0x2A },
    { KEYBOARD_MODIFIER_RIGHTGUI,   0xE05C },
    { KEYBOARD_MODIFIER_LEFTGUI,    0xE05B }
};

static void __not_in_flash_func(process_kbd_report)(
    hid_keyboard_report_t* report,
    hid_keyboard_report_t* prev_report
) {
    uint8_t new_modifiers = report->modifier & ~(prev_report->modifier);
    uint8_t old_modifiers = prev_report->modifier & ~(report->modifier);
    for (int i = 0; i < sizeof(mod2key) / sizeof(mod2key[0]); ++i) {
        if (old_modifiers & mod2key[i].mod) {
            handleScancode(mod2key[i].scancode | 0x0080);
        }
        if (new_modifiers & mod2key[i].mod) {
            handleScancode(mod2key[i].scancode);
        }
    }
    uint32_t retval = 0;
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
            retval = hid2scancode[pkc];
            handleScancode(0x80 | retval);
        }
    }
    for (uint8_t kc: report->keycode) {
        if (!kc) continue;
        retval = hid2scancode[kc];
        handleScancode(retval);
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
