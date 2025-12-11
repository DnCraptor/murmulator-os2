#ifndef _SYS_WAIT_H
#define _SYS_WAIT_H

#include <sys/types.h>   /* for size_t, ssize_t, etc. */

/* Begin extern "C" if C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	P_ALL = 0,
	P_PID = 1,
	P_PGID = 2,
	P_PIDFD = 3
} idtype_t;

#ifndef _PID_T_DECLARED
typedef long pid_t;
typedef long _pid_t;
#define __machine_off_t_defined
#define	_PID_T_DECLARED
#endif

pid_t __waitpid (pid_t pid, int* pstatus, int options);

/* End extern "C" if C++ */
#ifdef __cplusplus
}
#endif

#endif /* _SYS_WAIT_H */
