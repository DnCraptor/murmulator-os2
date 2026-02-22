#include <FreeRTOS.h>
#include <task.h>

#include <stdarg.h>

#include "sys/ioctl.h"
#include "errno.h"
#include "graphics.h"

int __ioctl(int fd, unsigned long request, void* opt) {
    if (request == TIOCGWINSZ && (fd == 1 || fd == 2)) {
        struct winsize* ws = (struct winsize*)opt;
        ws->ws_col    = (unsigned short)get_console_width();
        ws->ws_row    = (unsigned short)get_console_height();
        ws->ws_xpixel = (unsigned short)get_screen_width();
        ws->ws_ypixel = (unsigned short)get_screen_height();
        errno = 0;
        return 0;
    }
    /// TODO:
    errno = EINVAL;
    return -1;
}
