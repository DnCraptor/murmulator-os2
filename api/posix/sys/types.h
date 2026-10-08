#ifndef _SYS_TYPES_H_
#define _SYS_TYPES_H_

#include <stddef.h>
#include <stdint.h>

#ifndef __TINYC__
typedef __PTRDIFF_TYPE__ ssize_t;
#endif

#ifndef _MODE_T_DECLARED
#define _MODE_T_DECLARED
typedef unsigned int mode_t;
#endif

#ifndef _OFF_T_DECLARED
#define _OFF_T_DECLARED
typedef long long off_t;
#define __machine_off_t_defined
#endif

#ifndef _TIME_T_DECLARED
#define _TIME_T_DECLARED
typedef long time_t;
#endif

#ifndef _SUSECONDS_T_DECLARED
#define _SUSECONDS_T_DECLARED
typedef long suseconds_t;
#endif

#ifndef _USECONDS_T_DECLARED
#define _USECONDS_T_DECLARED
typedef unsigned long useconds_t;
#endif

#ifndef _PID_T_DECLARED
#define _PID_T_DECLARED
typedef long pid_t;
#endif

#ifndef _DEV_T_DECLARED
#define _DEV_T_DECLARED
typedef unsigned long dev_t;
#endif

#ifndef _INO_T_DECLARED
#define _INO_T_DECLARED
typedef unsigned long ino_t;
#endif

#ifndef _NLINK_T_DECLARED
#define _NLINK_T_DECLARED
typedef unsigned int nlink_t;
#endif

#ifndef _UID_T_DECLARED
#define _UID_T_DECLARED
typedef unsigned int uid_t;
#endif

#ifndef _GID_T_DECLARED
#define _GID_T_DECLARED
typedef unsigned int gid_t;
#endif

#endif
