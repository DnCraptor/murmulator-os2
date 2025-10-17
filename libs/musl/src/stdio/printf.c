#include <stdio.h>
#include <stdarg.h>

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

inline static void gouta(char* string) {
    typedef void (*t_ptr_t)(char*);
    ((t_ptr_t)_sys_table_ptrs[127])(string);
}

#undef printf
__attribute__((used, noinline, visibility("default")))
int printf(const char *restrict fmt, ...)
{
	gouta("printf\n");
	int ret;
	va_list ap;
	va_start(ap, fmt);
	ret = vfprintf(stdout, fmt, ap);
	va_end(ap);
	return ret;
}
