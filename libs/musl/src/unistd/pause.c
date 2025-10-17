#include <unistd.h>
#include "syscall.h"

int pause(void)
{
///	return sys_pause_cp();
return __syscall_ret(__syscall_cp(SYS_ppoll, 0, 0, 0, 0, 0, 0));
}
