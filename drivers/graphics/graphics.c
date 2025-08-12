#include "graphics.h"
#include <string.h>
#include <stdarg.h>

#include "app.h"

volatile uint8_t con_color = 7;
volatile uint8_t con_bgcolor = 0;
volatile int pos_x = 0;
volatile int pos_y = 0;
volatile uint8_t font_width = 8;
volatile uint8_t font_height = 16;
volatile uint8_t bitness = 16; // TODO:
volatile uint8_t* font_table = font_8x16;
static volatile bool lock_buffer = false;

void graphics_lock_buffer(bool v) {
    // TODO:
    lock_buffer = v;
}

void gbackspace() {
    if (!text_buffer) return;
    uint8_t* t_buf;
    pos_x--;
    if (pos_x < 0) {
        pos_x = TEXTMODE_COLS - 2;
        pos_y--;
        if (pos_y < 0) {
            pos_y = 0;
        }
    }
    t_buf = text_buffer + TEXTMODE_COLS * 2 * pos_y + 2 * pos_x;
    *t_buf++ = ' ';
    *t_buf++ = con_bgcolor << 4 | con_color & 0xF;
}

void draw_text(const char string[TEXTMODE_COLS + 1], uint32_t x, uint32_t y, uint8_t color, uint8_t bgcolor) {
    uint8_t* t_buf = text_buffer + TEXTMODE_COLS * 2 * y + 2 * x;
    for (int xi = TEXTMODE_COLS * 2; xi--;) {
        if (!*string) break;
        *t_buf++ = *string++;
        *t_buf++ = bgcolor << 4 | color & 0xF;
    }
}

void draw_window(const char title[TEXTMODE_COLS + 1], uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    char line[width + 1];
    memset(line, 0, sizeof line);
    width--;
    height--;
    // Рисуем рамки

    memset(line, 0xCD, width); // ═══


    line[0] = 0xC9; // ╔
    line[width] = 0xBB; // ╗
    draw_text(line, x, y, 11, 1);

    line[0] = 0xC8; // ╚
    line[width] = 0xBC; //  ╝
    draw_text(line, x, height + y, 11, 1);

    memset(line, ' ', width);
    line[0] = line[width] = 0xBA;

    for (int i = 1; i < height; i++) {
        draw_text(line, x, y + i, 11, 1);
    }

    snprintf(line, width - 1, " %s ", title);
    draw_text(line, x + (width - strlen(line)) / 2, y, 14, 3);
}

void graphics_set_con_color(uint8_t color, uint8_t bgcolor) {
    con_color = color;
    con_bgcolor = bgcolor;
}

