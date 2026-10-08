/* MOS application-local calendar support.
 * TODO(kernel): export calendar conversion and gettimeofday, and provide
 * an explicitly initialized wall clock with timezone configuration.
 * Slot 261 is newlib time(time_t *), NOT unsigned time(unsigned).
 * The present kernel does not initialize its wall clock.  Do not use that
 * clock for compiler date macros; SOURCE_DATE_EPOCH is their explicit source.
 */
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include "mos_time.h"

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void *)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs =
    (const unsigned long *)M_OS_API_SYS_TABLE_BASE;
#endif

time_t time(time_t *result)
{
    typedef time_t (*fn_t)(time_t *);
    return ((fn_t)_sys_table_ptrs[261])(result);
}

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

unsigned mos_clock_ms(void)
{
    typedef uint64_t (*fn_t)(void);
    return (unsigned)(((fn_t)_sys_table_ptrs[263])() / 1000);
}

/* Proleptic Gregorian calendar, UTC.  MOS has no timezone/DST API yet.
 * Split into 400-year eras so negative timestamps and century leap rules
 * work without iteration over years. */
struct tm *localtime(const time_t *value)
{
    static struct tm result;
    int64_t days, rem, z, era, year;
    unsigned doe, yoe, doy, mp, month, day;
    int leap;
    if (!value) {
        errno = EINVAL;
        return NULL;
    }
    days = (int64_t)*value / 86400;
    rem = (int64_t)*value % 86400;
    if (rem < 0) {
        rem += 86400;
        --days;
    }
    /* Keep tm_year representable even for 64-bit time_t. */
    if (days < -719162 || days > 2932896) {
        errno = EOVERFLOW;
        return NULL;
    }
    z = days + 719468;
    era = (z >= 0 ? z : z - 146096) / 146097;
    doe = (unsigned)(z - era * 146097);
    yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    year = (int64_t)yoe + era * 400;
    doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    mp = (5 * doy + 2) / 153;
    day = doy - (153 * mp + 2) / 5 + 1;
    month = mp < 10 ? mp + 3 : mp - 9;
    year += month <= 2;
    leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    result = (struct tm){0};
    result.tm_year = (int)year - 1900;
    result.tm_mon = (int)month - 1;
    result.tm_mday = (int)day;
    result.tm_hour = (int)(rem / 3600);
    result.tm_min = (int)(rem / 60 % 60);
    result.tm_sec = (int)(rem % 60);
    result.tm_wday = (int)((days + 4) % 7);
    if (result.tm_wday < 0)
        result.tm_wday += 7;
    {
        static const unsigned short before_month[] =
            {0,31,59,90,120,151,181,212,243,273,304,334};
        result.tm_yday = before_month[month - 1] + day - 1
                      + (leap && month > 2);
    }
    result.tm_isdst = 0;
    return &result;
}

int mos_tcc_timestamp(time_t *value)
{
    const char *p = getenv("SOURCE_DATE_EPOCH");
    uint64_t seconds = 0;
    if (!p)
        return -1;
    if (!*p)
        return -2;
    for (; *p; ++p) {
        unsigned digit = (unsigned)(*p - '0');
        if (digit > 9 || seconds > (UINT64_C(253402300799) - digit) / 10)
            return -2;
        seconds = seconds * 10 + digit;
    }
    *value = (time_t)seconds;
    if (*value < 0 || (uint64_t)*value != seconds)
        return -2;
    return 0;
}
