#include <poll.h>
#include <stdio.h>

int main(void)
{
    struct pollfd pfd = {
        .fd = 0,
        .events = POLLIN
    };

    printf("press key...\n");

    int r = poll(&pfd, 1, -1);
    if (r <= 0) {
        printf("poll failed\n");
        return 1;
    }

    if (!(pfd.revents & POLLIN)) {
        printf("no POLLIN\n");
        return 1;
    }

    int c = getchar();
    printf("got %c\n", c);
    printf("poll OK\n");
    return 0;
}
