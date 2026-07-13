#include <stdint.h>
// posix
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
// MOS specific
#include <system/ff.h>
#include <system/ver.h>
#include <system/cmd.h>
#include <system/rtos.h>
#include <system/time.h>
#include <system/misc.h>
#include <system/terminal.h>
#include <graphics/schema.h>
#include <graphics/draw.h>
#include <graphics/console.h>

const char TEMP[] = "TEMP";
const char _mc_con[] = ".mc.con";

static const char* title;

#define VIEW_LINE_BUFFER_SIZE 256

static FILE* view_file;
static off_t file_size;
static off_t top_offset;
static off_t next_line_offset;
static off_t next_page_offset;
static size_t top_line;
static size_t visible_lines;
static bool reached_eof;

static void m_window();
static void redraw_window();
static void bottom_line();
static bool m_prompt(const char* txt);
static inline void handle_down_pressed();

static bool marked_to_exit = false;

int __required_m_api_verion(void) {
    return M_API_VERSION;
}

// only SIGTERM is supported for now
int signal(void) {
    marked_to_exit = true;
    return 0;
}

#define PANEL_TOP_Y 0
#define FIRST_FILE_LINE_ON_PANEL_Y (PANEL_TOP_Y + 1)
static uint32_t MAX_HEIGHT, MAX_WIDTH;
static uint8_t PANEL_LAST_Y, F_BTN_Y_POS, LAST_FILE_LINE_ON_PANEL_Y;
#define TOTAL_SCREEN_LINES MAX_HEIGHT

static volatile uint32_t lastCleanableScanCode;
static volatile uint32_t lastSavedScanCode;
static bool hidePannels = false;

static volatile bool ctrlPressed = false;
static volatile bool altPressed = false;
static volatile bool delPressed = false;
static volatile bool leftPressed = false;
static volatile bool rightPressed = false;
static volatile bool upPressed = false;
static volatile bool downPressed = false;

typedef enum nav_event {
    NAV_EVENT_NONE = 0,
    NAV_EVENT_HOME,
    NAV_EVENT_END,
    NAV_EVENT_PAGE_UP,
    NAV_EVENT_PAGE_DOWN
} nav_event_t;

#define NAV_EVENT_QUEUE_SIZE 8
static volatile uint8_t nav_event_head;
static volatile uint8_t nav_event_tail;
static volatile nav_event_t nav_event_queue[NAV_EVENT_QUEUE_SIZE];

static inline void nav_event_push(nav_event_t event) {
    uint8_t next = (uint8_t)((nav_event_head + 1) % NAV_EVENT_QUEUE_SIZE);
    if (next == nav_event_tail) {
        return;
    }
    nav_event_queue[nav_event_head] = event;
    nav_event_head = next;
}

static inline nav_event_t nav_event_pop(void) {
    if (nav_event_tail == nav_event_head) {
        return NAV_EVENT_NONE;
    }
    nav_event_t event = nav_event_queue[nav_event_tail];
    nav_event_tail = (uint8_t)((nav_event_tail + 1) % NAV_EVENT_QUEUE_SIZE);
    return event;
}

static size_t line_s = 0;
static size_t line_e = 0;

inline static void scan_code_processed() {
  if (lastCleanableScanCode) {
      lastSavedScanCode = lastCleanableScanCode;
  }
  lastCleanableScanCode = 0;
}

static void scan_code_cleanup() {
  lastSavedScanCode = 0;
  lastCleanableScanCode = 0;
}

// type of F1-F10 function pointer
typedef void (*fn_1_12_ptr)(uint8_t);

#define BTN_WIDTH 8
typedef struct fn_1_12_tbl_rec {
    char pre_mark;
    char mark;
    char name[BTN_WIDTH];
    fn_1_12_ptr action;
} fn_1_12_tbl_rec_t;

#define BTNS_COUNT 12
typedef fn_1_12_tbl_rec_t fn_1_12_tbl_t[BTNS_COUNT];

static color_schema_t* pcs;

