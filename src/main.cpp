#include <cstdlib>
#include <cstring>
#include <pico.h>
#include <hardware/vreg.h>
#include <hardware/clocks.h>
#include <hardware/flash.h>
#include <hardware/watchdog.h>
#include <pico/bootrom.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include "sys_table.h"

#include "graphics.h"
#include "keyboard.h"

extern "C" {
#include "ps2.h"
#include "usb.h"
#include "ram_page.h"
#include "hardfault.h"
}

#include "ff.h"
#include "nespad.h"

#include "app.h"
#include "FreeRTOS.h"
#include "task.h"
#include "../../api/m-os-api-c-list.h"
#include "hooks.h"
#include "sound.h"
#include "psram_spi.h"

const char _mc_con[] = ".mc.con";
const char _cmd_history[] = ".cmd_history";
const char _flash_me[] = ".flash_me";
static const char SD[] = "SD";
static const char CD[] = "CD"; // current directory 
static const char BASE[] = "BASE"; 
static const char MOS[] = "MOS2"; 
extern "C" const char TEMP[] = "TEMP"; 
static const char PATH[] = "PATH"; 
static const char SWAP[] = "SWAP"; 
static const char COMSPEC[] = "COMSPEC"; 
static const char ctmp[] = "/tmp"; 
static const char ccmd[] = "/mos2/cmd";

static FATFS fs;
extern "C" FATFS* get_mount_fs() { return &fs; } // only one FS is supported foe now

semaphore vga_start_semaphore;
#define DISP_WIDTH (320)
#define DISP_HEIGHT (240)

uint16_t SCREEN[TEXTMODE_ROWS][TEXTMODE_COLS];

void __time_critical_func(render_core)() {
    multicore_lockout_victim_init();
    graphics_init();

    const auto buffer = (uint8_t *)SCREEN;

    graphics_set_bgcolor(0x000000);
    graphics_set_offset(0, 0);
    graphics_set_mode(TEXTMODE_DEFAULT);
    graphics_set_buffer(buffer, TEXTMODE_COLS, TEXTMODE_ROWS);
    graphics_set_textbuffer(buffer);
    clrScr(1);

    sem_acquire_blocking(&vga_start_semaphore);
    // 60 FPS loop
#define frame_tick (16666)
    uint64_t tick = time_us_64();
#ifdef TFT
    uint64_t last_renderer_tick = tick;
#endif
    uint64_t last_input_tick = tick;
    while (true) {
#ifdef TFT
        if (tick >= last_renderer_tick + frame_tick) {
            refresh_lcd();
            last_renderer_tick = tick;
        }
#endif
        // Every 5th frame
        if (tick >= last_input_tick + frame_tick * 5) {
            nespad_read();
            last_input_tick = tick;
        }
        tick = time_us_64();

        tight_loop_contents();
    }

    __unreachable();
}

void __always_inline run_application() {
    multicore_reset_core1();

    asm volatile (
        "mov r0, %[start]\n"
        "ldr r1, =%[vtable]\n"
        "str r0, [r1]\n"
        "ldmia r0, {r0, r1}\n"
        "msr msp, r0\n"
        "bx r1\n"
        :: [start] "r" (XIP_BASE + (FIRMWARE_OFFSET << 10)), [vtable] "X" (PPB_BASE + M33_VTOR_OFFSET)
    );

    __unreachable();
}

const char* tmp = "Murmulator (RP2350) OS v." MOS_VERSION;

extern "C" void show_logo(bool with_top) {
    uint32_t w = get_console_width();
    uint32_t y = get_console_height() - 1;
    uint32_t sz = strlen(tmp);
    uint32_t sps = (w - sz) / 2;

    for(uint32_t x = 0; x < w; ++x) {
        if(with_top) draw_text(" ", x, 0, 13, 1);
        draw_text(" ", x, y, 13, 1);
    }
    if(with_top) draw_text(tmp, sps, 0, 13, 1);
    draw_text(tmp, sps, y, 13, 1);
    graphics_set_con_color(7, 0); // TODO: config
}

static cmd_ctx_t* set_default_vars() {
    cmd_ctx_t* ctx = get_cmd_startup_ctx();
    set_ctx_var(ctx, CD, MOS);
    set_ctx_var(ctx, BASE, MOS);
    set_ctx_var(ctx, TEMP, ctmp);
    set_ctx_var(ctx, COMSPEC, ccmd);
    return ctx;
}

static char* open_config(UINT* pbr) {
    FILINFO* pfileinfo = (FILINFO*)pvPortMalloc(sizeof(FILINFO));
    size_t file_size = 0;
    char * cfn = "/config.sys";
    if (f_stat(cfn, pfileinfo) != FR_OK || (pfileinfo->fattrib & AM_DIR)) {
        cfn = "/mos2/config.sys";
        if (f_stat(cfn, pfileinfo) != FR_OK || (pfileinfo->fattrib & AM_DIR)) {
            vPortFree(pfileinfo);
            return 0;
        } else {
            file_size = (size_t)pfileinfo->fsize & 0xFFFFFFFF;
        }
    } else {
        file_size = (size_t)pfileinfo->fsize & 0xFFFFFFFF;
    }
    vPortFree(pfileinfo);

    FIL* pf = (FIL*)pvPortMalloc(sizeof(FIL));
    if(f_open(pf, cfn, FA_READ) != FR_OK) {
        vPortFree(pf);
        return 0;
    }
    char* buff = (char*)pvPortCalloc(file_size + 1, 1);
    if (f_read(pf, buff, file_size, pbr) != FR_OK) {
        goutf("Failed to read config.sys\n");
        vPortFree(buff);
        buff = 0;
    }
    f_close(pf);
    vPortFree(pf);
    return buff;
}

