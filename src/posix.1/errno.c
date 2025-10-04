#include "errno.h"

static int global_errno = 0;

int* __errno_location() {
    // TODO: per process from context
    return &global_errno;
}
