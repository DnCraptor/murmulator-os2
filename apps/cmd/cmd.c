#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "m-os-api-c-string.h"

// #define DEBUG
#if !M_API_VERSION
#define M_API_VERSION 26
#endif

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

inline static void blimp(uint32_t cnt, uint32_t ticks_to_delay) { // 194
    typedef void (*fn_ptr_t)(uint32_t, uint32_t);
    ((fn_ptr_t)_sys_table_ptrs[194])(cnt, ticks_to_delay);
}
inline static void gbackspace() {
    typedef void (*fn_ptr_t)();
    ((fn_ptr_t)_sys_table_ptrs[119])();
}

typedef struct cmd_ctx {
    uint32_t argc;
    // ...
} cmd_ctx_t;
inline static char* get_ctx_var(cmd_ctx_t* src, const char* k) {
    typedef char* (*fn_ptr_t)(cmd_ctx_t*, const char*);
    return ((fn_ptr_t)_sys_table_ptrs[140])(src, k);
}
inline static void graphics_set_con_color(uint8_t color, uint8_t bgcolor) {
    typedef void (*graphics_set_con_color_ptr_t)(uint8_t color, uint8_t bgcolor);
    ((graphics_set_con_color_ptr_t)_sys_table_ptrs[43])(color, bgcolor);
}
inline static bool cmd_enter_helper(cmd_ctx_t* ctx, string_t* s_cmd) {
    typedef bool (*fn_ptr_t)(cmd_ctx_t* ctx, string_t* s_cmd);
    return ((fn_ptr_t)_sys_table_ptrs[235])(ctx, s_cmd);
}
inline static int history_steps(cmd_ctx_t* ctx, int cmd_history_idx, string_t* s_cmd) {
    typedef int (*fn_ptr_t)(cmd_ctx_t* ctx, int cmd_history_idx, string_t* s_cmd);
    return ((fn_ptr_t)_sys_table_ptrs[234])(ctx, cmd_history_idx, s_cmd);
}
inline static void show_logo(bool with_top) {
    typedef void (*fn_ptr_t)(bool);
    ((fn_ptr_t)_sys_table_ptrs[183])(with_top);
}
inline static cmd_ctx_t* get_cmd_ctx() {
    typedef cmd_ctx_t* (*fn_ptr_t)();
    return ((fn_ptr_t)_sys_table_ptrs[138])();
}
inline static void cleanup_ctx(cmd_ctx_t* src) {
    typedef void (*fn_ptr_t)(cmd_ctx_t*);
    ((fn_ptr_t)_sys_table_ptrs[139])(src);
}
inline static void cmd_tab(cmd_ctx_t* ctx, string_t* s_cmd) {
    typedef void (*fn_ptr_t)(cmd_ctx_t* ctx, string_t* s_cmd);
    ((fn_ptr_t)_sys_table_ptrs[233])(ctx, s_cmd);
}
#define CHAR_CODE_BS    8
#define CHAR_CODE_UP    17
#define CHAR_CODE_DOWN  18
#define CHAR_CODE_ENTER '\n'
#define CHAR_CODE_TAB   '\t'
#define CHAR_CODE_ESC   0x1B
#define CHAR_CODE_EOF   0xFF

static string_t* s_cmd = NULL;

inline static void cmd_backspace() {
    if (s_cmd->size == 0) {
        blimp(10, 5);
        return;
    }
    string_resize(s_cmd, s_cmd->size - 1);
    gbackspace();
}

inline static void type_char(char c) {
    putc(c, stdout);
    string_push_back_c(s_cmd, c);
}

inline static void prompt(cmd_ctx_t* ctx) {
    const char* cd = get_ctx_var(ctx, "CD");
    graphics_set_con_color(13, 0);
    printf("[%s]", cd);
    graphics_set_con_color(7, 0);
    printf("$ ");
}

inline static bool cmd_enter(cmd_ctx_t* ctx) {
    putc('\n', stdout);
    if ( cmd_enter_helper(ctx, s_cmd) ) {
        return true;
    }
    prompt(ctx);
    string_resize(s_cmd, 0);
    return false;
}

inline static void cancel_entered() {
    while(s_cmd->size) {
        string_resize(s_cmd, s_cmd->size - 1);
        gbackspace();
    }
}

static int cmd_history_idx;

inline static void cmd_up(cmd_ctx_t* ctx) {
    cancel_entered();
    cmd_history_idx--;
    int idx = history_steps(ctx, cmd_history_idx, s_cmd);
    if (cmd_history_idx < 0) cmd_history_idx = idx;
    printf("%s", s_cmd->p);
}

inline static void cmd_down(cmd_ctx_t* ctx) {
    cancel_entered();
    if (cmd_history_idx == -2) cmd_history_idx = -1;
    cmd_history_idx++;
    history_steps(ctx, cmd_history_idx, s_cmd);
    printf("%s", s_cmd->p);
}

int main(int argc, char** argv) {
    show_logo(false);
    cmd_ctx_t* ctx = get_cmd_ctx();
    cleanup_ctx(ctx);
    s_cmd = new_string_v();
    prompt(ctx);
    cmd_history_idx = -2;
    while(1) {
        char c = getchar();
        if (c) {
            if (c == CHAR_CODE_BS) cmd_backspace();
            else if (c == CHAR_CODE_UP) cmd_up(ctx);
            else if (c == CHAR_CODE_DOWN) cmd_down(ctx);
            else if (c == CHAR_CODE_TAB) cmd_tab(ctx, s_cmd);
            else if (c == CHAR_CODE_ESC) {
                blimp(15, 1);
                printf("\n");
                string_resize(s_cmd, 0);
                prompt(ctx);
            }
            else if (c == CHAR_CODE_ENTER) {
                if ( cmd_enter(ctx) ) {
                    delete_string(s_cmd);
                    //goutf("[%s]EXIT to exec, stage: %d\n", ctx->curr_dir, ctx->stage);
                    return 0;
                }
            }
            else if (c != CHAR_CODE_EOF) type_char(c);
        }
    }
    __builtin_unreachable();
}
