// kbd_posix.c
#include <unistd.h>
#include <sys/fcntl.h>
#include <stdio.h>

char kbdcheckch(void) {
    int fd = fileno(stdin);

    int fl = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, fl | O_NONBLOCK);

    unsigned char ch;
    ssize_t n = read(fd, &ch, 1);

    fcntl(fd, F_SETFL, fl);

    if (n == 1 && (ch == 0x03 || ch == 0x1B))
        return ch;

    return 0;
}

char kbdread(void) {
    int c;
    do {
        c = getchar();   // BLOCKING
    } while (c == EOF);
    return (char)c;
}

void kbdbegin() { }

char kbdavailable() { return 1; }
