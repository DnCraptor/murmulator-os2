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

inline static void set_sys_clock_pll(uint32_t vco_freq,
                                     uint32_t post_div1,
                                     uint32_t post_div2) {
    ((clock_u32_u32_u32_fn_t)_sys_table_ptrs[105])(
        vco_freq, post_div1, post_div2);
}
#endif
