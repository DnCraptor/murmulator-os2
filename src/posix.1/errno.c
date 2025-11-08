#include "errno.h"
#include "sys_table.h"

static int global_errno = 0;

int* __in_hfa() __errno_location() {
    // TODO: per process from context
    return &global_errno;
}
