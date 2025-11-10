#include <cstdlib>
#include <cstring>
#include <pico.h>
#include <hardware/vreg.h>
#include <hardware/pwm.h>
#include <hardware/clocks.h>
#include <hardware/watchdog.h>
#include <hardware/structs/qmi.h>
#include <hardware/regs/sysinfo.h>
#include <pico/bootrom.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>

#include "psram_spi.h"

#include "graphics.h"

extern "C" {
#include "ps2.h"
#include "ff.h"
#include "usb.h"
#include "FreeRTOS.h"
#include "task.h"
#include "hooks.h"

#include "tests.h"
#include "sys_table.h"
#include "portable.h"

#include "keyboard.h"
#include "cmd.h"
#include "hardfault.h"
#include "hardware/exception.h"
#include "ram_page.h"
#include "overclock.h"
#include "app.h"
#include "sound.h"
#if TFT
#include "st7789.h"
#endif
}

#include "nespad.h"

extern "C" uint32_t flash_size;;
int flash_mhz = FLASH_FREQ_MHZ;
int psram_mhz = PSRAM_FREQ_MHZ;
uint new_flash_timings = 0;
uint new_psram_timings = 0;
static int vreg = VREG_VOLTAGE_1_60;
static int new_vreg = VREG_VOLTAGE_1_60;

