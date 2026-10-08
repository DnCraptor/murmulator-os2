/* MOS currently exposes whole wall-clock seconds only. */
#include <time.h>
#include <sys/time.h>
#include <errno.h>

int gettimeofday(struct timeval *restrict tv, void *restrict tz)
{
    time_t now;
    if (!tv || tz) {
        errno = EINVAL;
        return -1;
    }
    now = time(NULL);
    if (now == (time_t)-1)
        return -1;
    tv->tv_sec = now;
    /* Existing MOS API exports only whole wall-clock seconds.  The phase
     * of the boot timer is unrelated; do not invent fractional precision. */
    tv->tv_usec = 0;
    return 0;
}

