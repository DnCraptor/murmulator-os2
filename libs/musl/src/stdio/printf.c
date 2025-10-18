#include <stdio.h>
#include <stdarg.h>

#undef printf
__attribute__((used, noinline, visibility("default")))
int printf(const char *restrict fmt, ...)
{
	int ret;
	va_list ap;
	va_start(ap, fmt);
	ret = vfprintf(stdout, fmt, ap);
	va_end(ap);
	return ret;
}