#include <hardware/structs/qmi.h>
#include <hardware/structs/xip.h>
extern "C" uint32_t butter_psram_size;
extern "C" uint32_t BUTTER_PSRAM_GPIO = 0;
extern "C" bool rp2350a = true;
#define MB16 (16ul << 20)
#define MB8 (8ul << 20)
#define MB4 (4ul << 20)
#define MB1 (1ul << 20)
volatile uint8_t* PSRAM_DATA = (uint8_t*)0x11000000;
inline static uint32_t __not_in_flash_func(_butter_psram_size)() {
    for(register int i = MB8; i < MB16; ++i)
        PSRAM_DATA[i] = 16;
    for(register int i = MB4; i < MB8; ++i)
        PSRAM_DATA[i] = 8;
    for(register int i = MB1; i < MB4; ++i)
        PSRAM_DATA[i] = 4;
    for(register int i = 0; i < MB1; ++i)
        PSRAM_DATA[i] = 1;
    register uint32_t res = PSRAM_DATA[MB16 - 1];
    for (register int i = MB16 - MB1; i < MB16; ++i) {
        if (res != PSRAM_DATA[i])
            return 0;
    }
    return res << 20;
}
void __no_inline_not_in_flash_func(psram_init)(uint cs_pin) {
    gpio_set_function(cs_pin, GPIO_FUNC_XIP_CS1);

    // Enable direct mode, PSRAM CS, clkdiv of 10.
    qmi_hw->direct_csr = 10 << QMI_DIRECT_CSR_CLKDIV_LSB | \
                               QMI_DIRECT_CSR_EN_BITS | \
                               QMI_DIRECT_CSR_AUTO_CS1N_BITS;
    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS)
        ;

    // Enable QPI mode on the PSRAM
    const uint CMD_QPI_EN = 0x35;
    qmi_hw->direct_tx = QMI_DIRECT_TX_NOPUSH_BITS | CMD_QPI_EN;

    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS)
        ;

    // Set PSRAM timing for APS6404
    //
    // Using an rxdelay equal to the divisor isn't enough when running the APS6404 close to 133MHz.
    // So: don't allow running at divisor 1 above 100MHz (because delay of 2 would be too late),
    // and add an extra 1 to the rxdelay if the divided clock is > 100MHz (i.e. sys clock > 200MHz).
    const int max_psram_freq = psram_mhz * 1000000;
    const int clock_hz = clock_get_hz(clk_sys);
    int divisor = (clock_hz + max_psram_freq - 1) / max_psram_freq;
    if (divisor == 1 && clock_hz > 100000000) {
        divisor = 2;
    }
    int rxdelay = divisor;
    if (clock_hz / divisor > 100000000) {
        rxdelay += 1;
    }

    // - Max select must be <= 8us.  The value is given in multiples of 64 system clocks.
    // - Min deselect must be >= 18ns.  The value is given in system clock cycles - ceil(divisor / 2).
    const int clock_period_fs = 1000000000000000ll / clock_hz;
    const int max_select = (125 * 1000000) / clock_period_fs;  // 125 = 8000ns / 64
    const int min_deselect = (18 * 1000000 + (clock_period_fs - 1)) / clock_period_fs - (divisor + 1) / 2;

    qmi_hw->m[1].timing = 1 << QMI_M1_TIMING_COOLDOWN_LSB |
                          QMI_M1_TIMING_PAGEBREAK_VALUE_1024 << QMI_M1_TIMING_PAGEBREAK_LSB |
                          max_select << QMI_M1_TIMING_MAX_SELECT_LSB |
                          min_deselect << QMI_M1_TIMING_MIN_DESELECT_LSB |
                          rxdelay << QMI_M1_TIMING_RXDELAY_LSB |
                          divisor << QMI_M1_TIMING_CLKDIV_LSB;

    // Set PSRAM commands and formats
    qmi_hw->m[1].rfmt =
        QMI_M0_RFMT_PREFIX_WIDTH_VALUE_Q << QMI_M0_RFMT_PREFIX_WIDTH_LSB |\
        QMI_M0_RFMT_ADDR_WIDTH_VALUE_Q   << QMI_M0_RFMT_ADDR_WIDTH_LSB |\
        QMI_M0_RFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M0_RFMT_SUFFIX_WIDTH_LSB |\
        QMI_M0_RFMT_DUMMY_WIDTH_VALUE_Q  << QMI_M0_RFMT_DUMMY_WIDTH_LSB |\
        QMI_M0_RFMT_DATA_WIDTH_VALUE_Q   << QMI_M0_RFMT_DATA_WIDTH_LSB |\
        QMI_M0_RFMT_PREFIX_LEN_VALUE_8   << QMI_M0_RFMT_PREFIX_LEN_LSB |\
        6                                << QMI_M0_RFMT_DUMMY_LEN_LSB;

    qmi_hw->m[1].rcmd = 0xEB;

    qmi_hw->m[1].wfmt =
        QMI_M0_WFMT_PREFIX_WIDTH_VALUE_Q << QMI_M0_WFMT_PREFIX_WIDTH_LSB |\
        QMI_M0_WFMT_ADDR_WIDTH_VALUE_Q   << QMI_M0_WFMT_ADDR_WIDTH_LSB |\
        QMI_M0_WFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M0_WFMT_SUFFIX_WIDTH_LSB |\
        QMI_M0_WFMT_DUMMY_WIDTH_VALUE_Q  << QMI_M0_WFMT_DUMMY_WIDTH_LSB |\
        QMI_M0_WFMT_DATA_WIDTH_VALUE_Q   << QMI_M0_WFMT_DATA_WIDTH_LSB |\
        QMI_M0_WFMT_PREFIX_LEN_VALUE_8   << QMI_M0_WFMT_PREFIX_LEN_LSB;

    qmi_hw->m[1].wcmd = 0x38;

    // Disable direct mode
    qmi_hw->direct_csr = 0;

    // Enable writes to PSRAM
    hw_set_bits(&xip_ctrl_hw->ctrl, XIP_CTRL_WRITABLE_M1_BITS);

    // detect a chip size
    butter_psram_size = _butter_psram_size();
}

void __not_in_flash() flash_timings() {
    if (!new_flash_timings) {
        const int max_flash_freq = flash_mhz * MHZ;
        const int clock_hz = get_overclocking_khz() * 1000;
        int divisor = (clock_hz + max_flash_freq - 1) / max_flash_freq;
        if (divisor == 1 && clock_hz > 100000000) {
            divisor = 2;
        }
        int rxdelay = divisor;
        if (clock_hz / divisor > 100000000) {
            rxdelay += 1;
        }
        qmi_hw->m[0].timing = 0x60007000 |
                            rxdelay << QMI_M0_TIMING_RXDELAY_LSB |
                            divisor << QMI_M0_TIMING_CLKDIV_LSB;
    } else {
        qmi_hw->m[0].timing = new_flash_timings;
    }
}

