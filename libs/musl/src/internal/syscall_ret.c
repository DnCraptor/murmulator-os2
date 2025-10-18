#include <errno.h>
#include "syscall.h"

long __syscall_ret(unsigned long r)
{
	kprintf("__syscall_ret: %d\n", r);
	if (r > -4096UL) {
		errno = -r;
		return -1;
	}
	return r;
}
