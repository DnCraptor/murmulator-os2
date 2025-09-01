/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <math.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"

#include "graphics.h"

#include <string.h>
#include <pico/multicore.h>

#include "st7789.h"
#include "font6x8.h"
#include "st7789.pio.h"
#include "hardware/dma.h"

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 320
#endif

#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 240
#endif

// 126MHz SPI
#define SERIAL_CLK_DIV 3.0f
#define MADCTL_BGR_PIXEL_ORDER (1<<3)
#define MADCTL_ROW_COLUMN_EXCHANGE (1<<5)
#define MADCTL_COLUMN_ADDRESS_ORDER_SWAP (1<<6)


#define CHECK_BIT(var, pos) (((var)>>(pos)) & 1)

#ifndef TFT_RST_PIN
#define TFT_RST_PIN 8
#endif


#ifndef TFT_CS_PIN
#define TFT_CS_PIN 6
#endif


#ifndef TFT_LED_PIN
#define TFT_LED_PIN 9
#endif


#ifndef TFT_CLK_PIN
#define TFT_CLK_PIN 13
#endif 

#ifndef TFT_DATA_PIN
#define TFT_DATA_PIN 12
#endif

#ifndef TFT_DC_PIN
#define TFT_DC_PIN 10
#endif

#define TEXTMODE_COLS 53
#define TEXTMODE_ROWS 30

#define RGB888(r, g, b) ((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3))
static const uint16_t textmode_palette[16] = {
    //R, G, B
    RGB888(0x00,0x00, 0x00), //black
    RGB888(0x00,0x00, 0xC4), //blue
    RGB888(0x00,0xC4, 0x00), //green
    RGB888(0x00,0xC4, 0xC4), //cyan
    RGB888(0xC4,0x00, 0x00), //red
    RGB888(0xC4,0x00, 0xC4), //magenta
    RGB888(0xC4,0x7E, 0x00), //brown
    RGB888(0xC4,0xC4, 0xC4), //light gray
    RGB888(0x4E,0x4E, 0x4E), //dark gray
    RGB888(0x4E,0x4E, 0xDC), //light blue
    RGB888(0x4E,0xDC, 0x4E), //light green
    RGB888(0x4E,0xF3, 0xF3), //light cyan
    RGB888(0xDC,0x4E, 0x4E), //light red
    RGB888(0xF3,0x4E, 0xF3), //light magenta
    RGB888(0xF3,0xF3, 0x4E), //yellow
    RGB888(0xFF,0xFF, 0xFF), //white
};

static uint sm = 0;
static PIO pio = pio0;
static uint st7789_chan;

uint16_t palette[256];

uint8_t* text_buffer = NULL;
static uint8_t* graphics_buffer = NULL;

static uint graphics_buffer_width = 320;
static uint graphics_buffer_height = 240;
static int graphics_buffer_shift_x = 0;
static int graphics_buffer_shift_y = 0;

int graphics_mode = 0;

static const uint8_t init_seq[] = {
    1, 20, 0x01, // Software reset
    1, 10, 0x11, // Exit sleep mode
    2, 2, 0x3a, 0x55, // Set colour mode to 16 bit
#ifdef ILI9341
    // ILI9341
    2, 0, 0x36, MADCTL_ROW_COLUMN_EXCHANGE | MADCTL_BGR_PIXEL_ORDER, // Set MADCTL
#else
    // ST7789
    2, 0, 0x36, MADCTL_COLUMN_ADDRESS_ORDER_SWAP | MADCTL_ROW_COLUMN_EXCHANGE, // Set MADCTL
#endif
    5, 0, 0x2a, 0x00, 0x00, SCREEN_WIDTH >> 8, SCREEN_WIDTH & 0xff, // CASET: column addresses
    5, 0, 0x2b, 0x00, 0x00, SCREEN_HEIGHT >> 8, SCREEN_HEIGHT & 0xff, // RASET: row addresses
    1, 2, 0x20, // Inversion OFF
    1, 2, 0x13, // Normal display on, then 10 ms delay
    1, 2, 0x29, // Main screen turn on, then wait 500 ms
    0 // Terminate list
};
// Format: cmd length (including cmd byte), post delay in units of 5 ms, then cmd payload
// Note the delays have been shortened a little

static inline void lcd_set_dc_cs(const bool dc, const bool cs) {
    sleep_us(5);
    gpio_put_masked((1u << TFT_DC_PIN) | (1u << TFT_CS_PIN), !!dc << TFT_DC_PIN | !!cs << TFT_CS_PIN);
    sleep_us(5);
}

