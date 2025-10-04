// /usr/include/sys/stat.h
#ifndef _SYS_STAT_H_
#define	_SYS_STAT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* File permission bits */
#define S_IRUSR  0400  /* read permission, owner */
#define S_IWUSR  0200  /* write permission, owner */
#define S_IXUSR  0100  /* execute/search permission, owner */
#define S_IRGRP  0040  /* read permission, group */
#define S_IWGRP  0020  /* write permission, group */
#define S_IXGRP  0010  /* execute/search permission, group */
#define S_IROTH  0004  /* read permission, others */
#define S_IWOTH  0002  /* write permission, others */
#define S_IXOTH  0001  /* execute/search permission, others */

#ifndef _MODE_T_DECLARED
#define _MODE_T_DECLARED
typedef unsigned int mode_t;
#endif

#ifdef __cplusplus
}
#endif

#endif /* !_SYS_STAT_H_ */