void __not_in_flash() psram_timings() {
    if (!new_psram_timings) {
        const int max_psram_freq = psram_mhz * MHZ;
        const int clock_hz = get_overclocking_khz() * 1000;
        int divisor = (clock_hz + max_psram_freq - 1) / max_psram_freq;
        if (divisor == 1 && clock_hz > 100000000) {
            divisor = 2;
        }
        int rxdelay = divisor;
        if (clock_hz / divisor > 100000000) {
            rxdelay += 1;
        }
        qmi_hw->m[1].timing = (qmi_hw->m[1].timing & ~0x000000FFF) |
                            rxdelay << QMI_M0_TIMING_RXDELAY_LSB |
                            divisor << QMI_M0_TIMING_CLKDIV_LSB;
    } else {
        qmi_hw->m[1].timing = new_psram_timings;
    }
}

void __no_inline_not_in_flash_func(psram_deinit)(uint cs_pin) {
    hw_clear_bits(&xip_ctrl_hw->ctrl, XIP_CTRL_WRITABLE_M1_BITS);
    qmi_hw->m[1].timing = 0;
    qmi_hw->m[1].rfmt   = 0;
    qmi_hw->m[1].rcmd   = 0;
    qmi_hw->m[1].wfmt   = 0;
    qmi_hw->m[1].wcmd   = 0;
    qmi_hw->direct_csr = 0;
    if (gpio_get_function(cs_pin) == GPIO_FUNC_XIP_CS1) {
        gpio_set_function(cs_pin, GPIO_FUNC_SIO);
    }
    butter_psram_size = 0;
}

static FATFS fs;
extern "C" FATFS* get_mount_fs() { // only one FS is supported for now
    return &fs;
}
semaphore vga_start_semaphore;
static int override_drv = -1;
static int drv = DEFAULT_VIDEO_DRIVER;
extern "C" volatile bool reboot_is_requested;