int _init(void) {
    hidePannels = false;

    ctrlPressed = false;
    altPressed = false;
    delPressed = false;
    leftPressed = false;
    rightPressed = false;
    upPressed = false;
    downPressed = false;
    nav_event_head = 0;
    nav_event_tail = 0;

    marked_to_exit = false;
    line_s = 0;
    line_e = 0;
    view_file = NULL;
    file_size = 0;
    top_offset = 0;
    next_line_offset = 0;
    next_page_offset = 0;
    top_line = 0;
    visible_lines = 0;
    reached_eof = false;
    scan_code_cleanup();
}

static void do_nothing(uint8_t cmd) {
    char line[32];
    snprintf(line, MAX_WIDTH, "CMD: F%d", cmd + 1);
    const line_t lns[2] = {
        { -1, "Not yet implemented function" },
        { -1, line }
    };
    const lines_t lines = { sizeof(lns) / sizeof(lns[0]), 3, lns };
    draw_box(pcs, (MAX_WIDTH - 60) / 2, 7, 60, 10, "Info", &lines);
    vTaskDelay(1500);
    redraw_window();
}

static bool m_prompt(const char* txt) {
    const line_t lns[1] = {
        { -1, txt },
    };
    const lines_t lines = { sizeof(lns) / sizeof(lns[0]), 2, lns };
    draw_box(pcs, (MAX_WIDTH - 60) / 2, 7, 60, 10, "Are you sure?", &lines);
    bool yes = true;
    draw_button(pcs, (MAX_WIDTH - 60) / 2 + 16, 12, 11, "Yes", yes);
    draw_button(pcs, (MAX_WIDTH - 60) / 2 + 35, 12, 10, "No", !yes);
    while(1) {
        char c = getch_now();
        if (c == CHAR_CODE_ENTER) {
            scan_code_cleanup();
            return yes;
        }
        if (c == CHAR_CODE_TAB || leftPressed || rightPressed) { // TODO: own msgs cycle
            yes = !yes;
            draw_button(pcs, (MAX_WIDTH - 60) / 2 + 16, 12, 11, "Yes", yes);
            draw_button(pcs, (MAX_WIDTH - 60) / 2 + 35, 12, 10, "No", !yes);
            leftPressed = rightPressed = false;
            scan_code_cleanup();
        }
        if (c == CHAR_CODE_ESC) {
            scan_code_cleanup();
            return false;
        }
    }
}

static void mark_to_exit(uint8_t cmd) {
    marked_to_exit = true;
}

static void m_info(uint8_t cmd) {
    line_t plns[2] = {
        { 1, " It is Murmulator Viewer" },
        { 1, " Esc or F10 to Exit" }
    };
    lines_t lines = { sizeof(plns) / sizeof(plns[0]), 0, plns };
    draw_box(pcs, 5, 2, MAX_WIDTH - 15, MAX_HEIGHT - 6, "Help", &lines);
    char c = 0;
    while(c != CHAR_CODE_ENTER && c != CHAR_CODE_ESC) {
        c = getch_now();
    }
    redraw_window();
}

static fn_1_12_tbl_t fn_1_12_tbl = {
    ' ', '1', " Info ", m_info,
    ' ', '2', "      ", do_nothing,
    ' ', '3', "      ", do_nothing,
    ' ', '4', "      ", do_nothing,
    ' ', '5', "      ", do_nothing,
    ' ', '6', "      ", do_nothing,
    ' ', '7', "      ", do_nothing,
    ' ', '8', "      ", do_nothing,
    ' ', '9', "      ", do_nothing,
    '1', '0', " Exit ", mark_to_exit,
    '1', '1', "      ", do_nothing,
    '1', '2', "      ", do_nothing
};

static fn_1_12_tbl_t fn_1_12_tbl_alt = {
    ' ', '1', "      ", do_nothing,
    ' ', '2', "      ", do_nothing,
    ' ', '3', "      ", do_nothing,
    ' ', '4', "      ", do_nothing,
    ' ', '5', "      ", do_nothing,
    ' ', '6', "      ", do_nothing,
    ' ', '7', "      ", do_nothing,
    ' ', '8', "      ", do_nothing,
    ' ', '9', "      ", do_nothing,
    '1', '0', " Exit ", mark_to_exit,
    '1', '1', "      ", do_nothing,
    '1', '2', "      ", do_nothing
};

