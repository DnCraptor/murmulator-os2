#ifndef _SYS_TIME_H_
#define _SYS_TIME_H_

#include <sys/types.h>

struct timeval {
    time_t tv_sec;
    suseconds_t tv_usec;
};

int gettimeofday(struct timeval *__restrict, void *__restrict);

#endif
