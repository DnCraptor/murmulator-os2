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
