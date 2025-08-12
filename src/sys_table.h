#pragma once

#ifndef _SYS_TABLE_H
#define _SYS_TABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#define __in_systable(group) __attribute__((section(".sys_table" group)))
#define __in_hfa(group) __attribute__((section(".high_flash" group)))

extern unsigned long __in_systable() __aligned(4096) sys_table_ptrs[];

#ifdef __cplusplus
}
#endif

#endif