static fn_1_12_tbl_t fn_1_12_tbl_ctrl = {
    ' ', '1', "      ", do_nothing,
    ' ', '2', "      ", do_nothing,
    ' ', '3', "      ", do_nothing,
    ' ', '4', "      ", do_nothing,
    ' ', '5', "      ", do_nothing,
    ' ', '6', "      ", do_nothing,
    ' ', '7', "      ", do_nothing,
    ' ', '8', "      ", do_nothing,
    ' ', '9', "      ", do_nothing,
    '1', '0', " Exit ", mark_to_exit,
    '1', '1', "      ", do_nothing,
    '1', '2', "      ", do_nothing
};

static inline const fn_1_12_tbl_t* actual_fn_1_12_tbl() {
    const fn_1_12_tbl_t * ptbl = &fn_1_12_tbl;
    if (altPressed) {
        ptbl = &fn_1_12_tbl_alt;
    } else if (ctrlPressed) {
        ptbl = &fn_1_12_tbl_ctrl;
    }
    return ptbl;
}

static void draw_fn_btn(const fn_1_12_tbl_rec_t* prec, int left, int top) {
    char line[10];
    snprintf(line, MAX_WIDTH, "       ");
    // 1, 2, 3... button mark
    line[0] = prec->pre_mark;
    line[1] = prec->mark;
    draw_text(line, left, top, pcs->FOREGROUND_F1_12_COLOR, pcs->BACKGROUND_F1_12_COLOR);
    // button
    snprintf(line, MAX_WIDTH, prec->name);
    draw_text(line, left + 2, top, pcs->FOREGROUND_F_BTN_COLOR, pcs->BACKGROUND_F_BTN_COLOR);
}

static void bottom_line() {
    int i = 0;
    for (; i < BTNS_COUNT && (i + 1) * BTN_WIDTH < MAX_WIDTH; ++i) {
        const fn_1_12_tbl_rec_t* rec = &(*actual_fn_1_12_tbl())[i];
        draw_fn_btn(rec, i * BTN_WIDTH, F_BTN_Y_POS);
    }
    i = i * BTN_WIDTH;
    for (; i < MAX_WIDTH; ++i) {
        draw_text(" ", i, F_BTN_Y_POS, pcs->FOREGROUND_F1_12_COLOR, pcs->BACKGROUND_F1_12_COLOR);
    }
}

static void redraw_window() {
    m_window();
    bottom_line();
}

static bool read_view_line(char* buff, size_t buff_size) {
    size_t len = 0;
    bool got_data = false;

    for (;;) {
        int c = fgetc(view_file);
        if (c == EOF) {
            break;
        }
        got_data = true;
        if (c == '\n') {
            break;
        }
        if (c == '\r') {
            int next = fgetc(view_file);
            if (next != '\n' && next != EOF) {
                ungetc(next, view_file);
            }
            break;
        }
        if (len + 1 < buff_size) {
            buff[len++] = (char)c;
        }
    }

    buff[len] = 0;
    return got_data;
}

static off_t find_previous_line(off_t offset) {
    if (offset <= 0) {
        return 0;
    }

    off_t pos = offset - 1;
    if (fseeko(view_file, pos, SEEK_SET) != 0) {
        return offset;
    }

    int c = fgetc(view_file);
    if (c == '\n' && pos > 0) {
        --pos;
    }

    while (pos > 0) {
        if (fseeko(view_file, pos - 1, SEEK_SET) != 0) {
            return offset;
        }
        c = fgetc(view_file);
        if (c == '\n') {
            return pos;
        }
        --pos;
    }
    return 0;
}