void goutf(const char *__restrict str, ...) {
    FIL* f = get_stdout();
    va_list ap;
//#if DEBUG_HEAP_SIZE
    char buf[256];
//#else
//    char* buf = (char*)pvPortMalloc(512);
//#endif
    va_start(ap, str);
    vsnprintf(buf, 256, str, ap); // TODO: optimise (skip)
    va_end(ap);
    if (!f) {
        gouta(buf);
    } else {
        UINT bw;
        f_write(f, buf, strlen(buf), &bw); // TODO: error handling
    }
//#if !DEBUG_HEAP_SIZE
//    vPortFree(buf);
//#endif
}
static void graphics_rollup(uint8_t* graphics_buffer, uint32_t width) {
    uint32_t height = get_screen_height();
    uint8_t bit = get_screen_bitness();
    uint32_t h = height / font_height;
    if (pos_y >= h - 1) {
        uint32_t sz = (width * (height - 2 * font_height) * bit) >> 3;
        memmove(graphics_buffer, graphics_buffer + (width * bit * font_height >> 3), sz);
        memset(graphics_buffer + sz, 0, width * bit * font_height >> 3);
        pos_y = h - 2;
    }
}
static char* common_rollup(char* b, char* t_buf, uint32_t width, uint32_t height) {
    if (pos_y >= height - 1) {
        memmove(b, b + width * 2, width * (height - 2) * 2);
        t_buf = b + width * (height - 2) * 2;
        for(int i = 0; i < width; ++i) {
            *t_buf++ = ' ';
            *t_buf++ = con_bgcolor << 4 | con_color & 0xF;
        }
        pos_y = height - 2;
    }
    return b + width * 2 * pos_y + 2 * pos_x;
}
static void common_print_char(uint8_t* graphics_buffer, uint32_t width, uint32_t height, uint32_t x, uint32_t y, uint8_t color, uint8_t bgcolor, uint16_t c) {
    uint8_t bit = get_screen_bitness();
    uint8_t* pE = graphics_buffer + ((width * height * bit) >> 3);
    if (bit == 8) {
        uint8_t* p0 = graphics_buffer + width * y * font_height + x * font_width;
        uint8_t b = (color & 1) ? ((color >> 3) ? 3 : 2) : 0;
        uint8_t r = (color & 4) ? ((color >> 3) ? 3 : 2) : 0;
        uint8_t g = (color & 2) ? ((color >> 3) ? 3 : 2) : 0;
        uint8_t cf = (((r << 4) | (g << 2) | b) & 0x3f) | 0xc0;
        b = (bgcolor & 1) ? ((bgcolor >> 3) ? 3 : 2) : 0;
        r = (bgcolor & 4) ? ((bgcolor >> 3) ? 3 : 2) : 0;
        g = (bgcolor & 2) ? ((bgcolor >> 3) ? 3 : 2) : 0;
        uint8_t cb = (((r << 4) | (g << 2) | b) & 0x3f) | 0xc0;
        if (font_width == 6) {
            for (int glyph_line = 0; glyph_line < font_height; ++glyph_line) {
                uint8_t* p = p0 + width * glyph_line;
                if (p >= pE) return;
                uint8_t glyph_pixels = font_table[c * font_height + glyph_line];
                *p++ =  (glyph_pixels & 1)       ? cf : cb; if (p >= pE) return;
                *p++ = ((glyph_pixels >> 1) & 1) ? cf : cb; if (p >= pE) return;
                *p++ = ((glyph_pixels >> 2) & 1) ? cf : cb; if (p >= pE) return;
                *p++ = ((glyph_pixels >> 3) & 1) ? cf : cb; if (p >= pE) return;
                *p++ = ((glyph_pixels >> 4) & 1) ? cf : cb; if (p >= pE) return;
                *p   = ((glyph_pixels >> 5) & 1) ? cf : cb;
            }
            return;
        }
        for (int glyph_line = 0; glyph_line < font_height; ++glyph_line) {
            uint8_t* p = p0 + width * glyph_line;
            if (p >= pE) return;
            uint8_t glyph_pixels = font_table[c * font_height + glyph_line];
            *p++ =  (glyph_pixels & 1)       ? cf : cb; if (p >= pE) return;
            *p++ = ((glyph_pixels >> 1) & 1) ? cf : cb; if (p >= pE) return;
            *p++ = ((glyph_pixels >> 2) & 1) ? cf : cb; if (p >= pE) return;
            *p++ = ((glyph_pixels >> 3) & 1) ? cf : cb; if (p >= pE) return;
            *p++ = ((glyph_pixels >> 4) & 1) ? cf : cb; if (p >= pE) return;
            *p++ = ((glyph_pixels >> 5) & 1) ? cf : cb; if (p >= pE) return;
            *p++ = ((glyph_pixels >> 6) & 1) ? cf : cb; if (p >= pE) return;
            *p   =  (glyph_pixels >> 7)      ? cf : cb;
        }
        return;
    }
    if (bit == 4) {
        uint8_t* p0 = graphics_buffer + ((width * y * font_height) >> 1) + ((x * font_width) >> 1);
        uint8_t cf = color & 0x0F;
        uint8_t cb = bgcolor & 0x0F;
        if (font_width == 6) {
            for (int glyph_line = 0; glyph_line < font_height; ++glyph_line) {
                uint8_t* p = p0 + (width * glyph_line >> 1);
                if (p >= pE) return;
                uint8_t glyph_pixels = font_table[c * font_height + glyph_line];
                *p++ = (  (glyph_pixels & 1)       ? cf : cb ) | (( ((glyph_pixels >> 1) & 1) ? cf : cb ) << 4); if (p >= pE) return;
                *p++ = ( ((glyph_pixels >> 2) & 1) ? cf : cb ) | (( ((glyph_pixels >> 3) & 1) ? cf : cb ) << 4); if (p >= pE) return;
                *p   = ( ((glyph_pixels >> 4) & 1) ? cf : cb ) | (( ((glyph_pixels >> 5) & 1) ? cf : cb ) << 4);
            }
            return;
        }
        for (int glyph_line = 0; glyph_line < font_height; ++glyph_line) {
            uint8_t* p = p0 + (width * glyph_line >> 1);
            if (p >= pE) return;
            uint8_t glyph_pixels = font_table[c * font_height + glyph_line];
            *p++ = (  (glyph_pixels & 1)       ? cf : cb ) | (( ((glyph_pixels >> 1) & 1) ? cf : cb ) << 4); if (p >= pE) return;
            *p++ = ( ((glyph_pixels >> 2) & 1) ? cf : cb ) | (( ((glyph_pixels >> 3) & 1) ? cf : cb ) << 4); if (p >= pE) return;
            *p++ = ( ((glyph_pixels >> 4) & 1) ? cf : cb ) | (( ((glyph_pixels >> 5) & 1) ? cf : cb ) << 4); if (p >= pE) return;
            *p   = ( ((glyph_pixels >> 6) & 1) ? cf : cb ) | ((  (glyph_pixels >> 7)      ? cf : cb ) << 4);
        }
        return;
    }
    if (bit == 2) {
        uint8_t* p0 = graphics_buffer + ((width * y * font_height + x * font_width) >> 2);
//        uint8_t* pE = graphics_buffer + ((width * height) >> 2);
        uint8_t cf = color; // & 3; // TODO: mapping
        uint8_t cb = bgcolor; // & 3;
        if (font_width == 6) {
            for (int glyph_line = 0; glyph_line < font_height; ++glyph_line) {
                uint8_t* p = p0 + ((width * glyph_line) >> 2);
//                if (p >= pE) return;
                uint8_t glyph_pixels = font_table[c * font_height + glyph_line];
                *p = cb > cf ? ~glyph_pixels : glyph_pixels;
                if (p >= pE) return;
/*                uint8_t glyph_pixels = font_table[c * font_height + glyph_line];
                *p++ = (  (glyph_pixels & 1)       ? cf : cb ) | (( ((glyph_pixels >> 1) & 1) ? cf : cb ) << 2) |
                       (( ((glyph_pixels >> 2) & 1) ? cf : cb ) << 4 ) | (( ((glyph_pixels >> 3) & 1) ? cf : cb ) << 6);
                *p   = ( ((glyph_pixels >> 4) & 1) ? cf : cb ) | (( ((glyph_pixels >> 5) & 1) ? cf : cb ) << 2) | (*p & 0xF0);*/
            }
            return;
        }
        for (int glyph_line = 0; glyph_line < font_height; ++glyph_line) {
            uint8_t* p = p0 + ((width * glyph_line) >> 2);
            if (p >= pE) return;
//            if (p >= pE) return;
            uint8_t glyph_pixels = font_table[c * font_height + glyph_line];
            glyph_pixels = cb > cf ? ~glyph_pixels : glyph_pixels;
            *p = 0;
            *p   |= ((glyph_pixels    )  & 1) ? 0b00000011 : 0b00000001;
            *p   |= ((glyph_pixels << 1) & 1) ? 0b00001100 : 0b00000100;
            *p   |= ((glyph_pixels << 2) & 1) ? 0b00110000 : 0b00010000;
            *p++ |= ((glyph_pixels << 3) & 1) ? 0b11000000 : 0b01000000; if (p >= pE) return;
            *p = 0;
            *p   |= ((glyph_pixels << 4) & 1) ? 0b00000011 : 0b00000001;
            *p   |= ((glyph_pixels << 5) & 1) ? 0b00001100 : 0b00000100;
            *p   |= ((glyph_pixels << 6) & 1) ? 0b00110000 : 0b00010000;
            *p   |= ((glyph_pixels << 7)    ) ? 0b11000000 : 0b01000000;
        }
        return;
    }
    if (bit == 1) {
        uint32_t height = get_screen_height();
        uint8_t* p0 = graphics_buffer + ((width * y * font_height + x * font_width) >> 3);
//        uint8_t* pE = graphics_buffer + ((width * height) >> 3);
        uint8_t cf = color; // & 1; // TODO: mapping
        uint8_t cb = bgcolor; // & 1;
        if (font_width == 6) {
            for (int glyph_line = 0; glyph_line < font_height; ++glyph_line) {
                uint8_t* p = p0 + ((width * glyph_line) >> 3);
//                if (p >= pE) return;
                uint8_t glyph_pixels = font_table[c * font_height + glyph_line];
                *p = cb > cf ? ~glyph_pixels : glyph_pixels;
                if (p >= pE) return;
                continue;
                *p = (   (glyph_pixels & 1)       ? cf : cb )
                   | (( ((glyph_pixels >> 1) & 1) ? cf : cb ) << 1)
                   | (( ((glyph_pixels >> 2) & 1) ? cf : cb ) << 2)
                   | (( ((glyph_pixels >> 3) & 1) ? cf : cb ) << 3)
                   | (( ((glyph_pixels >> 4) & 1) ? cf : cb ) << 4)
                   | (( ((glyph_pixels >> 5) & 1) ? cf : cb ) << 5)
                   | (*p & 0b1100000);
            }
            return;
        }
        for (int glyph_line = 0; glyph_line < font_height; ++glyph_line) {
            uint8_t* p = p0 + ((width * glyph_line) >> 3);
            if (p >= pE) return;
//            if (p >= pE) return;
            uint8_t glyph_pixels = font_table[c * font_height + glyph_line]; // TODO: ch = c * font_height;
            *p = cb > cf ? ~glyph_pixels : glyph_pixels;
            continue;
        }
        return;
    }
}

