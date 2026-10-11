#include "overclock.h"
#include "graphics.h"
#include "cmd.h"
#include <pico/stdlib.h>
#include <hardware/clocks.h>
#include <hardware/vreg.h>

// defined in main.cpp / nespad.cpp
void flash_timings_for(uint32_t khz);
void psram_timings_for(uint32_t khz);
void nespad_reclock(uint32_t cpu_khz);
void psram_spi_reclock(uint32_t cpu_khz);
void sd_reclock(uint32_t cpu_khz);
void boot_substage(int sub); // main.cpp: start diagnostics
void graphics_reclock(void);
void i2s_reclock(void);
#include "FreeRTOS.h"
#include "task.h"
#include <hardware/structs/systick.h>

// everything already started on the old clk_sys follows the new one: PIO dividers of the drivers
// (video, audio, gamepad, SPI PSRAM, PIO SD) and the OS tick
static void reclock_peripherals(uint32_t khz) {
    boot_substage(4);
    nespad_reclock(khz);
    psram_spi_reclock(khz);
    sd_reclock(khz);
    boot_substage(5);
    graphics_reclock();
    boot_substage(6);
    i2s_reclock();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        // the port computed the SysTick reload from clk_sys at the scheduler start
        systick_hw->rvr = khz * 1000 / configTICK_RATE_HZ - 1;
        systick_hw->cvr = 0;
    }
}

static uint32_t overclocking_khz = OVERCLOCKING * 1000;
static uint32_t last_overclocking_khz = 0;
static uint32_t vco = 0;
static uint32_t postdiv1 = 0;
static uint32_t postdiv2 = 0;

void set_overclocking(uint32_t khz) {
    overclocking_khz = khz;
}
void set_last_overclocking(uint32_t khz) {
    last_overclocking_khz = khz;
    overclocking_khz = khz;
}
uint32_t __not_in_flash() get_overclocking_khz() {
    return last_overclocking_khz ? last_overclocking_khz : overclocking_khz;
}

void overclocking_ex(uint32_t _vco, uint32_t _postdiv1, uint32_t _postdiv2) {
    /** TODO:
    if (last_overclocking_khz != overclocking_khz || vco != _vco || postdiv1 != _postdiv1 || postdiv2 != _postdiv2) {
        overclocking_khz = vco / postdiv1 / postdiv2 / 1000;
        set_sys_clock_pll(_vco, _postdiv1, _postdiv2);
        last_overclocking_khz = overclocking_khz;
        vco = _vco;
        postdiv1 = _postdiv1;
        postdiv2 = _postdiv2;
    }
    */
    goutf("CPU: %f MHz (vco: %d MHz, pd1: %d, pd2: %d)\n", overclocking_khz / 1000.0, vco / 1000000, postdiv1, postdiv2);
}

/* Core voltage. RP2350 VREG steps (mV) by enum vreg_voltage value */
static const uint16_t vreg_mv_tbl[32] = {
     550,  600,  650,  700,  750,  800,  850,  900,  950, 1000, 1050, 1100, 1150, 1200, 1250, 1300,
    1350, 1400, 1500, 1600, 1650, 1700, 1800, 1900, 2000, 2350, 2500, 2650, 2800, 3000, 3150, 3300
};
static int vreg_override_mv = 0; // 0 - automatic, by frequency

// the closest supported voltage not below mv (limited by VREG_MIN_MV..VREG_MAX_MV)
static enum vreg_voltage mv_to_vreg(int mv) {
    if (mv < VREG_MIN_MV) mv = VREG_MIN_MV;
    if (mv > VREG_MAX_MV) mv = VREG_MAX_MV;
    for (int i = 0; i < 32; ++i) {
        if (vreg_mv_tbl[i] >= mv) return (enum vreg_voltage)i;
    }
    return VREG_VOLTAGE_1_60;
}

// safe defaults per frequency (as in murm386)
static int auto_vreg_mv(uint32_t khz) {
    if (khz > 504000) return 1650;
    if (khz >= 378000) return 1600;
    return 1500;
}

int get_vreg_mv(void) {
    return vreg_mv_tbl[vreg_get_voltage() & 31];
}

int get_vreg_override_mv(void) {
    return vreg_override_mv;
}

// "1.6", "1.60" (V), "1600" (mV), "AUTO" / "0" (automatic), or an old style vreg_voltage index (< 32)
int parse_vreg_mv(const char* t) {
    if (!t) return 0;
    int v = 0, frac = 0, fdig = 0;
    bool dot = false;
    for (; *t; ++t) {
        if (*t == '.' || *t == ',') { if (dot) break; dot = true; continue; }
        if (*t < '0' || *t > '9') break;
        if (dot) { if (fdig < 3) { frac = frac * 10 + (*t - '0'); fdig++; } }
        else v = v * 10 + (*t - '0');
    }
    if (dot) {
        while (fdig++ < 3) frac *= 10;
        return v * 1000 + frac;
    }
    if (v > 0 && v < 32) return vreg_mv_tbl[v];
    return v; // mV or 0
}

// 0 - automatic; the value is applied by the next overclocking() call
void set_vreg_mv(int mv) {
    vreg_override_mv = mv <= 0 ? 0 : vreg_mv_tbl[mv_to_vreg(mv)];
}

/* Set the system clock to overclocking_khz and the core voltage (explicit or automatic).
   Order matters: raising - voltage, settle, flash/PSRAM timings for the new clock (they are only
   slower at the old one), clock; lowering - clock, timings, voltage. */
void overclocking() {
    uint vco, postdiv1, postdiv2;
    uint32_t khz = overclocking_khz;
    if (!check_sys_clock_khz(khz, &vco, &postdiv1, &postdiv2)) {
        overclocking_khz = get_overclocking_khz(); // keep the current one
        return;
    }
    uint32_t cur_khz = clock_get_hz(clk_sys) / 1000;
    enum vreg_voltage v = mv_to_vreg(vreg_override_mv ? vreg_override_mv : auto_vreg_mv(khz));
    bool v_changed = v != vreg_get_voltage();
    if (khz >= cur_khz) {
        if (v_changed) {
            boot_substage(1);
            vreg_disable_voltage_limit();
            vreg_set_voltage(v);
            busy_wait_ms(50);
        }
        if (khz != cur_khz) {
            boot_substage(2);
            flash_timings_for(khz);
            psram_timings_for(khz);
            boot_substage(3);
            set_sys_clock_pll(vco, postdiv1, postdiv2);
        }
    } else {
        set_sys_clock_pll(vco, postdiv1, postdiv2);
        flash_timings_for(khz);
        psram_timings_for(khz);
        if (v_changed) {
            vreg_disable_voltage_limit();
            vreg_set_voltage(v);
            busy_wait_ms(10);
        }
    }
    last_overclocking_khz = khz;
    if (khz != cur_khz) {
        reclock_peripherals(khz);
    }
}
