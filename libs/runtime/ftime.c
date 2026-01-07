#include <sys/timeb.h>
#include <time.h>
#include <errno.h>

static inline uint64_t time_us_64(void) {
    typedef uint64_t (*fn_ptr_t)(void);
    return ((fn_ptr_t)_sys_table_ptrs[263])();
}

int ftime(struct timeb *tb)
{
    if (!tb) {
        errno = EINVAL;
        return -1;
    }

    uint64_t us = time_us_64();

    tb->time     = (time_t)(us / 1000000);
    tb->millitm  = (unsigned short)((us / 1000) % 1000);
    tb->timezone = 0;
    tb->dstflag  = 0;

    return 0;
}
