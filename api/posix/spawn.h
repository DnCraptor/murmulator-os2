#ifndef _SPAWN_H
#define _SPAWN_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

#include <errno.h>
#include <string.h>

/** TODO:
#include <features.h>

#define __NEED_mode_t
#define __NEED_pid_t
#define __NEED_sigset_t

#include <bits/alltypes.h>

struct sched_param;
*/
#define POSIX_SPAWN_RESETIDS        0x0001
#define POSIX_SPAWN_SETPGROUP       0x0002
#define POSIX_SPAWN_SETSIGDEF       0x0004
#define POSIX_SPAWN_SETSIGMASK      0x0008
#define POSIX_SPAWN_SETSCHEDPARAM   0x0010
#define POSIX_SPAWN_SETSCHEDULER    0x0020
#define POSIX_SPAWN_USEVFORK        0x0040
#define POSIX_SPAWN_SETSID          0x0080

typedef struct {
    int flags;          // bitmask of POSIX_SPAWN_*

    pid_t pgroup;       // for POSIX_SPAWN_SETPGROUP

    uid_t uid;          // for SETUID / RESETIDS
    uid_t euid;         // optional but useful

    gid_t gid;          // for SETGID / RESETIDS
    gid_t egid;

    unsigned long sigmask;   // stub for POSIX_SPAWN_SETSIGMASK
    unsigned long sigdefault;// stub for POSIX_SPAWN_SETSIGDEF

    int schedpolicy;    // stub for scheduler
    int schedprio;      // stub for scheduler parameter
} posix_spawnattr_t;

enum {
    ACTION_OPEN,
    ACTION_CLOSE,
    ACTION_DUP2
};

typedef struct {
    int type;
    // open
    int fd;
    const char *path;
    int oflag;
    int mode;
    // dup2
    int newfd;
} posix_spawn_file_action_t;

typedef struct {
    size_t count;
    posix_spawn_file_action_t *items;
} posix_spawn_file_actions_t;

inline static
int posix_spawn(
    pid_t *pid_out,
    const char *path,
    const posix_spawn_file_actions_t *actions,
    const posix_spawnattr_t *attr,
    char *const argv[],
    char *const envp[]
) {
	typedef int (*posix_spawn_t)(pid_t *__restrict, const char *__restrict, const posix_spawn_file_actions_t *,
		const posix_spawnattr_t *__restrict, char *const *__restrict, char *const *__restrict);
    return ((posix_spawn_t)_sys_table_ptrs[378])(pid_out, path, actions, attr, argv, envp);
}

inline static
int posix_spawnp(
    pid_t *pid_out,
    const char *path,
    const posix_spawn_file_actions_t *actions,
    const posix_spawnattr_t *attr,
    char *const argv[],
    char *const envp[]
) {
	typedef int (*posix_spawn_t)(pid_t *__restrict, const char *__restrict, const posix_spawn_file_actions_t *,
		const posix_spawnattr_t *__restrict, char *const *__restrict, char *const *__restrict);
    return ((posix_spawn_t)_sys_table_ptrs[396])(pid_out, path, actions, attr, argv, envp);
}

inline static
int posix_spawnattr_init(posix_spawnattr_t* attr) {
    if (!attr) return EINVAL;
    memset(attr, 0, sizeof(posix_spawnattr_t));
    return 0;
}

inline static
int posix_spawnattr_destroy(posix_spawnattr_t *) {
    return 0;
}

int posix_spawnattr_setflags(posix_spawnattr_t *, short);
int posix_spawnattr_getflags(const posix_spawnattr_t *__restrict, short *__restrict);

int posix_spawnattr_setpgroup(posix_spawnattr_t *, pid_t);
int posix_spawnattr_getpgroup(const posix_spawnattr_t *__restrict, pid_t *__restrict);

int posix_spawnattr_setsigmask(posix_spawnattr_t *__restrict, const sigset_t *__restrict);
int posix_spawnattr_getsigmask(const posix_spawnattr_t *__restrict, sigset_t *__restrict);

int posix_spawnattr_setsigdefault(posix_spawnattr_t *__restrict, const sigset_t *__restrict);
int posix_spawnattr_getsigdefault(const posix_spawnattr_t *__restrict, sigset_t *__restrict);

int posix_spawnattr_setschedparam(posix_spawnattr_t *__restrict, const struct sched_param *__restrict);
int posix_spawnattr_getschedparam(const posix_spawnattr_t *__restrict, struct sched_param *__restrict);
int posix_spawnattr_setschedpolicy(posix_spawnattr_t *, int);
int posix_spawnattr_getschedpolicy(const posix_spawnattr_t *__restrict, int *__restrict);

int posix_spawn_file_actions_init(posix_spawn_file_actions_t *);
int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *);

int posix_spawn_file_actions_addopen(posix_spawn_file_actions_t *__restrict, int, const char *__restrict, int, mode_t);
int posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *, int);
int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *, int, int);

#if defined(_BSD_SOURCE) || defined(_GNU_SOURCE)
int posix_spawn_file_actions_addchdir_np(posix_spawn_file_actions_t *__restrict, const char *__restrict);
int posix_spawn_file_actions_addfchdir_np(posix_spawn_file_actions_t *, int);
#endif


#ifdef __cplusplus
}
#endif

#endif