void gouta(char* buf) {
    uint8_t* graphics_buffer = get_buffer();
    if (!graphics_buffer) {
        return;
    }
    uint32_t width = get_screen_width();
    uint32_t height = get_screen_height();
    char c;
    if (!is_buffer_text()) {
        while (c = *buf++) {
            if (c == '\r') continue; // ignore DOS stile \r\n, only \n to start new line
            if (c == '\n') {
                pos_x = 0;
                pos_y++;
                graphics_rollup(graphics_buffer, width);
                continue;
            }
            if (pos_x >= width / font_width) {
                pos_x = 0;
                pos_y++;
                graphics_rollup(graphics_buffer, width);
                common_print_char(graphics_buffer, width, height, pos_x, pos_y, con_color, con_bgcolor, c);
            } else {
                common_print_char(graphics_buffer, width, height, pos_x, pos_y, con_color, con_bgcolor, c);
            }
            pos_x++;
        }
        return;
    }
    uint8_t* t_buf = graphics_buffer + width * 2 * pos_y + 2 * pos_x;
    while (c = *buf++) {
        if (c == '\r') continue; // ignore DOS stile \r\n, only \n to start new line
        if (c == '\n') {
            pos_x = 0;
            pos_y++;
            t_buf = common_rollup(graphics_buffer, t_buf, width, height);
            continue;
        }
        pos_x++;
        if (pos_x >= width) {
            pos_x = 0;
            pos_y++;
            t_buf = common_rollup(graphics_buffer, t_buf, width, height);
            *t_buf++ = c;
            *t_buf++ = con_bgcolor << 4 | con_color & 0xF;
            pos_x++;
        } else {
            *t_buf++ = c;
            *t_buf++ = con_bgcolor << 4 | con_color & 0xF;
        }
    }
}
uint8_t get_font_width(void) {
    return font_width;
}
uint8_t get_font_height(void) {
    return font_height;
}
uint8_t get_screen_bitness() {
    return bitness;
}

void fgoutf(FIL *f, const char *__restrict str, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, str);
    vsnprintf(buf, 512, str, ap); // TODO: optimise (skip)
    va_end(ap);
    if (!f) {
        gouta(buf);
    } else {
        UINT bw;
        f_write(f, buf, strlen(buf), &bw); // TODO: error handling
    }
}

void graphics_set_con_pos(int x, int y) {
    pos_x = x;
    pos_y = y;
}

void __putc(char c) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (ctx && ctx->std_out) {
        UINT bw;
        f_write(ctx->std_out, &c, 1, &bw);
    } else {
        char t[2] = { c, 0 };
        gouta(t);
    }
}
