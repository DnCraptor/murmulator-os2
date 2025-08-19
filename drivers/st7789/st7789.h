#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "gmodes.h"
#include <stdbool.h>
#include <stdint.h>

void tft_driver_init();
void tft_refresh(void*);
void tft_graphics_set_mode(enum graphics_mode_t mode);
bool tft_is_mode_text(int mode);
bool tft_is_text_mode();
uint32_t tft_console_width(void);
uint32_t tft_console_height(void);
uint32_t tft_screen_width(void);
uint32_t tft_screen_height(void);
uint8_t* get_tft_buffer(void);
void set_tft_buffer(uint8_t* b);
uint8_t get_tft_buffer_bitness(void);
size_t tft_buffer_size();
void tft_lock_buffer(bool b);
void tft_clr_scr(const uint8_t color);
void tft_set_bgcolor(uint32_t color888);
int tft_get_mode(void);
void tft_set_cursor_color(uint8_t color);
int tft_get_default_mode(void);
void tft_graphics_set_offset(const int x, const int y);

#ifdef __cplusplus
}
#endif
