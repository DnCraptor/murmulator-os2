#ifndef __system_clock_h__
#define __system_clock_h__

#include <stdint.h>

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void *)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs =
    (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

typedef void (*clock_void_fn_t)(void);
typedef void (*clock_u32_fn_t)(uint32_t);
typedef void (*clock_u32_u32_u32_fn_t)(uint32_t, uint32_t, uint32_t);
typedef uint32_t (*clock_get_u32_fn_t)(void);

inline static void overclocking(void) {
    ((clock_void_fn_t)_sys_table_ptrs[101])();
}

inline static uint32_t get_overclocking_khz(void) {
    return ((clock_get_u32_fn_t)_sys_table_ptrs[103])();
}

inline static void set_overclocking(uint32_t khz) {
    ((clock_u32_fn_t)_sys_table_ptrs[104])(khz);
}

// core voltage, mV (API v29)
inline static int get_vreg_mv(void) { // current
    return ((int (*)(void))_sys_table_ptrs[405])();
}
inline static void set_vreg_mv(int mv) { // 0 - automatic (by frequency); applied by overclocking()
    ((void (*)(int))_sys_table_ptrs[406])(mv);
}
inline static int get_vreg_override_mv(void) { // 0 - automatic
    return ((int (*)(void))_sys_table_ptrs[407])();
}
inline static int parse_vreg_mv(const char* t) { // "1.60", "1600", "AUTO" -> mV (0 - automatic)
    return ((int (*)(const char*))_sys_table_ptrs[408])(t);
}

inline static void set_sys_clock_pll(uint32_t vco_freq,
                                     uint32_t post_div1,
                                     uint32_t post_div2) {
    ((clock_u32_u32_u32_fn_t)_sys_table_ptrs[105])(
        vco_freq, post_div1, post_div2);
}
#endif
