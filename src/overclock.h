#pragma once
#include <stdint.h>

void set_overclocking(uint32_t khz);
uint32_t get_overclocking_khz();
void overclocking();
void overclocking_ex(uint32_t vco, uint32_t postdiv1, uint32_t postdiv2);
// internal
void set_last_overclocking(uint32_t khz);

// core voltage, mV
#define VREG_MIN_MV 850
#define VREG_MAX_MV 1700
int get_vreg_mv(void);          // current
int get_vreg_override_mv(void); // 0 - automatic (by frequency)
void set_vreg_mv(int mv);       // 0 - automatic; applied by overclocking()
int parse_vreg_mv(const char* t); // "1.60", "1600", "AUTO", old vreg_voltage index

#include <stddef.h>
// flash operations that keep the overclock flash timing and run at <= 252 MHz (use instead of
// the SDK ones); begin/end lower clk_sys once around a series of operations
void mos_flash_slow_begin(void);
void mos_flash_slow_end(void);
void mos_flash_range_erase(uint32_t flash_offs, size_t count);
void mos_flash_range_program(uint32_t flash_offs, const uint8_t *data, size_t count);
void mos_flash_do_cmd(const uint8_t *txbuf, uint8_t *rxbuf, size_t count);
// default clock and core voltage before a watchdog reset / reboot
void mos_prepare_reset(void);
