#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <fts.h>
#include "sys_table.h"

FTS* __libc() fts_open(char* const* argv, int options, int (*compar)(const FTSENT**, const FTSENT**)) {
    if (options & ~FTS_OPTIONMASK) {
        errno = EINVAL;
        return NULL;
    }
    if (!argv || !*argv) {
        errno = EINVAL;
        return NULL;
    }
	/// TODO:
        errno = EINVAL;
        return NULL;
}
