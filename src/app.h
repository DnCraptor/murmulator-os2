#pragma once
#ifndef _APP_H
#define _APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "FreeRTOS.h"
#include "task.h"

#include "ff.h"
#include "../../api/m-os-api-c-list.h"
#include "../../api/m-os-api-c-string.h"

#define FIRMWARE_MARKER_FN "/.firmware"

typedef struct {
    char* del_addr;
    char* prg_addr;
    uint16_t sec_num;
} sect_entry_t;

inline static void sect_entry_deallocator(sect_entry_t* s) {
    if (s->del_addr) vPortFree(s->del_addr);
    vPortFree(s);
}

typedef int (*bootb_ptr_t)( void );

typedef struct {
    bootb_ptr_t bootb[5];
    list_t* /*sect_entry_t*/ sections;
} bootb_ctx_t;

typedef struct {
    const char* key;
    char* value;
} vars_t;

typedef enum {
    INITIAL,
    PREPARED,
    FOUND,
    VALID,
    LOAD,
    EXECUTED,
    INVALIDATED,
    SIGTERM
} cmd_exec_stage_t;

typedef struct cmd_ctx {
    uint32_t argc;
    char** argv;
    char* orig_cmd;

    FIL* std_in;
    FIL* std_out;
    FIL* std_err;
    int ret_code;
    bool detached;

    struct cmd_ctx* prev;
    bootb_ctx_t* pboot_ctx;

    vars_t* vars;
    size_t vars_num;

    struct cmd_ctx* next;

    volatile cmd_exec_stage_t stage;
    void* user_data;
    bool forse_flash;
} cmd_ctx_t;

typedef struct UF2_Block_s {
    // 32 byte header
    uint32_t magicStart0;
    uint32_t magicStart1;
    uint32_t flags;
    uint32_t targetAddr;
    uint32_t payloadSize;
    uint32_t blockNo;
    uint32_t numBlocks;
    uint32_t fileSize; // or familyID;
    uint8_t data[476];
    uint32_t magicEnd;
} UF2_Block_t;

cmd_ctx_t* get_cmd_startup_ctx(); // system
char* get_ctx_var(cmd_ctx_t*, const char* key);
void set_ctx_var(cmd_ctx_t*, const char* key, const char* val);
bool exists(cmd_ctx_t* ctx);
void cleanup_ctx(cmd_ctx_t* src);
cmd_ctx_t* clone_ctx(cmd_ctx_t* src);
void remove_ctx(cmd_ctx_t*);
void cleanup_bootb_ctx(cmd_ctx_t* ctx);

bool is_new_app(cmd_ctx_t* ctx);
bool run_new_app(cmd_ctx_t* ctx);

bool load_app(cmd_ctx_t* ctx);
void exec(cmd_ctx_t* ctx);
cmd_ctx_t* get_cmd_ctx();

FIL * get_stdout(); // old system
FIL * get_stderr(); // old system

bool cmd_enter_helper(cmd_ctx_t* ctx, string_t* s_cmd);

char* next_token(char* t);

char* concat(const char* s1, const char* s2);
char* concat2(const char* s1, size_t sz, const char* s2);
char* copy_str(const char* s);

void vCmdTask(void *pv);
void app_signal(void);
int kill(uint32_t task_number);

void flash_block(uint8_t* buffer, size_t flash_target_offset);
bool load_firmware(char* pathname);
void link_firmware(FIL* pf, const char* pathname);

extern uint32_t flash_size;
void get_cpu_flash_jedec_id(uint8_t rx[4]);
void run_app(char * name);
int history_steps(cmd_ctx_t* ctx, int cmd_history_idx, string_t* s_cmd);

void mallocFailedHandler();
void overflowHook( TaskHandle_t pxTask, char *pcTaskName );
void reboot_me(void);

#ifdef __cplusplus
}
#endif

#endif

