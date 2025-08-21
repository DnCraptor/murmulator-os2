#pragma once

void tft_graphics_set_buffer(uint8_t* buffer, const uint16_t width, const uint16_t height);
void tft_graphics_init();
void refresh_lcd();
void tft_cleanup(void);
void tft_graphics_set_mode(int mode);
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
void tft_set_cursor_color(uint8_t color);
void tft_clrScr(const uint8_t color);
void tft_graphics_set_offset(const int x, const int y);
void tft_set_bgcolor(uint32_t color888);
int tft_get_mode(void);
int tft_get_default_mode(void);