void __time_critical_func(render_core)() {
    multicore_lockout_victim_init();
    graphics_init(drv);
    // graphics_driver_t* gd = get_graphics_driver();
    // install_graphics_driver(gd);
    sem_acquire_blocking(&vga_start_semaphore);
    while(!reboot_is_requested) {
        pcm_call();
#if 0
        if (drv == TFT_DRV) refresh_lcd();
#endif
        tight_loop_contents();
    }
    watchdog_enable(1, true);
    while(true) ;
    __unreachable();
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

static const char SD[] = "SD";
static const char CD[] = "CD"; // current directory 
static const char BASE[] = "BASE"; 
static const char MOS[] = "MOS2"; 
static const char TEMP[] = "TEMP"; 
static const char PATH[] = "PATH"; 
static const char SWAP[] = "SWAP"; 
static const char COMSPEC[] = "COMSPEC"; 
static const char ctmp[] = "/tmp"; 
static const char ccmd[] = "/mos2/cmd";

static void __always_inline check_firmware() {
    FIL f;
    if(f_open(&f, FIRMWARE_MARKER_FN, FA_READ) == FR_OK) {
        f_close(&f);
        f_unmount(SD);
        char* y = (char*)0x20000000 + (512 << 10) - 4;
	    y[0] = 0x37; y[1] = 0x0F; y[2] = 0xF0; y[3] = 0x17;
        watchdog_enable(1, false);
    }
}

inline static void unlink_firmware() {
    f_unlink(FIRMWARE_MARKER_FN);
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
                graphics_set_mode(mode);
            } else if (strcmp(t, "DRIVER") == 0) {
                t = next_token(t);
                // TODO:
            } else if (strcmp(t, "CPU") == 0) {
                t = next_token(t);
                int cpu = atoi(t);
                if (cpu > 123 && cpu < 1000) {
                    set_last_overclocking(cpu * 1000);
                }
            } else if (strcmp(t, "VREG") == 0) {
                t = next_token(t);
                new_vreg = atoi(t);
                if (new_vreg != vreg && new_vreg >= VREG_VOLTAGE_0_55 && new_vreg <= VREG_VOLTAGE_3_30) {
                    vreg = new_vreg;
                    vreg_set_voltage((enum vreg_voltage)vreg);
                }
            } else if (!new_flash_timings && strcmp(t, "FLASH") == 0) {
                t = next_token(t);
                int new_flash_mhz = atoi(t);
                if (flash_mhz != new_flash_mhz) {
                    flash_mhz = new_flash_mhz;
                    flash_timings();
                }
            } else if (strcmp(t, "FLASH_T") == 0) {
                t = next_token(t);
                char *endptr;
                new_flash_timings = (uint)strtol(t, &endptr, 16);
                if (*endptr == 0 && qmi_hw->m[0].timing != new_flash_timings) {
                    flash_timings();
                }
            } else if (!new_psram_timings && strcmp(t, "PSRAM") == 0) {
                t = next_token(t);
                int new_psram_mhz = atoi(t);
                if (psram_mhz != new_psram_mhz) {
                    psram_mhz = new_psram_mhz;
                    psram_timings();
                }
            } else if (strcmp(t, "PSRAM_T") == 0) {
                t = next_token(t);
                char *endptr;
                new_psram_timings = (uint)strtol(t, &endptr, 16);
                if (*endptr == 0 && qmi_hw->m[1].timing != new_psram_timings) {
                    psram_timings();
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
    if (!b_swap) {
        char* t1 = "/tmp/pagefile.sys 1M 64K 4K";
        char* t2 = (char*)pvPortMalloc(strlen(t1) + 1);
        strcpy(t2, t1);
        init_vram(t2);
        vPortFree(t2);
    }
    uint32_t overclocking = get_overclocking_khz();
    set_sys_clock_khz(overclocking, true);
    set_last_overclocking(overclocking);
}

const char mRP2350[] = "Murmulator (RP2350";
const char mOSV[] = ") OS v." MOS_VERSION_STR;

#ifdef DEBUG_VGA
extern "C" char vga_dbg_msg[1024];
#endif

extern "C" void show_logo(bool with_top) {
    uint32_t w = get_console_width();
    uint32_t y = get_console_height() - 1;
    uint32_t sz = sizeof(mRP2350) + sizeof(mOSV) - 1;
    uint32_t sps = (w - sz) / 2;

    for(uint32_t x = 0; x < w; ++x) {
        if(with_top) draw_text(" ", x, 0, 13, 1);
        draw_text(" ", x, y, 13, 1);
    }
    if(with_top) {
        draw_text(mRP2350, sps, 0, 13, 1);
        draw_text(rp2350a ? "A" : "B", sps + sizeof(mRP2350) - 1, 0, 13, 1);
        draw_text(mOSV, sps + sizeof(mRP2350), 0, 13, 1);
    }
    draw_text(mRP2350, sps, y, 13, 1);
    draw_text(rp2350a ? "A" : "B", sps + sizeof(mRP2350) - 1, y, 13, 1);
    draw_text(mOSV, sps + sizeof(mRP2350), y, 13, 1);
    graphics_set_con_color(7, 0); // TODO: config
}


// connection is possible 00->00 (external pull down)
static int __in_hfa() test_0000_case(uint32_t pin0, uint32_t pin1, int res) {
    gpio_init(pin0);
    gpio_set_dir(pin0, GPIO_OUT);
    sleep_ms(33);
    gpio_put(pin0, 1);

    gpio_init(pin1);
    gpio_set_dir(pin1, GPIO_IN);
    gpio_pull_down(pin1); /// external pulled down (so, just to ensure)
    sleep_ms(33);
    if ( gpio_get(pin1) ) { // 1 -> 1, looks really connected
        res |= (1 << 5) | 1;
    }
    gpio_deinit(pin0);
    gpio_deinit(pin1);
    return res;
}

// connection is possible 01->01 (no external pull up/down)
static int __in_hfa() test_0101_case(uint32_t pin0, uint32_t pin1, int res) {
    gpio_init(pin0);
    gpio_set_dir(pin0, GPIO_OUT);
    sleep_ms(33);
    gpio_put(pin0, 1);

    gpio_init(pin1);
    gpio_set_dir(pin1, GPIO_IN);
    gpio_pull_down(pin1);
    sleep_ms(33);
    if ( gpio_get(pin1) ) { // 1 -> 1, looks really connected
        res |= (1 << 5) | 1;
    }
    gpio_deinit(pin0);
    gpio_deinit(pin1);
    return res;
}

// connection is possible 11->11 (externally pulled up)
static int __in_hfa() test_1111_case(uint32_t pin0, uint32_t pin1, int res) {
    gpio_init(pin0);
    gpio_set_dir(pin0, GPIO_OUT);
    sleep_ms(33);
    gpio_put(pin0, 0);

    gpio_init(pin1);
    gpio_set_dir(pin1, GPIO_IN);
    gpio_pull_up(pin1); /// external pulled up (so, just to ensure)
    sleep_ms(33);
    if ( !gpio_get(pin1) ) { // 0 -> 0, looks really connected
        res |= 1;
    }
    gpio_deinit(pin0);
    gpio_deinit(pin1);
    return res;
}

static int __in_hfa() testPins(uint32_t pin0, uint32_t pin1) {
    int res = 0b000000;
#if PICO_RP2350
    /// do not try to test butter psram this way
    if (BUTTER_PSRAM_GPIO) {
        if (pin0 == BUTTER_PSRAM_GPIO || pin1 == BUTTER_PSRAM_GPIO) return res;
    }
    if (pin0 == 23 || pin1 == 23) return res; // SMPS Power
    if (pin0 == 24 || pin1 == 24) return res; // VBus sense
    if (pin0 == 25 || pin1 == 25) return res; // LED
#endif
    // try pull down case (passive)
    gpio_init(pin0);
    gpio_set_dir(pin0, GPIO_IN);
    gpio_pull_down(pin0);
    gpio_init(pin1);
    gpio_set_dir(pin1, GPIO_IN);
    gpio_pull_down(pin1);
    sleep_ms(33);
    int pin0vPD = gpio_get(pin0);
    int pin1vPD = gpio_get(pin1);
    gpio_deinit(pin0);
    gpio_deinit(pin1);
    /// the same for pull_down state, try pull up case (passive)
    gpio_init(pin0);
    gpio_set_dir(pin0, GPIO_IN);
    gpio_pull_up(pin0);
    gpio_init(pin1);
    gpio_set_dir(pin1, GPIO_IN);
    gpio_pull_up(pin1);
    sleep_ms(33);
    int pin0vPU = gpio_get(pin0);
    int pin1vPU = gpio_get(pin1);
    gpio_deinit(pin0);
    gpio_deinit(pin1);

    res = (pin0vPD << 4) | (pin0vPU << 3) | (pin1vPD << 2) | (pin1vPU << 1);

    if (pin0vPD == 1) {
        if (pin0vPU == 1) { // pin0vPD == 1 && pin0vPU == 1
            if (pin1vPD == 1) { // pin0vPD == 1 && pin0vPU == 1 && pin1vPD == 1
                if (pin1vPU == 1) { // pin0vPD == 1 && pin0vPU == 1 && pin1vPD == 1 && pin1vPU == 1
                    // connection is possible 11->11 (externally pulled up)
                    return test_1111_case(pin0, pin1, res);
                } else { // pin0vPD == 1 && pin0vPU == 1 && pin1vPD == 1 && pin1vPU == 0
                    // connection is impossible
                    return res;
                }
            } else { // pin0vPD == 1 && pin0vPU == 1 && pin1vPD == 0
                if (pin1vPU == 1) { // pin0vPD == 1 && pin0vPU == 1 && pin1vPD == 0 && pin1vPU == 1
                    // connection is impossible
                    return res;
                } else { // pin0vPD == 1 && pin0vPU == 1 && pin1vPD == 0 && pin1vPU == 0
                    // connection is impossible
                    return res;
                }
            }
        } else {  // pin0vPD == 1 && pin0vPU == 0
            if (pin1vPD == 1) { // pin0vPD == 1 && pin0vPU == 0 && pin1vPD == 1
                if (pin1vPU == 1) { // pin0vPD == 1 && pin0vPU == 0 && pin1vPD == 1 && pin1vPU == 1
                    // connection is impossible
                    return res;
                } else { // pin0vPD == 1 && pin0vPU == 0 && pin1vPD == 1 && pin1vPU == 0
                    // connection is possible 10->10 (pulled up on down, and pulled down on up?)
                    return res |= (1 << 5) | 1; /// NOT SURE IT IS POSSIBLE TO TEST SUCH CASE (TODO: think about real cases)
                }
            } else { // pin0vPD == 1 && pin0vPU == 0 && pin1vPD == 0
                if (pin1vPU == 1) { // pin0vPD == 1 && pin0vPU == 0 && pin1vPD == 0 && pin1vPU == 1
                    // connection is impossible
                    return res;
                } else { // pin0vPD == 1 && pin0vPU == 0 && pin1vPD == 0 && pin1vPU == 0
                    // connection is impossible
                    return res;
                }
            }
        }
    } else { // pin0vPD == 0
        if (pin0vPU == 1) { // pin0vPD == 0 && pin0vPU == 1
            if (pin1vPD == 1) { // pin0vPD == 0 && pin0vPU == 1 && pin1vPD == 1
                if (pin1vPU == 1) { // pin0vPD == 0 && pin0vPU == 1 && pin1vPD == 1 && pin1vPU == 1
                    // connection is impossible
                    return res;
                } else { // pin0vPD == 0 && pin0vPU == 1 && pin1vPD == 1 && pin1vPU == 0
                    // connection is impossible
                    return res;
                }
            } else { // pin0vPD == 0 && pin0vPU == 1 && pin1vPD == 0
                if (pin1vPU == 1) { // pin0vPD == 0 && pin0vPU == 1 && pin1vPD == 0 && pin1vPU == 1
                    // connection is possible 01->01 (no external pull up/down)
                    return test_0101_case(pin0, pin1, res);
                } else { // pin0vPD == 0 && pin0vPU == 1 && pin1vPD == 0 && pin1vPU == 0
                    // connection is impossible
                    return res;
                }
            }
        } else {  // pin0vPD == 0 && pin0vPU == 0
            if (pin1vPD == 1) { // pin0vPD == 0 && pin0vPU == 0 && pin1vPD == 1
                if (pin1vPU == 1) { // pin0vPD == 0 && pin0vPU == 0 && pin1vPD == 1 && pin1vPU == 1
                    // connection is impossible
                    return res;
                } else { // pin0vPD == 0 && pin0vPU == 0 && pin1vPD == 1 && pin1vPU == 0
                    // connection is impossible
                    return res;
                }
            } else { // pin0vPD == 0 && pin0vPU == 0 && pin1vPD == 0
                if (pin1vPU == 1) { // pin0vPD == 0 && pin0vPU == 0 && pin1vPD == 0 && pin1vPU == 1
                    // connection is impossible
                    return res;
                } else { // pin0vPD == 0 && pin0vPU == 0 && pin1vPD == 0 && pin1vPU == 0
                    // connection is possible 00->00 (externally pulled down)
                    return test_0000_case(pin0, pin1, res);
                }
            }
        }
    }
    return res;
}

#if TFT
static void tft_refresh(void* pv) {
    while(1) {
        refresh_lcd();
        vTaskDelay(20);
    }
}
#endif

static void __in_hfa() startup_vga(void) {
    if (override_drv >= 0) {
        drv = override_drv;
    } else {
        #ifdef HDMI_DRV
        uint8_t link6 = testPins(VGA_BASE_PIN, VGA_BASE_PIN + 1);
        if (link6 == 0 || link6 == 0x1F) {
            drv = VGA_DRV;
        }
        #ifdef TFT_DRV
        else {
            uint8_t link9 = testPins(VGA_BASE_PIN + 3, VGA_BASE_PIN + 4);
            bool pio9PD = !!(link9 & 0b010000);
            bool pio9PU = !!(link9 & 0b001000);
            if (!pio9PD && !pio9PU) {
                drv = TFT_DRV;
            } else {
    drv = HDMI_DRV;
            }
        }
        #else
        else {
            drv = HDMI_DRV;
        }
        #endif
        #endif
    }
#if TFT
    if (drv == TFT_DRV) {
        tft_graphics_set_buffer((uint8_t*)pvPortMalloc(320 * 240), 320, 240);
    }
#endif
    sem_init(&vga_start_semaphore, 0, 1);
    multicore_launch_core1(render_core);
    sem_release(&vga_start_semaphore);
    vTaskDelay(300);
#if TFT
    if (drv == TFT_DRV) {
        xTaskCreate(tft_refresh, "tft", 1024/*x4=4096*/, NULL, configMAX_PRIORITIES - 2, NULL);
    }
#endif
    clrScr(0);
}

void __in_hfa() info(bool with_sd) {
    uint32_t ram32 = 520 << 10;// get_cpu_ram_size();
    uint8_t rx[4];
    get_cpu_flash_jedec_id(rx);
    flash_size = (1 << rx[3]);
    goutf("CPU: RP2350%c %d MHz\n"
          "SRAM %d KB\n"
          "FLASH %d MB; JEDEC ID: %02X-%02X-%02X-%02X (max %d MHz, [T%p])\n",
          rp2350a ? 'A' : 'B',
          get_overclocking_khz() / 1000,
          ram32 >> 10,
          flash_size >> 20, rx[0], rx[1], rx[2], rx[3], flash_mhz, qmi_hw->m[0].timing
    );
    if (butter_psram_size) {
        goutf("Butter-PSRAM %d MB on GPIO%d (max %d MHz, [T%p])\n",
              butter_psram_size >> 20, BUTTER_PSRAM_GPIO, psram_mhz, qmi_hw->m[1].timing
        );
    }
    if (!butter_psram_size || BUTTER_PSRAM_GPIO == 47) {
        uint32_t psram32 = psram_size();
        if (psram32) {
            uint8_t rx8[8];
            psram_id(rx8);
            if (psram32) {
                goutf("Murm-PSRAM %d MB; MF ID: %02x; KGD: %02x; EID: %02X%02X-%02X%02X-%02X%02X\n",
                    psram32 >> 20, rx8[0], rx8[1], rx8[2], rx8[3], rx8[4], rx8[5], rx8[6], rx8[7]
                );
            }
        }
    }
    if (!with_sd) {
        goutf("\n");
        return;
    }
    FATFS* fs = get_mount_fs();
    goutf("SDCARD %d FATs; %d free clusters (%d KB each)\n",
          fs->n_fats, f_getfree32(fs), fs->csize >> 1
    );
    goutf("SWAP %d MB; RAM: %d KB; pages index: %d x %d KB\n",
          swap_size() >> 20, swap_base_size() >> 10, swap_pages(), swap_page_size() >> 10
    );
    goutf("VRAM %d KB; video mode: %d x %d x %d bit\n"
          "\n",
          get_buffer_size() >> 10, get_screen_width(), get_screen_height(), get_console_bitness()
    );
}

void usb_on_boot() {
    usb_driver(true);
    for(;;) { vTaskDelay(10); }
}

void caseF10(void) {
    if (FR_OK == f_mount(&fs, SD, 1)) {
        FIL f;
        link_firmware(&f, "unknown");
    }
    watchdog_enable(1, false);
    while(1);
}

void caseF12(void) {
    if (FR_OK == f_mount(&fs, SD, 1)) {
        unlink_firmware();
    }
    reset_usb_boot(0, 0);
    while(1);
}

void selectDRV1(void) {
    override_drv = HDMI_DRV;
}

void selectDRV2(void) {
    override_drv = VGA_DRV;
}

kbd_state_t* __in_hfa() process_input_on_boot() {
    char* y = (char*)0x20000000 + (512 << 10) - 4;
	bool magicUnlink = (y[0] == 0xFF && y[1] == 0x0F && y[2] == 0xF0 && y[3] == 0x17);
    if (magicUnlink) {
        *y++ = 0; *y++ = 0; *y++ = 0; *y++ = 0;
    }
    vTaskDelay(50);
    kbd_state_t* ks = get_kbd_state();
    for (int a = 0; a < 20; ++a) {
        uint8_t sc = ks->input & 0xFF;
        if ( sc == 1 /* Esc */) {
            break;
        }
        if ( (nespad_state & DPAD_START) && (nespad_state & DPAD_SELECT) || (sc ==0x44) /*F10*/ ) {
            caseF10();
        }
        // F12 or ENTER or START Boot to USB FIRMWARE UPDATE mode
        if ((nespad_state & DPAD_START) || (sc == 0x58) /*F12*/ || (sc == 0x1C) /*ENTER*/) {
            caseF12();
        }
        // F11 or SPACE or SELECT unlink prev uf2 firmware
        if (magicUnlink || (nespad_state & DPAD_SELECT) || (sc == 0x57) /*F11*/  || (sc == 0x39) /*SPACE*/) {
            if (FR_OK == f_mount(&fs, SD, 1)) {
                if (nespad_state & DPAD_B) {
                    usb_on_boot();
                }
                unlink_firmware(); // return to M-OS
                break;
            }
        } else if (sc == 0x47 /*HOME*/ && FR_OK == f_mount(&fs, SD, 1)) {
            usb_on_boot();
        }
        // DPAD A/TAB start with HDMI, if default is VGA, and vice versa
        if ((nespad_state & DPAD_A) || (sc == 0x0F) /*TAB*/) {
            selectDRV1();
            break;
        }
        // DPAD B start with VGA, if default is TV
        if ((nespad_state & DPAD_B)) {
            selectDRV2();
            break;
        }
        vTaskDelay(30);
        nespad_read();
    }
    return ks;
}

char* mount_os() {
    if (FR_OK != f_mount(&fs, SD, 1)) {
        return "SD Card not inserted or SD Card error!\nPls. insert it and reboot...\n";
    }
    FILINFO fno;
    if ((FR_OK != f_stat(ccmd, &fno)) || (fno.fattrib & AM_DIR)) {
        return "/mos2/cmd file is not found!\nPls. copy MOS folder to your SDCARD...\n";
    }
    return 0;
}

void test_cycle(kbd_state_t* ks) {
    while (true) {
        nespad_read();
        int y = graphics_con_y();
        goutf("Scancodes tester: %Xh   \n", ks->input);
        goutf("Joysticks' states: %02Xh %02Xh\n", nespad_state, nespad_state2);
        vTaskDelay(50);
        graphics_set_con_pos(0, y);
    }
    __unreachable();
}

void __in_hfa() init(void) {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    vreg_disable_voltage_limit();
    vreg_set_voltage(VREG_VOLTAGE_1_60);
    flash_timings();
    sleep_ms(100);
    uint32_t overclocking = get_overclocking_khz();
    if (! set_sys_clock_khz(overclocking, 0) ) {
        overclocking = 252000;
    }
    set_last_overclocking(overclocking);
    rp2350a = (*((io_ro_32*)(SYSINFO_BASE + SYSINFO_PACKAGE_SEL_OFFSET)) & 1);
#ifdef MURM2
    BUTTER_PSRAM_GPIO = rp2350a ?  8 : 47;
#elif ZERO2
    BUTTER_PSRAM_GPIO = 47;
#else
    BUTTER_PSRAM_GPIO = rp2350a ? 19 : 47;
#endif
    psram_init(BUTTER_PSRAM_GPIO);
    keyboard_init();
    nespad_begin(clock_get_hz(clk_sys) / 1000, NES_GPIO_CLK, NES_GPIO_DATA, NES_GPIO_LAT);
    if (!butter_psram_size || BUTTER_PSRAM_GPIO == 47) {
        init_psram();
    }
}

static void __in_hfa() vPostInit(void *pv) {
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    kbd_state_t* ks = process_input_on_boot();
    // send kbd reset only after initial process passed
    keyboard_send(0xFF);
    char* err = mount_os();
    if (!err) {
        check_firmware();
    } else {
        startup_vga();
        graphics_set_mode(graphics_get_default_mode());
        graphics_set_con_pos(0, 1);
        show_logo(true);
        info(false);
        graphics_set_con_color(12, 0);
        gouta(err);
        test_cycle(ks);
        __unreachable();
    }

    if (nespad_state & DPAD_SELECT) {
        set_default_vars();
    } else {
        load_config_sys();
    }
    startup_vga();
    graphics_set_mode(graphics_get_default_mode());
///    exception_set_exclusive_handler(HARDFAULT_EXCEPTION, hardfault_handler);
    show_logo(true);
    graphics_set_con_pos(0, 1);
    init_sound();
    gpio_put(PICO_DEFAULT_LED_PIN, false);

    info(true);

    setApplicationMallocFailedHookPtr(mallocFailedHandler);
    setApplicationStackOverflowHookPtr(overflowHook);

    vCmdTask(pv);
//    xTaskCreate(vCmdTask, "cmd", 1024/*x4=4096*/, NULL, configMAX_PRIORITIES - 1, NULL);
//    vTaskDelete( NULL );
}

__attribute__((constructor))
static void before_main(void) {
    char* y = (char*)0x20000000 + (512 << 10) - 4;
	bool magicSkip = (y[0] == 0x37 && y[1] == 0x0F && y[2] == 0xF0 && y[3] == 0x17);
    if (magicSkip) {
        *y++ = 0; *y++ = 0; *y++ = 0; *y++ = 0;
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
}

int main() {
    init();
    xTaskCreate(vPostInit, "cmd", 1024/*x4=4096*/, NULL, configMAX_PRIORITIES - 1, NULL);
	vTaskStartScheduler(); // it should never return
    draw_text("vTaskStartScheduler failed", 0, 4, 13, 1);
    while(sys_table_ptrs[0]);
    __unreachable();
}
