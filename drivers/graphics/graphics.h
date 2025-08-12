#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "stdbool.h"
#include "stdio.h"
#include "stdint.h"

#ifdef TFT
#include "st7789.h"
#endif
#ifdef HDMI
#include "hdmi.h"
#endif
#ifdef VGA
#include "vga.h"
#endif
#ifdef TV
#include "tv.h"
#endif
#ifdef SOFTTV
#include "tv-software.h"
#endif


#include "font6x8.h"
#include "font8x8.h"
#include "font8x16.h"

#include "ff.h"

enum graphics_mode_t {
    TEXTMODE_DEFAULT,
    GRAPHICSMODE_DEFAULT,

    TEXTMODE_53x30,

    TEXTMODE_160x100,

    CGA_160x200x16,
    CGA_320x200x4,
    CGA_640x200x2,

    TGA_320x200x16,
    EGA_320x200x16x4,
    VGA_320x240x256,
    VGA_320x200x256x4,
    // planar VGA
};

// Буффер текстового режима
extern uint8_t* text_buffer;

void graphics_init();

void graphics_set_mode(enum graphics_mode_t mode);

void graphics_set_buffer(uint8_t* buffer, uint16_t width, uint16_t height);

void graphics_set_offset(int x, int y);

void graphics_set_palette(uint8_t i, uint32_t color);

void graphics_set_textbuffer(uint8_t* buffer);

void graphics_set_bgcolor(uint32_t color888);

void graphics_set_flashmode(bool flash_line, bool flash_frame);

void draw_text(const char string[TEXTMODE_COLS + 1], uint32_t x, uint32_t y, uint8_t color, uint8_t bgcolor);
void draw_window(const char title[TEXTMODE_COLS + 1], uint32_t x, uint32_t y, uint32_t width, uint32_t height);

void clrScr(uint8_t color);


uint32_t get_screen_width();
uint32_t get_screen_height();
uint32_t get_console_width();
uint32_t get_console_height();
void graphics_set_con_color(uint8_t color, uint8_t bgcolor);
void goutf(const char *__restrict str, ...) _ATTRIBUTE ((__format__ (__printf__, 1, 2)));
void gouta(char* buf);
uint8_t* get_buffer();
uint8_t get_font_width(void);
uint8_t get_font_height(void);
bool is_buffer_text(void);
FIL* get_stdout();
uint8_t get_screen_bitness();
void fgoutf(FIL *f, const char *__restrict str, ...);
void graphics_set_con_pos(int x, int y);
void graphics_lock_buffer(bool);
void gbackspace();
void __putc(char);

#ifdef __cplusplus
}
#endif