static inline void lcd_write_cmd(const uint8_t* cmd, size_t count) {
    st7789_lcd_wait_idle(pio, sm);
    lcd_set_dc_cs(0, 0);
    st7789_lcd_put(pio, sm, *cmd++);
    if (count >= 2) {
        st7789_lcd_wait_idle(pio, sm);
        lcd_set_dc_cs(1, 0);
        for (size_t i = 0; i < count - 1; ++i)
            st7789_lcd_put(pio, sm, *cmd++);
    }
    st7789_lcd_wait_idle(pio, sm);
    lcd_set_dc_cs(1, 1);
}

static inline void lcd_set_window(const uint16_t x,
                                  const uint16_t y,
                                  const uint16_t width,
                                  const uint16_t height) {
    static uint8_t screen_width_cmd[] = { 0x2a, 0x00, 0x00, SCREEN_WIDTH >> 8, SCREEN_WIDTH & 0xff };
    static uint8_t screen_height_command[] = { 0x2b, 0x00, 0x00, SCREEN_HEIGHT >> 8, SCREEN_HEIGHT & 0xff };
    screen_width_cmd[2] = x;
    screen_width_cmd[4] = x + width - 1;

    screen_height_command[2] = y;
    screen_height_command[4] = y + height - 1;
    lcd_write_cmd(screen_width_cmd, 5);
    lcd_write_cmd(screen_height_command, 5);
}

static inline void lcd_init(const uint8_t* init_seq) {
    const uint8_t* cmd = init_seq;
    while (*cmd) {
        lcd_write_cmd(cmd + 2, *cmd);
        sleep_ms(*(cmd + 1) * 5);
        cmd += *cmd + 2;
    }
}

static inline void start_pixels() {
    const uint8_t cmd = 0x2c; // RAMWR
    st7789_lcd_wait_idle(pio, sm);
    st7789_set_pixel_mode(pio, sm, false);
    lcd_write_cmd(&cmd, 1);
    st7789_set_pixel_mode(pio, sm, true);
    lcd_set_dc_cs(1, 0);
}

void stop_pixels() {
    st7789_lcd_wait_idle(pio, sm);
    lcd_set_dc_cs(1, 1);
    st7789_set_pixel_mode(pio, sm, false);
}

uint offset;
void tft_graphics_init() {
    offset = pio_add_program(pio, &st7789_lcd_program);
    sm = pio_claim_unused_sm(pio, true);
    st7789_lcd_program_init(pio, sm, offset, TFT_DATA_PIN, TFT_CLK_PIN, SERIAL_CLK_DIV);

    gpio_init(TFT_CS_PIN);
    gpio_init(TFT_DC_PIN);
    gpio_init(TFT_RST_PIN);
    gpio_init(TFT_LED_PIN);
    gpio_set_dir(TFT_CS_PIN, GPIO_OUT);
    gpio_set_dir(TFT_DC_PIN, GPIO_OUT);
    gpio_set_dir(TFT_RST_PIN, GPIO_OUT);
    gpio_set_dir(TFT_LED_PIN, GPIO_OUT);

    gpio_put(TFT_CS_PIN, 1);
    gpio_put(TFT_RST_PIN, 1);
    lcd_init(init_seq);
    gpio_put(TFT_LED_PIN, 1);

    for (int i = 0; i < sizeof palette; i++) {
        /// TODO:
        graphics_set_palette(i, 0x0000);
    }

    st7789_chan = dma_claim_unused_channel(true);

    dma_channel_config c = dma_channel_get_default_config(st7789_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, true));
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);

    dma_channel_configure(
        st7789_chan, // Channel to be configured
        &c, // The configuration we just created
        &pio->txf[sm], // The write address
        NULL, // The initial read address - set later
        0, // Number of transfers - set later
        false // Don't start yet
    );

    tft_graphics_set_offset(0, 0);
    tft_graphics_set_mode(0);
    tft_clrScr(0);
}

void tft_graphics_set_mode(int mode) {
    if (mode > 1) mode = 1;
    graphics_mode = -1;
    sleep_ms(16);
    tft_clrScr(0);
    graphics_mode = mode;
}

void tft_graphics_set_buffer(uint8_t* buffer, const uint16_t width, const uint16_t height) {
    graphics_buffer = buffer;
    graphics_buffer_width = width;
    graphics_buffer_height = height;
}

void graphics_set_textbuffer(uint8_t* buffer) {
    text_buffer = buffer;
}

void tft_graphics_set_offset(const int x, const int y) {
    graphics_buffer_shift_x = x;
    graphics_buffer_shift_y = y;
}

void tft_clrScr(const uint8_t color) {
    if (!graphics_buffer) return;
    memset(graphics_buffer, color, graphics_buffer_height * graphics_buffer_width);
    // TODO: bgcolor
}

