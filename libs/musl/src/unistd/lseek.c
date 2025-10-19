#include <unistd.h>
#include "syscall.h"

off_t __lseek(int fd, off_t offset, int whence)
{
#ifdef SYS__llseek
	off_t result;
	return syscall(SYS__llseek, fd, (unsigned long)(offset >> 32), (unsigned long)offset, &result, whence) ? -1 : result;
#else
	return syscall(SYS_lseek, fd, offset, whence);
#endif
}

weak_alias(__lseek, lseek);
