#define _GNU_SOURCE
#include <fcntl.h>
#include "syscall.h"

#ifndef __SYSCALL_LL_O
#define __SYSCALL_LL_E(x) \
((union { long long ll; long l[2]; }){ .ll = x }).l[0], \
((union { long long ll; long l[2]; }){ .ll = x }).l[1]
#define __SYSCALL_LL_O(x) 0, __SYSCALL_LL_E((x))
#endif

ssize_t readahead(int fd, off_t pos, size_t len)
{
	return syscall(SYS_readahead, fd, __SYSCALL_LL_O(pos), len);
}