inline static void tokenizeCfg(char* s, size_t sz) {
    size_t i = 0;
    for (; i < sz; ++i) {
        if (s[i] == '=' || s[i] == '\n' || s[i] == '\r') {
            s[i] = 0;
        }
    }
    s[i] = 0;
}

static void load_config_sys() {
    cmd_ctx_t* ctx = set_default_vars();
    bool b_swap = false;
    bool b_base = false;
    bool b_temp = false;
    UINT br;
    char* buff = open_config(&br);
    if (buff) {
        tokenizeCfg(buff, br);
        char *t = buff;
        while (t - buff < br) {
            // goutf("%s\n", t);
            if (strcmp(t, PATH) == 0) {
                t = next_token(t);
                set_ctx_var(ctx, PATH, t);
            } else if (strcmp(t, TEMP) == 0) {
                t = next_token(t);
                set_ctx_var(ctx, TEMP, t);
                f_mkdir(t);
                b_temp = true;
            } else if (strcmp(t, COMSPEC) == 0) {
                t = next_token(t);
                set_ctx_var(ctx, COMSPEC, t);
            } else if (strcmp(t, SWAP) == 0) {
                t = next_token(t);
                init_vram(t);
                b_swap = true;
            } else if (strcmp(t, "GMODE") == 0) { // TODO: FONT
                t = next_token(t);
                int mode = atoi(t);
           ///     graphics_set_mode(mode);
            } else if (strcmp(t, "DRIVER") == 0) {
                t = next_token(t);
                // TODO:
            } else if (strcmp(t, "CPU") == 0) {
                t = next_token(t);
                int cpu = atoi(t);
                if (cpu > 123 && cpu < 450) {
            ///        set_last_overclocking(cpu * 1000);
                }
            } else if (strcmp(t, BASE) == 0) {
                t = next_token(t);
                set_ctx_var(ctx, BASE, t);
                f_mkdir(t);
                b_base = true;
            } else if (strcmp(t, CD) == 0) {
                t = next_token(t);
                set_ctx_var(ctx, CD, t);
                f_mkdir(t);
            } else {
                char* k = copy_str(t);
                t = next_token(t);
                set_ctx_var(ctx, k, t);
            }
            t = next_token(t);
        }
    }
    if (buff) vPortFree(buff);
    if (!b_temp) f_mkdir(ctmp);
    if (!b_base) f_mkdir(MOS);
    /**
    if (!b_swap) {
        char* t1 = "/tmp/pagefile.sys 1M 64K 4K";
        char* t2 = (char*)pvPortMalloc(strlen(t1) + 1);
        strcpy(t2, t1);
        init_vram(t2);
        vPortFree(t2);
    }*/
  //  uint32_t overclocking = get_overclocking_khz();
  //  set_sys_clock_khz(overclocking, true);
  //  set_last_overclocking(overclocking);
}

int __in_hfa() main() {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    
    volatile uint32_t *qmi_m0_timing=(uint32_t *)0x400d000c;
    vreg_disable_voltage_limit();
    vreg_set_voltage(VREG_VOLTAGE_1_60);
    sleep_ms(33);
    *qmi_m0_timing = 0x60007204;
    bool res = set_sys_clock_hz(configCPU_CLOCK_HZ, 0);
    *qmi_m0_timing = 0x60007303;

    keyboard_init();
    //keyboard_send(0xFF);
    nespad_begin(clock_get_hz(clk_sys) / 1000, NES_GPIO_CLK, NES_GPIO_DATA, NES_GPIO_LAT);

    char* y = (char*)0x20000000 + (512 << 10) - 4;
	bool magic = (y[0] == 0xFF && y[1] == 0x0F && y[2] == 0xF0 && y[3] == 0x17);
    if (magic) {
        *y++ = 0; *y++ = 0; *y++ = 0; *y++ = 0;
    }
    for (int i = 20; i--;) {
        nespad_read();
        sleep_ms(50);

        // F12 Boot to USB FIRMWARE UPDATE mode
        if (nespad_state & DPAD_START || getch_now() == 0x58) {
            reset_usb_boot(0, 0);
        }

        // Any other key/button - run launcher
//        if (magic || (nespad_state && !(nespad_state & DPAD_START)) || (input && input != 0x58)) {
//        }
    }

//    run_application();
    sem_init(&vga_start_semaphore, 0, 1);
    multicore_launch_core1(render_core);
    sem_release(&vga_start_semaphore);
    sleep_ms(250);
    load_config_sys();
    init_psram();
    show_logo(true);
    uint8_t rx[4];
    get_cpu_flash_jedec_id(rx);
    flash_size = (1 << rx[3]);

    f_mount(&fs, SD, 1);
    init_sound();

    xTaskCreate(vCmdTask, "cmd", 1024/*x4=4096*/, NULL, configMAX_PRIORITIES - 1, NULL);

    setApplicationMallocFailedHookPtr(mallocFailedHandler);
    setApplicationStackOverflowHookPtr(overflowHook);

    /* Start the scheduler. */
	vTaskStartScheduler();
    // it should never return
    draw_text("vTaskStartScheduler failed", 0, 4, 13, 1);

    while(sys_table_ptrs[0]); // to ensure linked
    __unreachable();
}
