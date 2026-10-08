#ifndef _TIME_H_
#define _TIME_H_

#include <stddef.h>
#include <sys/types.h>

typedef long clock_t;

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

#define CLOCKS_PER_SEC 1000000L

time_t time(time_t *);
struct tm *localtime(const time_t *);

#endif
