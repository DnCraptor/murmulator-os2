/* Existing MOS wall-clock API; no new kernel entry is required. */
#include <time.h>
#include <stddef.h>
#include <errno.h>

time_t time(time_t *result)
{
    typedef time_t (*fn_t)(time_t *);
    return ((fn_t)_sys_table_ptrs[261])(result);
}

