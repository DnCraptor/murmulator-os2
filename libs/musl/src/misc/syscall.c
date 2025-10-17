#define _BSD_SOURCE
#include <unistd.h>
#include "syscall.h"
#include <stdarg.h>

#undef syscall

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

typedef void (*goutf_ptr_t)(const char *__restrict str, ...) __attribute__ ((__format__ (__printf__, 1, 2)));
#define kprintf(...) ((goutf_ptr_t)_sys_table_ptrs[41])(__VA_ARGS__)

typedef syscall_arg_t (*fn_ptr_t)(syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t);

/// W/A's TODO:
long __syscall(long n, ...) {
	if (n >= 512 || !_sys_table_ptrs[n]) {
		kprintf("__syscall(%ph, ...)\n", n);
		return 0;
	}
	va_list ap;
	syscall_arg_t a,b,c,d,e,f;
	va_start(ap, n);
	a=va_arg(ap, syscall_arg_t);
	b=va_arg(ap, syscall_arg_t);
	c=va_arg(ap, syscall_arg_t);
	d=va_arg(ap, syscall_arg_t);
	e=va_arg(ap, syscall_arg_t);
	f=va_arg(ap, syscall_arg_t);
	va_end(ap);
	return ((fn_ptr_t)_sys_table_ptrs[n])(a,b,c,d,e,f);
}

long syscall(long n, ...)
{
	if (n >= 512 || !_sys_table_ptrs[n]) {
		kprintf("syscall(%ph, ...)\n", n);
		return 0;
	}
	va_list ap;
	syscall_arg_t a,b,c,d,e,f;
	va_start(ap, n);
	a=va_arg(ap, syscall_arg_t);
	b=va_arg(ap, syscall_arg_t);
	c=va_arg(ap, syscall_arg_t);
	d=va_arg(ap, syscall_arg_t);
	e=va_arg(ap, syscall_arg_t);
	f=va_arg(ap, syscall_arg_t);
	va_end(ap);
	return ((fn_ptr_t)_sys_table_ptrs[n])(a,b,c,d,e,f);
//	return __syscall_ret(__syscall(n,a,b,c,d,e,f));
}

long syscall_cp(long n, ...) {
	if (n >= 512 || !_sys_table_ptrs[n]) {
		kprintf("syscall_cp(%ph, ...)\n", n);
		return 0;
	}
	va_list ap;
	syscall_arg_t a,b,c,d,e,f;
	va_start(ap, n);
	a=va_arg(ap, syscall_arg_t);
	b=va_arg(ap, syscall_arg_t);
	c=va_arg(ap, syscall_arg_t);
	d=va_arg(ap, syscall_arg_t);
	e=va_arg(ap, syscall_arg_t);
	f=va_arg(ap, syscall_arg_t);
	va_end(ap);
	return ((fn_ptr_t)_sys_table_ptrs[n])(a,b,c,d,e,f);
}
