#include "libc.h"

struct __libc __libc = {
    .can_do_threads = 0,
    .threaded = 0,
    .secure = 0,
    .need_locks = 0,
    .threads_minus_1 = 0,
    .auxv = 0,
    .tls_head = 0,
    .tls_size = 0,
    .tls_align = 0,
    .tls_cnt = 0,
    .page_size = 4096,
    .global_locale = 0
};

size_t __hwcap;
char *__progname=0, *__progname_full=0;

weak_alias(__progname, program_invocation_short_name);
weak_alias(__progname_full, program_invocation_name);