void st7789_dma_pixels(const uint16_t* pixels, const uint num_pixels) {
    // Ensure any previous transfer is finished.
    dma_channel_wait_for_finish_blocking(st7789_chan);

    dma_channel_hw_addr(st7789_chan)->read_addr = (uintptr_t)pixels;
    dma_channel_hw_addr(st7789_chan)->transfer_count = num_pixels;
    const uint ctrl = dma_channel_hw_addr(st7789_chan)->ctrl_trig;
    dma_channel_hw_addr(st7789_chan)->ctrl_trig = ctrl | DMA_CH0_CTRL_TRIG_INCR_READ_BITS;
}

void __not_in_flash() refresh_lcd() {
    if(!graphics_buffer) return;
    switch (graphics_mode) {
        case 0:
            lcd_set_window(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            start_pixels();
            for (int y = 0; y < SCREEN_HEIGHT; y++) {
                // TODO add auto adjustable padding?
                st7789_lcd_put_pixel(pio, sm, 0x0000);

                for (int x = 0; x < TEXTMODE_COLS; x++) {
                    const uint16_t offset = (y / 8) * (TEXTMODE_COLS * 2) + x * 2;
                    const uint8_t c = text_buffer[offset];
                    const uint8_t colorIndex = text_buffer[offset + 1];
                    const uint8_t glyph_row = font_6x8[c * 8 + y % 8];

                    for (uint8_t bit = 0; bit < 6; bit++) {
                        st7789_lcd_put_pixel(pio, sm, textmode_palette[(c && CHECK_BIT(glyph_row, bit))
                                                                           ? colorIndex & 0x0F
                                                                           : colorIndex >> 4 & 0x0F]);
                    }
                }
                st7789_lcd_put_pixel(pio, sm, 0x0000);
            }
            stop_pixels();
            break;
        case 1: {
            const uint8_t* bitmap = graphics_buffer;
            lcd_set_window(graphics_buffer_shift_x, graphics_buffer_shift_y, graphics_buffer_width,
                           graphics_buffer_height);
            uint32_t i = graphics_buffer_width * graphics_buffer_height;
            start_pixels();
            // st7789_dma_pixels(graphics_buffer, i);
            while (--i) {
               st7789_lcd_put_pixel(pio, sm, palette[*bitmap++]);
            }
            stop_pixels();
        }
    }
    // st7789_lcd_wait_idle(pio, sm);
}

void graphics_set_palette(const uint8_t i, const uint32_t color) {
    palette[i] = (uint16_t)color;
}

void tft_set_bgcolor(uint32_t color888)
{
    graphics_set_palette(255, color888);
}

void tft_cleanup(void) {
    graphics_mode = -1;
    if (dma_channel_is_claimed(st7789_chan)) {
        dma_channel_abort(st7789_chan);
        dma_channel_unclaim(st7789_chan);
    }
    if (pio_sm_is_claimed(pio, sm)) {
        pio_sm_set_enabled(pio, sm, false);
        pio_sm_unclaim(pio, sm);
    }
    pio_remove_program(pio, &st7789_lcd_program, offset);
    gpio_deinit(TFT_CS_PIN);
    gpio_deinit(TFT_DC_PIN);
    gpio_deinit(TFT_RST_PIN);
    gpio_deinit(TFT_LED_PIN);
//    for (int i = 0; i < sizeof palette; i++) {
//        graphics_set_palette(i, 0x0000);
//    }
    sm = -1;
    st7789_chan = -1;
    if (graphics_buffer) {
        vPortFree(graphics_buffer);
        graphics_buffer = 0;
    }
}

bool tft_is_mode_text(int mode) {
    return mode == 0;
}

bool tft_is_text_mode() {
    return hdmi_is_mode_text(graphics_mode);
}

uint32_t tft_console_width(void) {
    if (graphics_mode == 1) {
        return graphics_buffer_width / 6;
    }
    return graphics_buffer_width;
}
uint32_t tft_console_height(void) {
    if (graphics_mode == 1) {
        return graphics_buffer_height / 8;
    }
    return graphics_buffer_height;
}
uint32_t tft_screen_width(void) {
    return graphics_buffer_width;
}
uint32_t tft_screen_height(void) {
    return graphics_buffer_height;
}
uint8_t* get_tft_buffer(void) {
    return graphics_buffer;
}
void set_tft_buffer(uint8_t* b) {
    graphics_buffer = b;
}

uint8_t get_tft_buffer_bitness(void) {
    return graphics_mode == 1 ? 8 : 16;
}

size_t tft_buffer_size() {
    if (!graphics_buffer) return 0;
    return 320*240;
}

void tft_lock_buffer(bool b) {
///    lock_buffer = b;
}

void tft_set_cursor_color(uint8_t color) {
///    _cursor_color = color;
}

int tft_get_mode(void) {
    return graphics_mode;
}

int tft_get_default_mode(void) {
    return 0;
}