static void m_window() {
    if (hidePannels) return;
    draw_panel(pcs, 0, PANEL_TOP_Y, MAX_WIDTH, PANEL_LAST_Y + 1, title, 0);

    if (!view_file || fseeko(view_file, top_offset, SEEK_SET) != 0) {
        const line_t lns[2] = {
            { -1, "Unable to read file:" },
            { -1, title }
        };
        const lines_t lines = { sizeof(lns) / sizeof(lns[0]), 3, lns };
        draw_box(pcs, (MAX_WIDTH - 60) / 2, 7, 60, 10, "Error", &lines);
        sleep_ms(1500);
        return;
    }

    char buff[VIEW_LINE_BUFFER_SIZE];
    const size_t height = MAX_HEIGHT - 3;
    const size_t draw_width = MAX_WIDTH > 2 ? MAX_WIDTH - 2 : 0;
    size_t y = 1;

    visible_lines = 0;
    reached_eof = false;
    next_line_offset = top_offset;
    next_page_offset = top_offset;

    while (y <= height) {
        off_t line_offset = ftello(view_file);
        if (!read_view_line(buff, sizeof(buff))) {
            reached_eof = true;
            break;
        }

        if (visible_lines == 1) {
            next_line_offset = line_offset;
        }
        if (draw_width < sizeof(buff)) {
            buff[draw_width] = 0;
        }
        draw_text(buff, 1, y, pcs->FOREGROUND_FIELD_COLOR, pcs->BACKGROUND_FIELD_COLOR);
        ++visible_lines;
        ++y;
    }

    next_page_offset = ftello(view_file);
    if (visible_lines < 2) {
        next_line_offset = next_page_offset;
    }
    if (next_page_offset >= file_size) {
        reached_eof = true;
    }

    line_s = top_line;
    line_e = visible_lines ? top_line + visible_lines - 1 : top_line;
    snprintf(buff, sizeof(buff), " Line: %u  Offset: %u/%u",
             (unsigned)(top_line + 1),
             (unsigned)top_offset,
             (unsigned)file_size);
    draw_text(
        buff,
        2,
        PANEL_LAST_Y,
        pcs->FOREGROUND_FIELD_COLOR,
        pcs->BACKGROUND_FIELD_COLOR
    );
}

static scancode_handler_t scancode_handler;

static bool scancode_handler_impl(const uint32_t ps2scancode) { // core ?
    lastCleanableScanCode = ps2scancode & 0xFF; // ensure
    bool numlock = get_leds_stat() & PS2_LED_NUM_LOCK;
    if (ps2scancode == 0xE048 || (ps2scancode == 0x48 && !numlock)) {
        upPressed = true;
        goto r;
    } else if (ps2scancode == 0xE0C8 || (ps2scancode == 0xC8 && !numlock)) {
        upPressed = false;
        goto r;
    } else if (ps2scancode == 0xE050 || (ps2scancode == 0x50 && !numlock)) {
        downPressed = true;
        goto r;
    } else if (ps2scancode == 0xE0D0 || (ps2scancode == 0xD0 && !numlock)) {
        downPressed = false;
        goto r;
    } else if (ps2scancode == 0xE047 || (ps2scancode == 0x47 && !numlock)) {
        nav_event_push(NAV_EVENT_HOME);
        lastCleanableScanCode = 0;
        goto r;
    } else if (ps2scancode == 0xE04F || (ps2scancode == 0x4F && !numlock)) {
        nav_event_push(NAV_EVENT_END);
        lastCleanableScanCode = 0;
        goto r;
    } else if (ps2scancode == 0xE049 || (ps2scancode == 0x49 && !numlock)) {
        nav_event_push(NAV_EVENT_PAGE_UP);
        lastCleanableScanCode = 0;
        goto r;
    } else if (ps2scancode == 0xE051 || (ps2scancode == 0x51 && !numlock)) {
        nav_event_push(NAV_EVENT_PAGE_DOWN);
        lastCleanableScanCode = 0;
        goto r;
    } else if (ps2scancode == 0xE0C7 || ps2scancode == 0xE0CF ||
               ps2scancode == 0xE0C9 || ps2scancode == 0xE0D1 ||
               (numlock &&
                (ps2scancode == 0x47 || ps2scancode == 0xC7 ||
                 ps2scancode == 0x4F || ps2scancode == 0xCF ||
                 ps2scancode == 0x49 || ps2scancode == 0xC9 ||
                 ps2scancode == 0x51 || ps2scancode == 0xD1))) {
        lastCleanableScanCode = 0;
        goto r;
    }
    uint8_t c = (uint8_t)ps2scancode & 0xFF;
    if (c == 0x4B) {
        leftPressed = true;
    } else if (c == 0xCB) {
        leftPressed = false;
    } else if (c == 0x4D) {
        rightPressed = true;
    } else if (c == 0xCD) {
        rightPressed = false;
    } else if (c == 0x38) {
        altPressed = true;
    } else if (c == 0xB8) {
        altPressed = false;
    } else if (c == 0x1D) {
        ctrlPressed = true;
    } else if (c == 0x9D) {
        ctrlPressed = false;
    } else if (c == 0x53) {
        delPressed = true;
    } else if (c == 0xD3) {
        delPressed = false;
    }
r:
    if (scancode_handler) {
        return scancode_handler(ps2scancode);
    }
    return false;
}

