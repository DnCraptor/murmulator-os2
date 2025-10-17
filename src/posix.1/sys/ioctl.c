#include <FreeRTOS.h>
#include <task.h>

#include <stdarg.h>

#include "sys/ioctl.h"
#include "errno.h"

int __ioctl(int fd, unsigned long request, ...) {
    if (request == TIOCGWINSZ && (fd == 1 || fd == 2)) {
    	va_list ap;
	    va_start(ap, request);
        struct winsize* ws = va_arg(ap, struct winsize*);
        /// TODO: graphics_driver_t??
        ws->ws_col = 80;
        ws->ws_row = 30;
        ws->ws_xpixel = 640;
        ws->ws_ypixel = 480;
        va_end(ap);
        errno = 0;
        return 0;
    }
    /// TODO:
    errno = EINVAL;
    return -1;
}

