#include <sys/socket.h>
#include "syscall.h"

int bind(int fd, const struct sockaddr *addr, socklen_t len)
{
/// TODO:	return socketcall(bind, fd, addr, len, 0, 0, 0);
	return -1;
}
