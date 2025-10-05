// /usr/include/sys/fcntl.h
#ifndef _FCNTL_H_
#define	_FCNTL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

#ifndef _MODE_T_DECLARED
#define _MODE_T_DECLARED
typedef unsigned int mode_t;
#endif

/* File access modes */
#define O_RDONLY    0x0000  /* open for reading only */
#define O_WRONLY    0x0001  /* open for writing only */
#define O_RDWR      0x0002  /* open for reading and writing */

/* File creation and status flags */
#define O_CREAT     0x0040  /* create file if it does not exist */
#define O_EXCL      0x0080  /* error if O_CREAT and the file exists */
#define O_TRUNC     0x0200  /* truncate file to zero length */
#define O_APPEND    0x0400  /* append on each write */
#define O_NONBLOCK  0x0800  /* non-blocking mode */
#define O_SYNC      0x1000  /* write according to synchronized I/O file integrity completion */
#define O_NOFOLLOW  0x2000  /* do not follow symbolic links */

/**
 * Opens a file and returns a file descriptor for subsequent I/O operations.
 *
 * @param path
 *     Path to the file to be opened. Can be absolute or relative.
 *
 * @param oflag
 *     File access mode and options. Must include exactly one of:
 *       - O_RDONLY : open for reading only
 *       - O_WRONLY : open for writing only
 *       - O_RDWR   : open for reading and writing
 *     Additional flags may be combined using bitwise OR, such as:
 *       - O_CREAT  : create file if it does not exist (requires 'mode')
 *       - O_EXCL   : with O_CREAT, fail if file already exists
 *       - O_TRUNC  : truncate existing file to length 0
 *       - O_APPEND : append all writes to the end of file
 *       - O_NONBLOCK, O_SYNC, O_NOFOLLOW, etc.
 *
 * @param ...
 *     Optional argument of type mode_t, required if O_CREAT is specified.
 *     Defines the file's permissions (e.g., 0644), modified by the process umask.
 *
 * @return
 *     On success: non-negative file descriptor.
 *     On failure: -1 is returned and errno is set appropriately.
 *
 * @errors
 *     EACCES  - Permission denied.
 *     EEXIST  - File exists and O_CREAT|O_EXCL was used.
 *     ENOENT  - File does not exist and O_CREAT not specified.
 *     ENOTDIR - A path component is not a directory.
 *     EISDIR  - Tried to open a directory for writing.
 *     EMFILE  - Process limit of open files reached.
 *     ENFILE  - System-wide limit of open files reached.
 *     EINVAL  - Invalid flags.
 */
int __open(const char *path, int oflag, mode_t mode);
inline static int open(const char *path, int oflag, ...) {
    va_list ap;
    mode_t mode = 0;
    va_start(ap, oflag);
    /* mode only if O_CREAT */
    if (oflag & O_CREAT) {
        mode = va_arg(ap, mode_t);
    }
    va_end(ap);
    return __open(path, oflag, mode);
}

#ifdef __cplusplus
}
#endif

#endif /* !_FCNTL_H_ */
