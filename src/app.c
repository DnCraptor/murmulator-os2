#include "app.h"
#include <pico/platform.h>
#include <hardware/flash.h>
#include <pico/bootrom.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include "ff.h"
#include "graphics.h"
#include "keyboard.h"
#include "sys_table.h"
#include "elf32.h"
#include "../../api/m-os-api-c-hash.h"

extern const char TEMP[];
const char _flash_me[] = ".flash_me";

#define M_OS_APP_TABLE_BASE ((size_t*)0x10000000ul)
typedef int (*boota_ptr_t)( void *argv );
#define OS_FLASH_SIZE (132 << 10) // 4k sys table + 64k LFA + 64k HFA

volatile bool reboot_is_requested = false;

typedef struct {
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

uint32_t flash_size;

static void __in_hfa() debug_sections(sect_entry_t* sect_entries) {
    if (sect_entries) {
        for (uint16_t i = 0; sect_entries[i].del_addr != 0; ++i) {
            goutf("sec #%d: [%p][%p] %d\n", i, sect_entries[i].del_addr, sect_entries[i].prg_addr, sect_entries[i].sec_num);
        }
    }
}

inline static uint32_t __in_hfa() read_flash_block(FIL * f, uint8_t * buffer, uint32_t expected_flash_target_offset, UF2_Block_t* puf2, size_t* psz) {
    *psz = 0;
    UINT bytes_read = 0;
    uint32_t data_sector_index = 0;
    for(; data_sector_index < FLASH_SECTOR_SIZE; data_sector_index += 256) {
        //fgoutf(get_stdout(), "Read block: %ph; ", f_tell(f));
        f_read(f, puf2, sizeof(UF2_Block_t), &bytes_read);
        if (!bytes_read) break;
        if (puf2->targetAddr == XIP_BASE + 0xFFFF00) { // ignore such block
            f_read(f, puf2, sizeof(UF2_Block_t), &bytes_read);
            if (!bytes_read) break;
        }
        *psz += bytes_read;
        //fgoutf(get_stdout(), "(%d bytes) ", bytes_read);
        if (expected_flash_target_offset != puf2->targetAddr - XIP_BASE) {
            f_lseek(f, f_tell(f) - sizeof(UF2_Block_t)); // we will reread this block, it doesnt belong to this continues block
            expected_flash_target_offset = puf2->targetAddr - XIP_BASE;
            *psz -= bytes_read;
            //fgoutf(get_stdout(), "Flash target offset: %ph\n", expected_flash_target_offset);
            break;
        }
        //fgoutf(get_stdout(), "Flash target offset: %ph\n", expected_flash_target_offset);
        memcpy(buffer + data_sector_index, puf2->data, 256);
        expected_flash_target_offset += 256;
    }
    return expected_flash_target_offset;
}

// TODO: remove on exit from app (ctx_cleanup?)
static size_t flash_addr = M_OS_APP_TABLE_BASE;
static list_t* flash_list = NULL;
static list_t* lst = NULL;
typedef struct to_flash_rec {
    size_t offset;
    size_t size;
} to_flash_rec_t;

size_t __in_hfa() free_app_flash(void) {
    return flash_size - (OS_FLASH_SIZE << 10) - (flash_addr - XIP_BASE);
}

void __not_in_flash_func(flash_block)(uint8_t* buffer, size_t flash_target_offset) {
    if (!flash_list) {
        flash_list = new_list_v(0, 0, 0);
    }
    node_t* n = flash_list->first;
    while (n) {
        to_flash_rec_t* rec = (to_flash_rec_t*)n->data;
        if (flash_target_offset >= rec->offset && flash_target_offset < rec->offset + FLASH_SECTOR_SIZE) {
            fgoutf(get_stdout(),
                "WARN: Attempt to use already allocated flash block: [%p]-[%p] (rejected)\n",
                 flash_target_offset,
                 flash_target_offset + FLASH_SECTOR_SIZE
            );
            return;
        }
        n = n->next;
    }
    to_flash_rec_t* rec = (to_flash_rec_t*)pvPortMalloc(sizeof(to_flash_rec_t));
    rec->offset = flash_target_offset;
    rec->size = FLASH_SECTOR_SIZE;
    flash_addr = flash_target_offset + FLASH_SECTOR_SIZE + XIP_BASE;
    list_push_back(flash_list, rec);
    uint8_t *e = (uint8_t*)(XIP_BASE + flash_target_offset);
    for (uint32_t i = 0; i < FLASH_SECTOR_SIZE; ++i) {
        if (e[i] != buffer[i]) {
            e = 0;
            break;
        }
    }
    if (e) { // the block is already eq.s to flash state
        return;
    }
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    multicore_lockout_start_blocking();
    const uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(flash_target_offset, FLASH_SECTOR_SIZE);
    flash_range_program(flash_target_offset, buffer, FLASH_SECTOR_SIZE);
    restore_interrupts(ints);
    multicore_lockout_end_blocking();
    gpio_put(PICO_DEFAULT_LED_PIN, false);
}

void __in_hfa() link_firmware(FIL* pf, const char* pathname) {
    f_open(pf, FIRMWARE_MARKER_FN, FA_CREATE_ALWAYS | FA_CREATE_NEW | FA_WRITE);
    fgoutf(pf, "%s\n", pathname);
    f_close(pf);
}

bool __not_in_flash_func(load_firmware_sram)(char* pathname) {
    FILINFO* pfi = (FILINFO*)pvPortMalloc(sizeof(FILINFO));
    FIL* pf = (FIL*)pvPortMalloc(sizeof(FIL));
    if (FR_OK != f_stat(pathname, pfi) || FR_OK != f_open(pf, pathname, FA_READ)) {
        vPortFree(pf);
        vPortFree(pfi);
        return false;
    }
    if (flash_list) list_cleanup(flash_list);
    uint32_t expected_to_write_size = pfi->fsize >> 1;
    vPortFree(pfi);
    UF2_Block_t* uf2 = (UF2_Block_t*)pvPortMalloc(sizeof(UF2_Block_t));
    char* alloc = (char*)pvPortCalloc(1, FLASH_SECTOR_SIZE + 511);
    char* buffer = (char*)((uint32_t)(alloc + 511) & 0xFFFFFE00); // align 512

    uint32_t flash_target_offset = 0;
    uint32_t already_written = 0;
    while(true) {
        size_t sz = 0;
        uint32_t next_flash_target_offset = read_flash_block(pf, buffer, flash_target_offset, uf2, &sz);
        if (next_flash_target_offset == flash_target_offset) {
            break;
        }
        if (sz == 0) { // replace target
            flash_target_offset = next_flash_target_offset; 
            fgoutf(get_stdout(), "Replace targe offset: %ph\n", flash_target_offset);
            continue;
        }
        already_written += FLASH_SECTOR_SIZE;
        uint32_t pcts = already_written * 100 / expected_to_write_size;
        if (pcts > 100) pcts = 100;
        if (flash_target_offset < (FIRMWARE_OFFSET << 10)) {
            fgoutf(get_stdout(), "Unexpected offset: %ph (%d%%). Breaking the process...\n", flash_target_offset, pcts);
            goto err;
        }
        fgoutf(get_stdout(), "Erase and write to flash, offset: %ph (%d%%)\n", flash_target_offset, pcts);
        flash_block(buffer, flash_target_offset);
        flash_target_offset = next_flash_target_offset;
    }
    vPortFree(alloc);
    f_close(pf);
    goutf("Write FIRMWARE MARKER '%s' to '%s'\n", pathname, FIRMWARE_MARKER_FN);
    link_firmware(pf, pathname);
    goutf("Reboot is required!\n");
    reboot_is_requested = true;
    while(true) ;
err:
    vPortFree(alloc);
    f_close(pf);
    vPortFree(pf);
    vPortFree(uf2);
    return false;
}

bool __in_hfa() load_firmware(char* pathname) {
    if (flash_list) delete_list(flash_list);
    FILINFO* pfileinfo = pvPortMalloc(sizeof(FILINFO));
    f_stat(pathname, pfileinfo);
    size_t sz = (size_t)((pfileinfo->fsize >> 1) & 0xFFFFFFFF);
    size_t max = (flash_size - (128l << 10));
    if (max < sz) { // TODO: free, ...
        goutf("ERROR: Firmware too large: %dK (%dK max) Canceled!\n", (sz >> 10), (max >> 10));
        vPortFree(pfileinfo);
        return false;
    }
    vPortFree(pfileinfo);
    goutf("Loading firmware: '%s'\n", pathname);
    return load_firmware_sram(pathname);
}

void __in_hfa() vAppTask(void *pv) {
    int res = ((boota_ptr_t)M_OS_APP_TABLE_BASE[0])(pv); // TODO: 0 - 2nd page, what exactly page used by app?
    // goutf("RET_CODE: %d\n", res);
    vTaskDelete( NULL );
    // TODO: ?? return res;
}

void __in_hfa() run_app(char * name) {
    xTaskCreate(vAppTask, name, 2048, NULL, configMAX_PRIORITIES - 1, NULL);
}

// Декодирование и обновление инструкции BL для ссылки типа R_ARM_THM_PC22
// Функция для разрешения ссылки типа R_ARM_THM_PC22
void __in_hfa() resolve_thm_pc22(uint16_t* addr, uint16_t* addr_ref, uint32_t sym_val) {
    uint16_t instr0 = *addr;
    uint16_t instr = *(addr + 1);
    // Декодирование текущего смещения
    uint32_t S = (instr0 >> 10) & 1;
    uint32_t J1 = (instr >> 13) & 1;
    uint32_t J2 = (instr >> 11) & 1;
    uint32_t imm10 = instr0 & 0x03FF;
    uint32_t imm11 = instr & 0x07FF;

    uint32_t I1 = (~(J1 ^ S) & 1);
    uint32_t I2 = (~(J2 ^ S) & 1);
    uint32_t offset = (I1 << 23) | (I2 << 22) | (imm10 << 12) | (imm11 << 1);
    if (S) {
        offset |= 0xFF800000; // знак расширение
    }

    // Вычисление нового смещения
    uint32_t new_offset = (uint32_t)((int32_t)offset + (int32_t)sym_val - (int32_t)addr_ref);
    S = new_offset >> 31;
    I1 = (new_offset >> 23) & 1;
    I2 = (new_offset >> 22) & 1;
    imm10 = (new_offset >> 12) & 0x03FF;
    imm11 = (new_offset >> 1) & 0x07FF;

    J1 = (~(I1 ^ S) & 1);
    J2 = (~(I2 ^ S) & 1);
    //goutf("ov: %ph off: %ph noff: %ph -> %d:%d:%d:%x:%x", *addr, offset, new_offset, S, J1, J2, imm10, imm11);

    // Обновление инструкции
    *addr++ = 0xF000 | (S << 10) | imm10;
    *addr = (0b11010 << 11) | (J1 << 13) | (J2 << 11) | imm11;
    //goutf("%04X %04X -> %04X %04X [%p]\n" , instr0, instr, *(addr-1), *addr, addr - 1);
}

// Разрешение ссылки типа R_ARM_THM_JUMP24 (B.W в Thumb-2)
void __in_hfa() resolve_thm_jump24(uint16_t* addr, uint16_t* addr_ref, uint32_t sym_val) {
    uint16_t instr0 = *addr;
    uint16_t instr1 = *(addr + 1);

    // Декодирование текущего смещения
    uint32_t S     = (instr0 >> 10) & 1;
    uint32_t J1    = (instr1 >> 13) & 1;
    uint32_t J2    = (instr1 >> 11) & 1;
    uint32_t imm10 = instr0 & 0x03FF;
    uint32_t imm11 = instr1 & 0x07FF;

    uint32_t I1 = (~(J1 ^ S)) & 1;
    uint32_t I2 = (~(J2 ^ S)) & 1;

    uint32_t offset = (S << 24) | (I1 << 23) | (I2 << 22) |
                      (imm10 << 12) | (imm11 << 1);
    // Знаковое расширение
    if (S) {
        offset |= 0xFF000000;
    }

    // Вычисление нового смещения
    int32_t rel = (int32_t)offset + (int32_t)sym_val - (int32_t)addr_ref;

    S     = (rel >> 24) & 1;
    I1    = (rel >> 23) & 1;
    I2    = (rel >> 22) & 1;
    imm10 = (rel >> 12) & 0x03FF;
    imm11 = (rel >> 1) & 0x07FF;

    J1 = (~(I1 ^ S)) & 1;
    J2 = (~(I2 ^ S)) & 1;

    // Обновление инструкции B.W
    *addr++ = 0xF000 | (S << 10) | imm10;
    *addr   = (0b11100 << 11) | (J1 << 13) | (J2 << 11) | imm11;
}

// вставить где-то рядом с resolve_thm_pc22/resolve_thm_jump24
// value -- (A + S) — итоговое 32-битное значение, которое нужно вставить (MOVW/MOVT берут 16 бит)
void __in_hfa() resolve_thm_alu_abs_g0_nc(uint16_t* addr, uint32_t value) {
    // addr указывает на первые два halfword'а 32-bit thumb инструкции (little-endian)
    uint16_t instr0 = addr[0];
    uint16_t instr1 = addr[1];

    // Выделяем imm части по формату imm4:i:imm3:imm8 -> K = imm4<<12 | i<<11 | imm3<<8 | imm8
    // извлечь существующий imm (addend A в инструкции)
    uint32_t imm4 = instr0 & 0x000F;             // bits [3:0] of first halfword
    uint32_t i_bit = (instr0 >> 10) & 0x1;       // 'i' bit commonly at bit10 of instr0
    uint32_t imm3 = (instr1 >> 12) & 0x7;        // bits [14:12] of second halfword
    uint32_t imm8 = instr1 & 0x00FF;             // bits [7:0] of second halfword

    uint32_t addend = (imm4 << 12) | (i_bit << 11) | (imm3 << 8) | imm8;
    // В спецификации для MOVW/MOVT addend интерпретируется как 16-bit signed в ряде случаев;
    // но для ABS (MOVW/MOVT) обычно рассматриваем как 16-bit unsigned/appropriate.
    // Мы формируем новое imm16 из (value & 0xFFFF).
    uint32_t new_imm16 = value & 0xFFFF;

    // Разложить new_imm16 обратно в поля
    uint32_t new_imm4 = (new_imm16 >> 12) & 0xF;
    uint32_t new_i   = (new_imm16 >> 11) & 0x1;
    uint32_t new_imm3= (new_imm16 >> 8) & 0x7;
    uint32_t new_imm8= new_imm16 & 0xFF;

    // Теперь нужно быть уверенным, что инcтрукция действительно MOVW/MOVT (T3)
    // Проверяем шаблон первых битов (32-bit Thumb инструкции имеют первые полусловa с 11110/11111)
    // и базовую опкодовую маску для MOVW/MOVT (приближённая проверка):
    //
    // MOVW T3 has top halfword bits: 11110 0 10 0 ... (примерно 0xF2? / 0xF240)
    // MOVT T3 has top halfword bits: 11110 0 10 1 ... (примерно 0xF2C0)
    //
    // Мы делаем негрубую проверку маской; если не совпадает — логируем и вернёмся.
    uint16_t hi5 = instr0 >> 11; // top 5 bits
    if (!(hi5 == 0x1F || hi5 == 0x1E)) {
        // не похоже на 32-bit thumb первую половину; это неожиданный случай
        goutf("WARN: resolve_thm_alu_abs_g0_nc: unexpected thumb first halfword: %04X %04X\n", instr0, instr1);
        return;
    }

    // Более точное различение MOVW/MOVT: смотреть биты опкода в instr0/instr1.
    // Упрощённая маска: проверим, что instr0 имеет формат 11110x10xx...... (типично для MOVW/MOVT)
    if (((instr0 & 0xF800) != 0xF200) && ((instr0 & 0xF800) != 0xF000) && ((instr0 & 0xF800) != 0xF000)) {
        // не уверены, что это MOVW/MOVT — но пытаемся anyway (логи).
        // это лишь предупреждение — реальные проверки можно расширить по нужде.
        // goutf("NOTE: resolve_thm_alu_abs_g0_nc: operand doesn't match MOVW/MOVT mask: %04X\n", instr0);
    }

    // Перезаписать поля imm
    // Удаляем старые поля и вставляем новые — аккуратно с остальными битами в полусловах.
    // instr0: сохраняем все биты кроме imm4 (bits [3:0]) и i (bit10)
    uint16_t new_instr0 = instr0;
    new_instr0 &= ~(0x000F);                // clear imm4 (bits 3:0)
    new_instr0 |= (uint16_t)(new_imm4 & 0xF);
    new_instr0 &= ~(1u << 10);              // clear i bit
    new_instr0 |= (uint16_t)((new_i & 0x1) << 10);

    // instr1: clear imm3 (bits [14:12]) and imm8 (bits [7:0])
    uint16_t new_instr1 = instr1;
    new_instr1 &= ~(0x7000);                // clear imm3 (bits 14..12)
    new_instr1 |= (uint16_t)((new_imm3 & 0x7) << 12);
    new_instr1 &= ~(0x00FF);                // clear imm8
    new_instr1 |= (uint16_t)(new_imm8 & 0xFF);

    // Записать обратно
    addr[0] = new_instr0;
    addr[1] = new_instr1;

    //Dbg:
    #if DEBUG_APP_LOAD
    goutf("Resolved MOVW/MOVT: old_addend=%04X new_imm16=%04X -> %04X %04X\n",
          addend & 0xFFFF, new_imm16, addr[0], addr[1]);
    #endif
}

static const char* __in_hfa() st_predef(const char* v) {
    if(strlen(v) == 2) {
        if (v[0] == '$' && v[1] == 't') {
            return "$t (Thumb)";
        }
        if (strcmp(v, "$d") == 0) {
            return "$d (data)";
        }
    }
    return v;
}

typedef struct {
    FIL *f2;
    elf32_header *pehdr;
    int symtab_off;
    elf32_sym* psym;
    char* pstrtab;
    list_t* /*sect_entry_t*/ sections_lst;
} load_sec_ctx;

static load_sec_ctx* libc_pctx = 0;
static hash_table_t* libc_idx = 0;

static char* __in_hfa() sec_prg_addr(const load_sec_ctx* ctx, int sec_num) {
    if (ctx == libc_pctx) {
        sec_num = -1-sec_num; // use nagative numbers for libc (to do not intersect)
    }
    node_t* n = ctx->sections_lst->first;
    while (n) {
        sect_entry_t* se = (sect_entry_t*)n->data;
        if (se->sec_num == sec_num) {
            return se->prg_addr;
        }
        n = n->next;
    }
    return 0;
}

static void __in_hfa() add_sec(load_sec_ctx* ctx, char* del_addr, char* prg_addr, int num) {
    sect_entry_t* se = (sect_entry_t*)pvPortMalloc(sizeof(sect_entry_t));
    if (!se) return; // TODO: message with fault
    // goutf("sec: [%p]\n", se);
    se->del_addr = del_addr;
    se->prg_addr = prg_addr;
    se->sec_num = ctx == libc_pctx ? -1-num : num; // use nagative numbers for libc (to do not intersect)
    list_push_back(ctx->sections_lst, se);
}

inline static uint8_t* __in_hfa() sec_align(uint32_t sz, uint8_t* *pdel_addr, uint8_t* *real_addr, uint32_t a, bool write_access) {
    uint8_t* res = (uint8_t*)pvPortMalloc(sz);
    if (a == 0 || a == 1) {
        *pdel_addr = res;
    }
    else if ((uint32_t)res & (a - 1)) {
        vPortFree(res);
        res = (uint8_t*)pvPortMalloc(sz + (a - 1));
        *pdel_addr = res;
        if ((uint32_t)res & (a - 1)) {
            res = (uint8_t*)(((uint32_t)res & (0xFFFFFFFF ^ (a - 1))) + a);
        }
    } else {
        *pdel_addr = res;
    }
    // goutf("4del: [%p]\n", *pdel_addr);
    *real_addr = res;
    if (!write_access) {
        res = flash_addr;
        flash_addr += sz;
        if ((uint32_t)res & (a - 1)) {
            flash_addr += (a - 1);
            res = (uint8_t*)(((uint32_t)res & (0xFFFFFFFF ^ (a - 1))) + a);
        }
        // goutf("res: %ph\n" , res);
    }
    return res;
}

static const char* __in_hfa() st_spec_sec(uint16_t st) {
    if (st >= 0xff00 && st <= 0xff1f)
        return "PROC";
    switch (st)
    {
    case 0xffff:
        return "HIRESERVE";
    case 0xfff2:
        return "COMMON";
    case 0xfff1:
        return "ABS";
    case 0:
        return "UNDEF";
    default:
        break;
    }
    return 0;
}

static const char* s = "Unexpected ELF file";

bool __in_hfa() is_new_app(cmd_ctx_t* ctx) {
    if (!ctx->orig_cmd) {
        gouta("Unable to open file: NULL\n");
        return false;
    }
    FIL* f = (FIL*)pvPortMalloc(sizeof(FIL));
    if (f_open(f, ctx->orig_cmd, FA_READ) != FR_OK) {
        vPortFree(f);
        goutf("Unable to open file: '%s'\n", ctx->orig_cmd);
        return false;
    }
    struct elf32_header ehdr;
    UINT rb;
    if (f_read(f, &ehdr, sizeof(ehdr), &rb) != FR_OK) {
        f_close(f);
        vPortFree(f);
        goutf("Unable to read an ELF file header: '%s'\n", ctx->orig_cmd);
        return false;
    }
    f_close(f);
    vPortFree(f);
    if (ehdr.common.magic != ELF_MAGIC) {
        goutf("It is not an ELF file: '%s'\n", ctx->orig_cmd);
        return false;
    }
    if (ehdr.common.version != 1 || ehdr.common.version2 != 1) {
        goutf("%s '%s' version: %d:%d\n", s, ctx->orig_cmd, ehdr.common.version, ehdr.common.version2);
        return false;
    }
    if (ehdr.common.arch_class != 1 || ehdr.common.endianness != 1) {
        goutf("%s '%s' class: %d; endian: %d\n", s, ctx->orig_cmd, ehdr.common.arch_class, ehdr.common.endianness);
        return false;
    }
    if (ehdr.common.machine != EM_ARM) {
        goutf("%s '%s' machine type: %d; expected: %d\n", s, ctx->orig_cmd, ehdr.common.machine, EM_ARM);
        return false;
    }
    if (ehdr.common.abi != 0) {
        goutf("%s '%s' ABI type: %d; expected: %d\n", s, ctx->orig_cmd, ehdr.common.abi, 0);
        return false;
    }
    if (ehdr.flags & EF_ARM_ABI_FLOAT_HARD) {
        goutf("%s '%s' EF_ARM_ABI_FLOAT_HARD: %04Xh\n", s, ctx->orig_cmd, ehdr.flags);
        return false;
    }
    ctx->stage = VALID;
    if (ctx->next) {
        if (!is_new_app(ctx->next)) return false;
    }
    return true;
}

void __in_hfa() cleanup_bootb_ctx(cmd_ctx_t* ctx) {
    // goutf("cleanup_bootb_ctx [%p]\n", ctx);
    bootb_ctx_t* bootb_ctx = ctx->pboot_ctx;
    if (!bootb_ctx) return;
    if (bootb_ctx->sections) {
        if (flash_list) {
            uint32_t min_addr = 0xFFFFFFFF;
            uint32_t max_addr = 0;
            node_t* n = bootb_ctx->sections->first;
            while (n) {
                sect_entry_t* se = (sect_entry_t*)n->data;
                uint32_t prg_addr = se->prg_addr;
                if (prg_addr <= 0x20000000) {
                    if ( min_addr > prg_addr ) min_addr = prg_addr;
                    if ( max_addr < prg_addr ) max_addr = prg_addr;
                }
                n = n->next;
            }
            flash_addr = M_OS_APP_TABLE_BASE;
            // goutf("flash_addr [%p]\n", flash_addr);
            node_t* fn = flash_list->first;
            while (fn) {
                uint32_t nn = fn->next;
                to_flash_rec_t* fse = (to_flash_rec_t*)fn->data;
                uint32_t faddr = fse->offset + XIP_BASE;
                if (faddr >= min_addr && faddr <= max_addr) {
                    // goutf("Free [%p]\n", faddr);
                    list_erase_node(flash_list, fn);
                }
                else {
                    faddr += fse->size;
                    if (flash_addr < faddr) {
                        flash_addr = faddr;
                        // goutf("flash_addr [%p]\n", flash_addr);
                    }
                }
                fn = nn;
            }
        }
        delete_list(bootb_ctx->sections);
        bootb_ctx->sections = 0;
    }
    vPortFree(bootb_ctx);
    ctx->pboot_ctx = 0;
    // gouta("cleanup_bootb_ctx <<\n");
}

bool __in_hfa() run_new_app(cmd_ctx_t* ctx) {
    // todo:
    gouta("tba\n");
    return false;
}

static uint32_t load_sec2mem_wrapper(load_sec_ctx* pctx, uint32_t req_idx, bool try_to_use_flash);

static uint8_t* __in_hfa() load_sec2mem(load_sec_ctx * c, uint16_t sec_num, bool try_to_use_flash) {
    uint8_t* prg_addr = sec_prg_addr(c, sec_num);
    if (prg_addr != 0) {
        // goutf("Section #%d already in mem @ %p\n", sec_num, prg_addr);
        return prg_addr;
    }
    size_t prev_flash_addr = flash_addr;
    size_t new_flash_addr = flash_addr;
    UINT rb;
    uint8_t* real_ram_addr = 0;
    uint8_t* del_addr = 0;
    elf32_shdr* psh = (elf32_shdr*)pvPortMalloc(sizeof(elf32_shdr));
    if (!psh) {
        gouta("Not enough RAM\n");
        return prg_addr;
    }
    if (f_lseek(c->f2, c->pehdr->sh_offset + sizeof(elf32_shdr) * sec_num) == FR_OK &&
        f_read(c->f2, psh, sizeof(elf32_shdr), &rb) == FR_OK && rb == sizeof(elf32_shdr)
    ) {
        // goutf("free_sz: %d; psh->sh_size: %d\n", free_sz, psh->sh_size);
        if (psh->sh_size + psh->sh_addralign + RESERVED_RAM > xPortGetFreeHeapSize()) {
            gouta("Not enough RAM.\n");
            goto e1;
        }
        bool write_access = try_to_use_flash ? (psh->sh_flags & 1) : true; // required to provide write access
        prg_addr = sec_align(psh->sh_size, &del_addr, &real_ram_addr, psh->sh_addralign, write_access);
        new_flash_addr = flash_addr;
        bool alloc_enough = psh->sh_flags & SHF_ALLOC; // file may not contain such section, just allocation is enough
        if (psh->sh_type == SHT_NOBITS) {
            memset(real_ram_addr, 0, psh->sh_size);
        } else {
            FRESULT r = f_lseek(c->f2, psh->sh_offset);
            if (!alloc_enough && r != FR_OK) {
                goutf("f_lseek->[%d] failed: %d\n", psh->sh_offset, r);
                goto e1;
            }
            r = f_read(c->f2, real_ram_addr, psh->sh_size, &rb);
            if (!alloc_enough && r != FR_OK) {
                goutf("f_read->[%p](%d) failed: %d (sz: %d)\n", real_ram_addr, psh->sh_size, r, rb);
                goto e1;
            }
            if (!alloc_enough && rb != psh->sh_size) {
                goutf("f_read->[%p](%d) passed: %d but sz: %d\n", real_ram_addr, psh->sh_size, r, rb);
                goto e1;
            }
        }
        #if DEBUG_APP_LOAD
        goutf("Program section #%d (%d bytes) allocated into %ph\n", sec_num, psh->sh_size, prg_addr);
        #endif
        add_sec(c, del_addr, prg_addr, sec_num);
        // links and relocations
        if (f_lseek(c->f2, c->pehdr->sh_offset) != FR_OK) {
            goutf("Unable to locate sections @ %ph\n", c->pehdr->sh_offset);
            goto e1;
        }
        while (f_read(c->f2, psh, sizeof(elf32_shdr), &rb) == FR_OK && rb == sizeof(elf32_shdr)) {
            // goutf("Section info: %d type: %d\n", psh->sh_info, psh->sh_type);
            if (psh->sh_type == REL_SEC && psh->sh_info == sec_num) {
                uint32_t r2 = f_tell(c->f2);
                elf32_rel rel;
                for (uint32_t j = 0; j < psh->sh_size / sizeof(rel); ++j) {
                    if (f_lseek(c->f2, psh->sh_offset + j * sizeof(rel)) != FR_OK ||
                        f_read(c->f2, &rel, sizeof(rel), &rb) != FR_OK || rb != sizeof(rel)
                    ) {
                        goutf("Unable to read REL record #%d in section #%d\n", j, psh->sh_info);
                        goto e1;
                    }
                    uint32_t rel_sym = rel.rel_info >> 8;
                    uint8_t rel_type = rel.rel_info & 0xFF;
                    // goutf("rel_offset: %p; rel_sym: %d; rel_type: %d\n", rel.rel_offset, rel_sym, rel_type);
                    if (f_lseek(c->f2, c->symtab_off + rel_sym * sizeof(elf32_sym)) != FR_OK ||
                        f_read(c->f2, c->psym, sizeof(elf32_sym), &rb) != FR_OK || rb != sizeof(elf32_sym)
                    ) {
                        goutf("Unable to read .symtab section #%d\n", rel_sym);
                        goto e1;
                    }
                    char* rel_str_sym = st_spec_sec(c->psym->st_shndx);
                    if (rel_str_sym != 0) {
                        char* fn_name = c->pstrtab + c->psym->st_name;
                        #if 0
                        if (c != libc_pctx) { // load from libc
                            // goutf("load from libc: %s\n", fn_name);
                            uint32_t libc_req_idx = 0;
                            if (!hash_table_get(libc_idx, fn_name, &libc_req_idx)) {
                                goutf("Unsupported link from STRTAB record #%d to section #%d (%s): %s (and not libc fn)\n",
                                    rel_sym, c->psym->st_shndx, rel_str_sym,
                                    st_predef(fn_name)
                                );
                            } else {
                                // адрес функции в libc (A + S уже учтены в load_sec2mem_wrapper)
                                uint32_t libc_target = load_sec2mem_wrapper(libc_pctx, libc_req_idx, try_to_use_flash);
                                if (!libc_target) {
                                    goutf("Unable to load link from STRTAB record #%d to section #%d (%s): %s from the libc\n",
                                        rel_sym, c->psym->st_shndx, rel_str_sym,
                                        st_predef(fn_name)
                                    );
                                } else {
                                    // указатель в загруженном приложении (где нужно применить релокацию)
                                    uint32_t* reloc_in_app = (uint32_t*)(real_ram_addr + rel.rel_offset);
                                    // указатель на эталонную инструкцию/место (используется для PC-relative resolver'ов)
                                    uint32_t* reloc_ref = (uint32_t*)(prg_addr + rel.rel_offset);
                                    switch (rel_type) {
                                        case 2: // R_ARM_ABS32: P <- S + A (+ existing addend)
                                        {
                                            uint32_t addend = *reloc_in_app;    // A — addend, уже записанный в месте релокации
                                            *reloc_in_app = addend + libc_target;
                                            break;
                                        }
                                        case 10: // R_ARM_THM_PC22 (Thumb BL/BLX(1) wide)
                                            // sym_val должен быть абсолютным адресом цели (A+S). resolve_thm_pc22 выполнит нужное кодирование.
                                            resolve_thm_pc22((uint16_t*)reloc_in_app, (uint16_t*)reloc_ref, libc_target);
                                            break;
                                        default:
                                            goutf("WARN: Unsupported libc REL type %d -> symbol: %s\n",
                                                    rel_type, c->pstrtab + c->psym->st_name);
                                            break;
                                    }
                                    continue;
                                }
                            }
                        } else { // it is already libc
                         #endif
                            goutf("[libc] Unsupported link from STRTAB record #%d to section #%d (%s): %s\n",
                                    rel_sym, c->psym->st_shndx, rel_str_sym,
                                    st_predef(fn_name)
                            );
                        ///}
                        goto e1;
                    }
                    uint32_t* rel_addr_real = (uint32_t*)(real_ram_addr + rel.rel_offset /*10*/); /*f7ff fffe 	bl	0*/
                    uint32_t* rel_addr_ref  = (uint32_t*)(prg_addr      + rel.rel_offset /*10*/); /*f7ff fffe 	bl	0*/
                    // DO NOT resolve it for any case, it may be 16-bit alligned, and will hang to load 32-bit
                    //uint32_t P = *rel_addr; /*f7ff fffe 	bl	0*/
                    uint32_t S = c->psym->st_value;
                    uint8_t* sec_addr_ref  = prg_addr;
                    // goutf("rel_offset: %p; rel_sym: %d; rel_type: %d -> %d\n", rel.rel_offset, rel_sym, rel_type, c->psym->st_shndx);
                    if (c->psym->st_shndx != sec_num) {
                        sec_addr_ref = load_sec2mem(c, c->psym->st_shndx, try_to_use_flash);
                        if (sec_addr_ref == 0) {
                            goto e1;
                        }
                    }
                    uint32_t A = sec_addr_ref;
                    // Разрешение ссылки
                    switch (rel_type) {
                        case 2: //R_ARM_ABS32:
                            // goutf("rel_type: %d; *rel_addr += A: %ph + S: %ph\n", rel_type, A, S);
                            *rel_addr_real += S + A;
                            break;
                        //case 3: //R_ARM_REL32:
                            //*rel_addr_real = S - P + A; // todo: signed?
                            // break;
                        case 10: //R_ARM_THM_PC22:
                            resolve_thm_pc22(rel_addr_real, rel_addr_ref, A + S);
                            break;
                            /*
                        case 30: // R_ARM_THM_JUMP24
                            gouta("WARN: Untested REL type: R_ARM_THM_JUMP24\n");
                            resolve_thm_jump24(rel_addr_real, rel_addr_ref, A + S);
                            break;
                        case 102: // R_ARM_THM_ALU_ABS_G0_NC
                            gouta("WARN: Untested REL type: R_ARM_THM_ALU_ABS_G0_NC\n");
                            if (((uintptr_t)rel_addr_real & 0x1) || ((uintptr_t)rel_addr_real & 0x2)) {
                                goutf("WARN: REL type 102 misaligned addr: %p\n", rel_addr_real);
                                goutf("REL type %d -> symbol: %s\n", rel_type, c->pstrtab + c->psym->st_name);
                                break;
                            }
                            resolve_thm_alu_abs_g0_nc((uint16_t*)rel_addr_real, S + A);
                            break;*/
                        default:
                            goutf("WARN: Unsupported REL type %d -> symbol: %s\n", rel_type, c->pstrtab + c->psym->st_name);
                            goto e1;
                    }
                    //goutf("= %ph\n", *rel_addr);
                }
                f_lseek(c->f2, r2);
            }
        }
        #if DEBUG_APP_LOAD
        goutf("Section #%d - load completed @%ph\n", sec_num, prg_addr);
        #endif
    } else {
        goutf("Unable to read section #%d info\n", sec_num);
        goto e1;
    }
    goto e2;
e1:
    prg_addr = 0;
    flash_addr = prev_flash_addr;
    vTaskDelay(3000);
e2:
    vPortFree(psh);
    size_t sz = new_flash_addr - prev_flash_addr;
    if (prg_addr && sz) {
        to_flash_rec_t* o = (to_flash_rec_t*)pvPortMalloc(sizeof(to_flash_rec_t));
        o->offset = prg_addr - XIP_BASE; // TODO: out of empty flash?
        o->size = sz;
        char* tmp = get_ctx_var(get_cmd_startup_ctx(), TEMP);
        if(!tmp) tmp = "";
        size_t cdl = strlen(tmp);
        char * flash_me_file = concat(tmp, _flash_me);
        FIL* f = (FIL*)pvPortMalloc(sizeof(FIL));
        if ( FR_OK != f_open(f, flash_me_file, FA_WRITE | FA_CREATE_ALWAYS) ) {
            goutf("Unable to open file '%s'\n", flash_me_file);
            goto e5;
        }
        size_t target_offset = prg_addr - (size_t)M_OS_APP_TABLE_BASE;
        if ( FR_OK != f_lseek(f, target_offset) ) {
            goutf("Unable to seek file '%s' to %d\n", flash_me_file, target_offset);
            goto e4;
        }
        UINT bw;
        if ( FR_OK != f_write(f, real_ram_addr, sz, &bw) || sz != bw ) {
            goutf("Unable to write %d bytes to file '%s'\n", sz, flash_me_file);
            goto e4;
        }
        // goutf("Write %Xh bytes passed to '%s'; target offset: %ph\n", bw, flash_me_file, target_offset);
    e4:
        f_close(f);
    e5:
        vPortFree(flash_me_file);
        vPortFree(f);
        list_push_back(lst, o);
        vPortFree(del_addr);
        del_addr = 0;
        if (flash_addr & 0x00000FFF) {
            flash_addr &= 0xFFFFF000;
            flash_addr += 4096;
        }
    }
    if (!prg_addr && del_addr) {
        vPortFree(del_addr);
    }
    return prg_addr;
}

static uint32_t __in_hfa() load_sec2mem_wrapper(load_sec_ctx* pctx, uint32_t req_idx, bool try_to_use_flash) {
    if (!pctx || !pctx->f2) return 0;
    if (req_idx != 0xFFFFFFFF) {
        #if DEBUG_APP_LOAD
        goutf("Loading .symtab section #%d\n", req_idx);
        #endif
        UINT rb;
        elf32_sym* psym = pvPortMalloc(sizeof(elf32_sym));
        if (!psym) {
            gouta("Not enough RAM\n");
            goto e3;
        }
        if (f_lseek(pctx->f2, pctx->symtab_off + req_idx * sizeof(elf32_sym)) != FR_OK ||
            f_read(pctx->f2, psym, sizeof(elf32_sym), &rb) != FR_OK || rb != sizeof(elf32_sym)
        ) {
            goutf("Unable to read .symtab section #%d\n", req_idx);
            vPortFree(psym);
            goto e3;
        }
        uint16_t st_shndx = psym->st_shndx;
        uint32_t st_value = psym->st_value;
        vPortFree(psym);
        uint8_t* t = load_sec2mem(pctx, st_shndx, try_to_use_flash);
        if (!t) return 0;
        // debug_sections(pctx->psections_list);
        return (uint32_t)(t + st_value);
    }
e3:
    return 0;
}

static bool __in_hfa() pre_load_libc(cmd_ctx_t* ctx) {
    if (libc_idx == 0) {
        FIL* f = (FIL*)pvPortMalloc(sizeof(FIL));
        if (!f) {
    a:
            gouta("[libc] Not enough RAM\n");
            ctx->ret_code = -1;
            return false;
        }
        const char* fn = "/mos2/libc"; // TODO: var
        if (f_open(f, fn, FA_READ) != FR_OK) {
            vPortFree(f);
            goutf("[libc] Unable to open file: '%s'\n", fn);
            ctx->ret_code = -1;
            return false;
        }
        bool try_to_use_flash = ctx->forse_flash;
        elf32_header* pehdr = (elf32_header*)pvPortMalloc(sizeof(elf32_header));
        if (!pehdr) {
    a1:
            f_close(f);
            vPortFree(f);
            goto a;
        }
        UINT rb;
        if (f_read(f, pehdr, sizeof(elf32_header), &rb) != FR_OK) {
            goutf("[libc] Unable to read an ELF file header: '%s'\n", fn);
            goto e1;
        }
        elf32_shdr* psh = (elf32_shdr*)pvPortMalloc(sizeof(elf32_shdr));
        if (!psh) {
    a2:
            vPortFree(pehdr);
            goto a1;
        }
        bool ok = f_lseek(f, pehdr->sh_offset + sizeof(elf32_shdr) * pehdr->sh_str_index) == FR_OK;
        if (!ok || f_read(f, psh, sizeof(elf32_shdr), &rb) != FR_OK || rb != sizeof(elf32_shdr)) {
            goutf("[libc] Unable to read .shstrtab section header @ %d+%d (read: %d)\n", f_tell(f), sizeof(elf32_shdr), rb);
            goto e11;
        }
        char* symtab = (char*)pvPortMalloc(psh->sh_size);
        if (!symtab) {
    a3:
            vPortFree(psh);
            goto a2;
        }
        ok = f_lseek(f, psh->sh_offset) == FR_OK;
        if (!ok || f_read(f, symtab, psh->sh_size, &rb) != FR_OK || rb != psh->sh_size) {
            goutf("[libc] Unable to read .shstrtab section @ %d+%d (read: %d)\n", f_tell(f), psh->sh_size, rb);
            goto e2;
        }
        f_lseek(f, pehdr->sh_offset);
        int symtab_off = -1;
        int strtab_off = -1;
        UINT symtab_len, strtab_len = 0;
        while ((symtab_off < 0 || strtab_off < 0) && f_read(f, psh, sizeof(elf32_shdr), &rb) == FR_OK && rb == sizeof(elf32_shdr)) { 
            if(psh->sh_type == 2 && 0 == strcmp(symtab + psh->sh_name, ".symtab")) {
                symtab_off = psh->sh_offset;
                symtab_len = psh->sh_size;
            }
            if(psh->sh_type == 3 && 0 == strcmp(symtab + psh->sh_name, ".strtab")) {
                strtab_off = psh->sh_offset;
                strtab_len = psh->sh_size;
            }
        }
        if (symtab_off < 0 || strtab_off < 0) {
            goutf("[libc] Unable to find .strtab/.symtab sections\n");
            goto e2;
        }
        f_lseek(f, strtab_off);
        char* strtab = (char*)pvPortMalloc(strtab_len);
        if (!strtab) {
    a4:
            vPortFree(symtab);
            goto a3;
        }
        if (f_read(f, strtab, strtab_len, &rb) != FR_OK || rb != strtab_len) {
            goutf("[libc] Unable to read .strtab section\n");
            goto e3;
        }
        f_lseek(f, symtab_off);
        elf32_sym* psym = pvPortMalloc(sizeof(elf32_sym));;
        if (!psym) {
    a5:
            vPortFree(strtab);
            goto a4;
        }
        libc_idx = hash_table_create(64);
        for (uint32_t i = 0; i < symtab_len / sizeof(elf32_sym); ++i) {
            if (f_read(f, psym, sizeof(elf32_sym), &rb) != FR_OK || rb != sizeof(elf32_sym)) {
                goutf("Unable to read .symtab section #%d\n", i);
                break;
            }
            char* gfn = strtab + psym->st_name;
            hash_table_put(libc_idx, gfn, i);
        }
        libc_pctx = (load_sec_ctx*)pvPortMalloc(sizeof(load_sec_ctx));
        if (!libc_pctx) {
    a6:
            vPortFree(psym);
            goto a5;
        }
        libc_pctx->f2 = f;
        libc_pctx->pehdr = pehdr;
        libc_pctx->symtab_off = symtab_off;
        libc_pctx->psym = psym;
        libc_pctx->pstrtab = strtab;
        libc_pctx->sections_lst = 0;
        vPortFree(symtab);
        vPortFree(psh);
        return true;
    e8:
        vPortFree(psym);
    e3:
        vPortFree(strtab);
    e2:
        vPortFree(symtab);
    e11:
        vPortFree(psh);
    e1:
        vPortFree(pehdr);
        f_close(f);
        vPortFree(f);
        return false;
    }
    return true;
}

bool __in_hfa() load_app(cmd_ctx_t* ctx) {
    if (!ctx->orig_cmd) {
        gouta("Unable to load file: NULL\n");
        return false;
    }
    char * fn = ctx->orig_cmd;
    cleanup_bootb_ctx(ctx);
    ctx->pboot_ctx = (bootb_ctx_t*)pvPortMalloc(sizeof(bootb_ctx_t));
    if (!ctx->pboot_ctx) {
a:
        gouta("Not enough RAM\n");
        ctx->ret_code = -1;
        return false;
    }
    bootb_ctx_t* bootb_ctx = ctx->pboot_ctx;
    memset(bootb_ctx, 0, sizeof(bootb_ctx_t)); // ensure context is empty
    FIL* f = (FIL*)pvPortMalloc(sizeof(FIL));
    if (!f) {
        goto a;
    }
    if (f_open(f, fn, FA_READ) != FR_OK) {
        vPortFree(f);
        goutf("Unable to open file: '%s'\n", fn);
        ctx->ret_code = -1;
        return false;
    }
    bool try_to_use_flash = ctx->forse_flash;
    if (!try_to_use_flash) {
        size_t free_sz = xPortGetFreeHeapSize();
        if (free_sz < (f->obj.objsize >> 1)) {
            try_to_use_flash = true;
            gouta("Attempt to use flash (by size)\n");
            goutf("     free: %d\n", free_sz);
            goutf(" required: %dK\n", (size_t)(f->obj.objsize >> 11));
        }
    } else {
        gouta("Attempt to use flash (Alt+Enter)\n");
    }
    elf32_header* pehdr = (elf32_header*)pvPortMalloc(sizeof(elf32_header));
    if (!pehdr) {
a1:
        f_close(f);
        vPortFree(f);
        goto a;
    }
    UINT rb;
    if (f_read(f, pehdr, sizeof(elf32_header), &rb) != FR_OK) {
        goutf("Unable to read an ELF file header: '%s'\n", fn);
        goto e1;
    }
    elf32_shdr* psh = (elf32_shdr*)pvPortMalloc(sizeof(elf32_shdr));
    if (!psh) {
a2:
        vPortFree(pehdr);
        goto a1;
    }
    bool ok = f_lseek(f, pehdr->sh_offset + sizeof(elf32_shdr) * pehdr->sh_str_index) == FR_OK;
    if (!ok || f_read(f, psh, sizeof(elf32_shdr), &rb) != FR_OK || rb != sizeof(elf32_shdr)) {
        goutf("Unable to read .shstrtab section header @ %d+%d (read: %d)\n", f_tell(f), sizeof(elf32_shdr), rb);
        goto e11;
    }
    char* symtab = (char*)pvPortMalloc(psh->sh_size);
    if (!symtab) {
a3:
        vPortFree(psh);
        goto a2;
    }
    ok = f_lseek(f, psh->sh_offset) == FR_OK;
    if (!ok || f_read(f, symtab, psh->sh_size, &rb) != FR_OK || rb != psh->sh_size) {
        goutf("Unable to read .shstrtab section @ %d+%d (read: %d)\n", f_tell(f), psh->sh_size, rb);
        goto e2;
    }
    f_lseek(f, pehdr->sh_offset);
    int symtab_off = -1;
    int strtab_off = -1;
    UINT symtab_len, strtab_len = 0;
    while ((symtab_off < 0 || strtab_off < 0) && f_read(f, psh, sizeof(elf32_shdr), &rb) == FR_OK && rb == sizeof(elf32_shdr)) { 
        if(psh->sh_type == 2 && 0 == strcmp(symtab + psh->sh_name, ".symtab")) {
            symtab_off = psh->sh_offset;
            symtab_len = psh->sh_size;
        }
        if(psh->sh_type == 3 && 0 == strcmp(symtab + psh->sh_name, ".strtab")) {
            strtab_off = psh->sh_offset;
            strtab_len = psh->sh_size;
        }
    }
    if (symtab_off < 0 || strtab_off < 0) {
        goutf("Unable to find .strtab/.symtab sections\n");
        goto e2;
    }
    f_lseek(f, strtab_off);
    char* strtab = (char*)pvPortMalloc(strtab_len);
    if (!strtab) {
a4:
        vPortFree(symtab);
        goto a3;
    }
    if (f_read(f, strtab, strtab_len, &rb) != FR_OK || rb != strtab_len) {
        goutf("Unable to read .strtab section\n");
        goto e3;
    }
    f_lseek(f, symtab_off);
    elf32_sym* psym = pvPortMalloc(sizeof(elf32_sym));;
    if (!psym) {
a5:
        vPortFree(strtab);
        goto a4;
    }
    uint32_t _init_idx = 0xFFFFFFFF;
    uint32_t _fini_idx = 0xFFFFFFFF;
    uint32_t main_idx = 0xFFFFFFFF;
    uint32_t req_idx = 0xFFFFFFFF;
    uint32_t sig_idx = 0xFFFFFFFF;
    uint32_t w_init_idx = 0xFFFFFFFF;
    uint32_t w_fini_idx = 0xFFFFFFFF;

    load_sec_ctx* pctx = (load_sec_ctx*)pvPortMalloc(sizeof(load_sec_ctx));
    if (!pctx) {
a6:
        vPortFree(psym);
        goto a5;
    }
    pctx->f2 = f;
    pctx->pehdr = pehdr;
    pctx->symtab_off = symtab_off;
    pctx->psym = psym;
    pctx->pstrtab = strtab;
    pctx->sections_lst = new_list_v(0, sect_entry_deallocator, 0);
///    pre_load_libc(ctx);
///    libc_pctx->sections_lst = pctx->sections_lst; // one list to load app and libc functions
    for (uint32_t i = 0; i < symtab_len / sizeof(elf32_sym); ++i) {
        if (f_read(f, psym, sizeof(elf32_sym), &rb) != FR_OK || rb != sizeof(elf32_sym)) {
            goutf("Unable to read .symtab section #%d\n", i);
            break;
        }
        if (psym->st_info == STR_TAB_GLOBAL_FUNC) {
            char* gfn = strtab + psym->st_name;
            if (0 == strcmp("_init", gfn)) {
                _init_idx = i;
            } else if (0 == strcmp("__required_m_api_verion", gfn)) {
                req_idx = i;
            } else if (0 == strcmp("_fini", gfn)) {
                _fini_idx = i;
            } else if (0 == strcmp("main", gfn)) {
                main_idx = i;
            } else if (0 == strcmp("signal", gfn)) {
                sig_idx = i;
            }
        }
        if (psym->st_info == STR_TAB_WEAK_FUNC) {
            char* gfn = strtab + psym->st_name;
            if (0 == strcmp("_init", gfn)) {
                w_init_idx = i;
            } else if (0 == strcmp("_fini", gfn)) {
                w_fini_idx = i;
            }
        }
    }
    if(try_to_use_flash && !lst) {
        lst = new_list_v(0, 0, 0);
    }
    if (_init_idx == 0xFFFFFFFF && w_init_idx != 0xFFFFFFFF) _init_idx = w_init_idx;
    if (_fini_idx == 0xFFFFFFFF && w_fini_idx != 0xFFFFFFFF) _fini_idx = w_fini_idx;
    bootb_ctx->bootb[0] = load_sec2mem_wrapper(pctx, req_idx, try_to_use_flash);
    bootb_ctx->bootb[1] = load_sec2mem_wrapper(pctx, _init_idx, try_to_use_flash);
    bootb_ctx->bootb[2] = load_sec2mem_wrapper(pctx, main_idx, try_to_use_flash);
    bootb_ctx->bootb[3] = load_sec2mem_wrapper(pctx, _fini_idx, try_to_use_flash);
    bootb_ctx->bootb[4] = load_sec2mem_wrapper(pctx, sig_idx, try_to_use_flash);
    if(try_to_use_flash) {
        node_t* n = lst->first;
        uint32_t min_addr = 0xFFFFFFFF;
        uint32_t max_addr = 0;
        while(n) {
            to_flash_rec_t* tf = (to_flash_rec_t*)n->data;
            if ( min_addr > tf->offset + XIP_BASE + (FIRMWARE_OFFSET << 10) ) min_addr = tf->offset + XIP_BASE + (FIRMWARE_OFFSET << 10);
            if ( max_addr < tf->offset + XIP_BASE + (FIRMWARE_OFFSET << 10) + tf->size ) max_addr = tf->offset + XIP_BASE + (FIRMWARE_OFFSET << 10) + tf->size;
            n = n->next;
        }
        delete_list(lst);
        lst = 0;

        goutf("Going to flash: [%p]-[%p] %dK (%d pages)\n", min_addr, max_addr, 1 + ((max_addr - min_addr) >> 10), 1 + ((max_addr - min_addr) >> 12));
        if (xPortGetFreeHeapSize() < (FLASH_SECTOR_SIZE + 511)) {
            goutf("WARN: free_sz: %d; required: %d\n", xPortGetFreeHeapSize(), (FLASH_SECTOR_SIZE + 511));
            goto e8;
        }
        char* alloc = (char*)pvPortCalloc(1, FLASH_SECTOR_SIZE + 511);
        if (!alloc) {
    a7:
            vPortFree(pctx);
            goto a6;
        }
        char* buffer = (char*)((uint32_t)(alloc + 511) & 0xFFFFFE00); // align 512
        char* tmp = get_ctx_var(get_cmd_startup_ctx(), TEMP);
        if(!tmp) tmp = "";
        size_t cdl = strlen(tmp);
        char * flash_me_file = concat(tmp, _flash_me);
        if (!flash_me_file) {
    a8:
            vPortFree(alloc);
            goto a7;
        }
        FIL* f = (FIL*)pvPortMalloc(sizeof(FIL));
        if (!f) {
            vPortFree(flash_me_file);
            goto a8;
        }
        if ( FR_OK != f_open(f, flash_me_file, FA_READ) ) {
            goutf("Unable to open file '%s'\n", flash_me_file);
            goto e5;
        }
        for (uint32_t addr = min_addr; addr < max_addr; addr += FLASH_SECTOR_SIZE) {
            size_t target_offset = addr - (size_t)M_OS_APP_TABLE_BASE;
            // goutf("seek file '%s' to %d\n", flash_me_file, target_offset);
            if ( FR_OK != f_lseek(f, target_offset) ) {
                goutf("Unable to seek file '%s' to %d\n", flash_me_file, target_offset);
                goto e4;
            }
            UINT br;
            // goutf("read %d bytes from file '%s'\n", FLASH_SECTOR_SIZE, flash_me_file);
            if ( FR_OK != f_read(f, buffer, FLASH_SECTOR_SIZE, &br) || !br ) {
                goutf("Unable to read %d bytes from file '%s'\n", FLASH_SECTOR_SIZE, flash_me_file);
                goto e4;
            }
            uint32_t flash_target_offset = addr - XIP_BASE;
            goutf("Erase and write to flash, offset: %ph\n", flash_target_offset);
            flash_block(buffer, flash_target_offset);
        }
        // gouta("Done\n");
    e4:
        f_close(f);
    e5:
        vPortFree(flash_me_file);
        vPortFree(alloc);
    }
e8:
    vPortFree(psym);
e3:
    vPortFree(strtab);
e2:
    vPortFree(symtab);
e11:
    vPortFree(psh);
e1:
    vPortFree(pehdr);
    f_close(f);
    vPortFree(f);
    bootb_ctx->sections = pctx->sections_lst;
    vPortFree(pctx);
    #if DEBUG_APP_LOAD
///    debug_sections(bootb_ctx->sect_entries);
    goutf("[%p][%p][%p][%p]\n", bootb_ctx->bootb[0], bootb_ctx->bootb[1], bootb_ctx->bootb[2], bootb_ctx->bootb[3]);
    #endif
    if (bootb_ctx->bootb[2] == 0) {
        goutf("'main' global function is not found in (or failed to load from) the '%s' elf-file\n", fn);
        ctx->ret_code = -1;
        return false;
    }
    ctx->stage = LOAD;
    if (ctx->next) {
        if (!load_app(ctx->next)) return false;
    }
    return true;
}

volatile bootb_ptr_t bootb_sync_signal = NULL;
#if DEBUG_HEAP_SIZE
void vShowAlloc( void );
#endif

static void __in_hfa() exec_sync(cmd_ctx_t* ctx) {
    #if DEBUG_APP_LOAD
         goutf("orig_cmd: [%p] %s, pipe: [%p] ", ctx, ctx->orig_cmd, ctx->next);
         if (ctx->argc) {
            goutf("%d argc [", ctx->argc);
            for(int i = 0; i < ctx->argc; i++) {
                goutf("%s ", ctx->argv[i]);
            }
            goutf("]\n");
         } else {
            goutf("0 argc\n");
         }
    #endif
    const TaskHandle_t th = xTaskGetCurrentTaskHandle();
    vTaskSetThreadLocalStoragePointer(th, 0, ctx);

    bootb_ctx_t* bootb_ctx = ctx->pboot_ctx;
    #if DEBUG_APP_LOAD
    goutf("__required_m_api_verion: [%p]\n", bootb_ctx->bootb[0]);
    #endif
    int rav = bootb_ctx->bootb[0] ? bootb_ctx->bootb[0]() : MIN_API_VERSION;
    #if DEBUG_APP_LOAD
    goutf("rav: %d\n", rav);
    #endif
    if (rav > M_API_VERSION) {
        goutf("Required by application '%s' M-API version %d is grater than provided (%d)\n",  ctx->argv[0], rav, M_API_VERSION);
        ctx->ret_code = -2;
        return;
    }
    if (rav < MIN_API_VERSION) { // unsupported earliest versions
        goutf("Application '%s' uses M-API version %d, that less than minimal required version: %d\n", ctx->argv[0], rav, MIN_API_VERSION);
        ctx->ret_code = -3;
        return;
    }
    if (bootb_ctx->bootb[1]) {
        int x = bootb_ctx->bootb[1]();
        #if DEBUG_APP_LOAD
        goutf("_init done: %p\n", x);
        #endif
    }
    #if DEBUG_APP_LOAD
    goutf("EXEC main: [%p]\n", bootb_ctx->bootb[2]);
    goutf("EXEC signal: [%p]\n", bootb_sync_signal);
    #endif
    bootb_sync_signal = bootb_ctx->bootb[4];
    int res = bootb_ctx->bootb[2] ? bootb_ctx->bootb[2]() : -3;
    bootb_sync_signal = NULL;
    #if DEBUG_APP_LOAD
    goutf("EXEC RET_CODE: %d -> _fini: %p\n", res, bootb_ctx->bootb[3]);
    #endif
    if (bootb_ctx->bootb[3]) {
        bootb_ctx->bootb[3]();
        #if DEBUG_APP_LOAD
        gouta("_fini done\n");
        #endif
    }
    ctx->ret_code = res;
}

static void __in_hfa() vAppDetachedTask(void *pv) {
    cmd_ctx_t* ctx = (cmd_ctx_t*)pv;
    #if DEBUG_APP_LOAD
    goutf("vAppDetachedTask: %s [%p]\n", ctx->orig_cmd, ctx);
    #endif
    const TaskHandle_t th = xTaskGetCurrentTaskHandle();
    vTaskSetThreadLocalStoragePointer(th, 0, ctx);
    exec_sync(ctx);
    remove_ctx(ctx);
    #if DEBUG_APP_LOAD
    goutf("vAppDetachedTask: [%p] <<<\n", ctx);
    #endif
    vTaskDelete( NULL );
}

void __in_hfa() exec(cmd_ctx_t* ctx) {
    do {
        cmd_ctx_t* pipe_ctx = ctx->next;
        #if DEBUG_APP_LOAD
        goutf("EXEC [%p]->[%p]\n", ctx, pipe_ctx);
        #endif
        if (ctx->detached) {
            cmd_ctx_t* ctxi = clone_ctx(ctx);
            #if DEBUG_APP_LOAD
            goutf("Clone ctx [%p]->[%p]\n", ctx, ctxi);
            #endif
            xTaskCreate(vAppDetachedTask, ctxi->argv[0], 1024/*x 4 = 4096*/, ctxi, configMAX_PRIORITIES - 1, NULL);
            cleanup_ctx(ctx);
        } else {
            exec_sync(ctx);
            cleanup_bootb_ctx(ctx);
            if (ctx->stage != PREPARED) { // it is expected cmd/cmd0 will prepare ctx for next run for application, in other case - cleanup ctx
                cleanup_ctx(ctx);
            }
#if DEBUG_HEAP_SIZE
    {
        vShowAlloc();
        size_t free_sz = xPortGetFreeHeapSize();
        goutf(" free_sz: %d\n", free_sz);
    }
#endif
            #if DEBUG_APP_LOAD
            goutf("EXEC [%p] <<\n", ctx);
            #endif
        }
        ctx = pipe_ctx;
    } while(ctx);
}

void __in_hfa() mallocFailedHandler(size_t sz) {
    gouta("WARN: malloc failed\n");
    {
        HeapStats_t stat;
        vPortGetHeapStats(&stat);
        goutf(
            "Requested: %d (%dK)\n"
            "Heap memory: %d (%dK)\n"
            " available bytes total: %d (%dK)\n"
            "         largets block: %d (%dK)\n",
            sz, sz >> 10,
            configTOTAL_HEAP_SIZE, configTOTAL_HEAP_SIZE >> 10,
            stat.xAvailableHeapSpaceInBytes, stat.xAvailableHeapSpaceInBytes >> 10,
            stat.xSizeOfLargestFreeBlockInBytes, stat.xSizeOfLargestFreeBlockInBytes >> 10
        );
        goutf(
            "        smallest block: %d (%dK)\n"
            "           free blocks: %d\n"
            "    min free remaining: %d (%dK)\n"
            "           allocations: %d\n"
            "                 frees: %d\n",
            stat.xSizeOfSmallestFreeBlockInBytes, stat.xSizeOfSmallestFreeBlockInBytes >> 10,
            stat.xNumberOfFreeBlocks,
            stat.xMinimumEverFreeBytesRemaining, stat.xMinimumEverFreeBytesRemaining >> 10,
            stat.xNumberOfSuccessfulAllocations, stat.xNumberOfSuccessfulFrees
        );
    }
}

void __in_hfa() overflowHook( TaskHandle_t pxTask, char *pcTaskName ) {
    goutf("WARN: stack overflow on task: '%s'\n", pcTaskName);
}

void __in_hfa() vCmdTask(void *pv) {
    const TaskHandle_t th = xTaskGetCurrentTaskHandle();
    cmd_ctx_t* ctx = get_cmd_startup_ctx();
    vTaskSetThreadLocalStoragePointer(th, 0, ctx);
    while(1) {
#if DEBUG_HEAP_SIZE
    {
        vShowAlloc();
        size_t free_sz = xPortGetFreeHeapSize();
        goutf(" free_sz: %d (before)\n", free_sz);
    }
#endif
        if (!ctx->argc && !ctx->argv) {
            ctx->argc = 1;
            ctx->argv = (char**)pvPortMalloc(sizeof(char*));
            char* comspec = get_ctx_var(ctx, "COMSPEC");
            ctx->argv[0] = copy_str(comspec);
            if(ctx->orig_cmd) vPortFree(ctx->orig_cmd);
            ctx->orig_cmd = copy_str(comspec);
        }
        // goutf("Lookup for: %s\n", ctx->orig_cmd);
        // goutf("be [%p]\n", xPortGetFreeHeapSize());
        bool b_exists = exists(ctx);
        // goutf("ae [%p]\n", xPortGetFreeHeapSize());
        if (b_exists) {
            size_t len = strlen(ctx->orig_cmd); // TODO: more than one?
            // goutf("Command found: %s\n", ctx->orig_cmd);
            if (len > 4 && (strcmp(ctx->orig_cmd + len - 5, ".m1p2") == 0 || strcmp(ctx->orig_cmd + len - 5, ".m2p2") == 0 )) {
                if(load_firmware(ctx->orig_cmd)) { // TODO: by ctx
                    ctx->stage = LOAD;
                    vTaskSetThreadLocalStoragePointer(th, 0, ctx);
                    //run_app(ctx->orig_cmd);
                    int res = ((boota_ptr_t)M_OS_APP_TABLE_BASE[0])(ctx->orig_cmd); // TODO: 0 - 2nd page, what exactly page used by app?
                    // goutf("RET_CODE: %d\n", res);
                    ctx->stage = EXECUTED;
                    cleanup_ctx(ctx);
                } else {
                    goutf("Unable to execute command: '%s' (failed to load it)\n", ctx->orig_cmd);
                    ctx->stage = INVALIDATED;
                    goto e;
                }
            } else if(is_new_app(ctx)) {
                // gouta("Command has appropriate format\n");
                if (load_app(ctx)) {
                    // goutf("Load passed for: %s\n", ctx->orig_cmd);
                    exec(ctx);
                   // while(!__getch());
                    // goutf("Exec passed for: %s\n", ctx->orig_cmd);
                } else {
                    goutf("Unable to execute command: '%s' (failed to load it)\n", ctx->orig_cmd);
                    ctx->stage = INVALIDATED;
                    goto e;
                }
            } else {
                goutf("Unable to execute command: '%s' (unknown format)\n", ctx->orig_cmd);
                ctx->stage = INVALIDATED;
                goto e;
            }
        } else {
            goutf("Illegal command: '%s'\n", ctx->orig_cmd);
            ctx->stage = INVALIDATED;
            goto e;
        }
        // repair system context
        ctx = get_cmd_startup_ctx();
        vTaskSetThreadLocalStoragePointer(th, 0, ctx);
        continue;
e:
        if (ctx->stage != PREPARED) { // it is expected cmd/cmd0 will prepare ctx for next run for application, in other case - cleanup ctx
            cleanup_ctx(ctx);
        }
    }
    vTaskDelete( NULL );
}

// support sygnal for current "sync_ctx" context only for now
void __in_hfa() app_signal(void) {
    if (bootb_sync_signal) bootb_sync_signal();
}

int __in_hfa() kill(uint32_t task_number) {
    configRUN_TIME_COUNTER_TYPE ulTotalRunTime, ulStatsAsPercentage;
    volatile UBaseType_t uxArraySize = uxTaskGetNumberOfTasks();
    TaskStatus_t *pxTaskStatusArray = pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );
    uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, &ulTotalRunTime );
    int res = 0;
    for ( UBaseType_t x = 0; x < uxArraySize; x++ ) {
        if (pxTaskStatusArray[ x ].xTaskNumber == task_number) {
            res = 1;
            cmd_ctx_t* ctx = (cmd_ctx_t*) pvTaskGetThreadLocalStoragePointer(pxTaskStatusArray[ x ].xHandle, 0);
            if (ctx) {
                ctx->stage = SIGTERM;
                if (ctx->pboot_ctx && ctx->pboot_ctx->bootb[4]) {
                    ctx->pboot_ctx->bootb[4](); // signal SIGTERM
                    res = 2;
                }
            } else {
                res = 3;
            }
            break;
        }
    }
    vPortFree( pxTaskStatusArray );
    return res;
}

void __not_in_flash_func(reboot_me)(void) {
    reboot_is_requested = true;
}