static inline void redraw_current_panel() {
    m_window();
}

static void enter_pressed() {
    handle_down_pressed();
}

inline static void handle_pagedown_pressed() {
    if (hidePannels || reached_eof || visible_lines == 0) return;
    top_offset = next_page_offset;
    top_line += visible_lines;
    m_window();
}

inline static void handle_down_pressed() {
    if (hidePannels || visible_lines < 2) return;
    top_offset = next_line_offset;
    ++top_line;
    m_window();
}

inline static void handle_pageup_pressed() {
    if (hidePannels || top_offset == 0 || visible_lines == 0) return;
    size_t count = visible_lines;
    while (count-- && top_offset > 0) {
        top_offset = find_previous_line(top_offset);
        if (top_line > 0) {
            --top_line;
        }
    }
    m_window();
}

inline static void handle_up_pressed() {
    if (hidePannels || top_offset == 0) return;
    top_offset = find_previous_line(top_offset);
    if (top_line > 0) {
        --top_line;
    }
    m_window();
}

inline static void handle_home_pressed() {
    if (hidePannels || top_offset == 0) return;
    top_offset = 0;
    top_line = 0;
    m_window();
}

static size_t count_lines_before(off_t offset) {
    if (offset <= 0 || fseeko(view_file, 0, SEEK_SET) != 0) {
        return 0;
    }

    size_t lines = 0;
    off_t pos = 0;
    int c;
    while (pos < offset && (c = fgetc(view_file)) != EOF) {
        ++pos;
        if (c == '\n') {
            ++lines;
        } else if (c == '\r') {
            int next = fgetc(view_file);
            if (next == '\n') {
                ++pos;
            } else if (next != EOF) {
                ungetc(next, view_file);
            }
            ++lines;
        }
    }
    return lines;
}

inline static void handle_end_pressed() {
    if (hidePannels || file_size <= 0) return;

    off_t offset = file_size;
    size_t count = MAX_HEIGHT - 3;
    while (count-- && offset > 0) {
        offset = find_previous_line(offset);
    }

    top_offset = offset;
    top_line = count_lines_before(top_offset);
    m_window();
}

inline static void fn_1_12_btn_pressed(uint8_t fn_idx) {
    if (fn_idx > 11) fn_idx -= 18; // F11-12
    (*actual_fn_1_12_tbl())[fn_idx].action(fn_idx);
}

inline static void cmd_backspace() {
}

inline static void handle_tab_pressed() {
    if (hidePannels) {
        return;
    }
}

inline static void restore_console(cmd_ctx_t* ctx) {
    op_console(ctx, f_read, FA_READ);
}

inline static void save_console(cmd_ctx_t* ctx) {
    op_console(ctx, (FRFpvUpU_ptr_t)f_write, FA_CREATE_ALWAYS | FA_WRITE);
}

inline static void hide_pannels() {
    hidePannels = !hidePannels;
    if (hidePannels) {
        restore_console(get_cmd_ctx());
    } else {
        save_console(get_cmd_ctx());
        m_window();
    }
}

