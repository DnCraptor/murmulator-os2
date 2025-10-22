#include <stdarg.h>
#include "sys_table.h"
#include "internal/__stdio.h"

int __libc() __vprintf(const char *restrict fmt, va_list ap) {
	return __vfprintf(stdout, fmt, ap);
}

int __libc() __printf(const char *restrict fmt, ...) {
	int ret;
	va_list ap;
	va_start(ap, fmt);
	ret = __vfprintf(stdout, fmt, ap);
	va_end(ap);
	return ret;
}
