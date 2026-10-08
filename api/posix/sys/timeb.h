#ifndef _SYS_TIMEB_H_
#define _SYS_TIMEB_H_

#include <sys/types.h>

struct timeb {
    time_t time;
    unsigned short millitm;
    short timezone;
    short dstflag;
};

int ftime(struct timeb *);

#endif