static inline void work_cycle(cmd_ctx_t* ctx) {
    uint8_t repeat_cnt = 0;
    for(;;) {
        char c = getch_now();
        if (c) {
            if (c == CHAR_CODE_BS) cmd_backspace();
            else if (c == CHAR_CODE_UP) handle_up_pressed();
            else if (c == CHAR_CODE_DOWN) handle_down_pressed();
            else if (c == CHAR_CODE_TAB) handle_tab_pressed();
            else if (c == CHAR_CODE_ENTER) enter_pressed();
            else if (c == CHAR_CODE_ESC) mark_to_exit(9);
            else if (ctrlPressed && (c == 'o' || c == 'O' || c == 0x99 /*Щ*/ || c == 0xE9 /*щ*/)) hide_pannels();
        }

        if (lastSavedScanCode != lastCleanableScanCode && lastSavedScanCode > 0x80) {
            repeat_cnt = 0;
        } else {
            repeat_cnt++;
            if (repeat_cnt > 0xFE && lastSavedScanCode < 0x80) {
               lastCleanableScanCode = lastSavedScanCode + 0x80;
               repeat_cnt = 0;
            }
        }
        uint8_t sc = lastCleanableScanCode;
        if ((sc >= 0x3B && sc <= 0x44) || sc == 0x57 || sc == 0x58 || sc == 0x49 || sc == 0x51 || sc == 0x1C || sc == 0x9C) { // F1..12 down,/..
            scan_code_processed();
        } else if ((sc >= 0xBB && sc <= 0xC4) || sc == 0xD7 || sc == 0xD8) { // F1..12 up
            if (lastSavedScanCode == sc - 0x80) {
                fn_1_12_btn_pressed(sc - 0xBB);
                scan_code_processed();
            }
        } else if (sc == 0x1D || sc == 0x9D || sc == 0x38 || sc == 0xB8) { // Ctrl / Alt
            bottom_line();
            scan_code_processed();
        }

        switch (nav_event_pop()) {
            case NAV_EVENT_HOME:
                handle_home_pressed();
                break;
            case NAV_EVENT_END:
                handle_end_pressed();
                break;
            case NAV_EVENT_PAGE_UP:
                handle_pageup_pressed();
                break;
            case NAV_EVENT_PAGE_DOWN:
                handle_pagedown_pressed();
                break;
            case NAV_EVENT_NONE:
            default:
                break;
        }
        if(marked_to_exit) {
            restore_console(ctx);
            return;
        }
    }
}

inline static void start_viewer(cmd_ctx_t* ctx) {
    m_window();
    bottom_line();
    work_cycle(ctx);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: mcview <file>\n");
        return 1;
    }
    title = argv[1];
    view_file = fopen(title, "rb");
    if (!view_file) {
        fprintf(stderr, "Unable to open file: %s\n", title);
        return 1;
    }
    if (fseeko(view_file, 0, SEEK_END) != 0) {
        fprintf(stderr, "Unable to seek file: %s\n", title);
        fclose(view_file);
        return 1;
    }
    file_size = ftello(view_file);
    if (file_size < 0 || fseeko(view_file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "Unable to determine file size: %s\n", title);
        fclose(view_file);
        return 1;
    }
    graphics_set_con_pos(-1, 0);
    MAX_WIDTH = get_console_width();
    MAX_HEIGHT = get_console_height();
    F_BTN_Y_POS = TOTAL_SCREEN_LINES - 1;
    PANEL_LAST_Y = F_BTN_Y_POS - 1;
    LAST_FILE_LINE_ON_PANEL_Y = PANEL_LAST_Y - 1;
    cmd_ctx_t* ctx = get_cmd_ctx();
    save_console(ctx);

    pcs = calloc(1, sizeof(color_schema_t));
    if (!pcs) {
        fprintf(stderr, "Not enough memory for viewer state\n");
        fclose(view_file);
        return 1;
    }
    pcs->BACKGROUND_FIELD_COLOR = 1, // Blue
    pcs->FOREGROUND_FIELD_COLOR = 7, // White
    pcs->HIGHLIGHTED_FIELD_COLOR = 15, // LightWhite
    pcs->BACKGROUND_F1_12_COLOR = 0, // Black
    pcs->FOREGROUND_F1_12_COLOR = 7, // White
    pcs->BACKGROUND_F_BTN_COLOR = 3, // Green
    pcs->FOREGROUND_F_BTN_COLOR = 0, // Black
    pcs->BACKGROUND_CMD_COLOR = 0, // Black
    pcs->FOREGROUND_CMD_COLOR = 7, // White
    pcs->BACKGROUND_SEL_BTN_COLOR = 11, // Light Blue
    pcs->FOREGROUND_SELECTED_COLOR = 0, // Black
    pcs->BACKGROUND_SELECTED_COLOR = 11, // Light Blue

    scancode_handler = get_scancode_handler();
    set_scancode_handler(scancode_handler_impl);

    start_viewer(ctx);

    set_scancode_handler(scancode_handler);
    fclose(view_file);
    view_file = NULL;
    free(pcs);
    graphics_set_con_pos(0, PANEL_LAST_Y);

    return 0;
}
