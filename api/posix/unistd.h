// /usr/include/unistd.h
#ifndef _UNISTD_H_
#define _UNISTD_H_

#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

#define PATH_MAX 255

#ifndef AT_FDCWD
#define AT_FDCWD (-100)
#endif

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

/**
 * Closes a file descriptor, releasing any resources associated with it.
 *
 * @param fildes
 *     File descriptor obtained from a successful call to open(), creat(), dup(), 
 *     pipe(), or similar. It must be a valid, open descriptor belonging to the calling process.
 *
 * @return
 *     On success: returns 0.
 *     On failure: returns -1 and sets errno to indicate the error.
 *
 * @errors
 *     EBADF  - The file descriptor 'fd' is not valid or not open.
 *     EINTR  - The call was interrupted by a signal before closing completed.
 *     EIO    - An I/O error occurred while flushing data to the underlying device.
 *
 * @notes
 *     - After a successful call, the file descriptor becomes invalid for future use.
 *     - Any further read(), write(), or lseek() calls on a closed descriptor will fail with EBADF.
 *     - If multiple descriptors refer to the same open file description (via dup() or fork()),
 *       the underlying file is closed only after all such descriptors have been closed.
 */
inline static int close(int fildes) {
    typedef int (*fn_ptr_t)(int);
    return ((fn_ptr_t)_sys_table_ptrs[266])(fildes);
}

#include <stddef.h>

/**
 * Reads from a file descriptor.
 *
 * @param fildes  File descriptor to read from.
 * @param buf     Pointer to the buffer to store read data.
 * @param count   Maximum number of bytes to read.
 *
 * @return
 *     On success: returns the number of bytes read (0 indicates EOF).
 *     On failure: returns -1 and sets errno.
 *
 * @errors
 *     EBADF  - 'fd' is not valid or not open for reading.
 *     EFAULT - 'buf' points outside accessible address space.
 *     EINTR  - Interrupted by signal.
 */
inline static int read(int fildes, void* buf, size_t count) {
    typedef int (*fn_ptr_t)(int, void*, size_t);
    return ((fn_ptr_t)_sys_table_ptrs[270])(fildes, buf, count);
}
/**
 * Writes to a file descriptor.
 *
 * @param fildes  File descriptor to write to.
 * @param buf     Pointer to the buffer with data to write.
 * @param count   Number of bytes to write.
 *
 * @return
 *     On success: returns the number of bytes written.
 *     On failure: returns -1 and sets errno.
 *
 * @errors
 *     EBADF  - 'fd' is not valid or not open for writing.
 *     EFAULT - 'buf' points outside accessible address space.
 *     EINTR  - Interrupted by signal.
 */
inline static int write(int fildes, const void *buf, size_t count) {
    typedef int (*fn_ptr_t)(int, const void*, size_t);
    return ((fn_ptr_t)_sys_table_ptrs[271])(fildes, buf, count);
}

/**
 * Duplicates a file descriptor
 *
 * @param oldfd  Existing file descriptor.
 *
 * @return
 *     On success: returns newfd.
 *     On failure: returns -1 and sets errno.
 *
 * @errors
 *     EBADF  - oldfd is not valid.
 *     EMFILE - newfd is out of available range.
 */
inline static int dup(int oldfd) {
    typedef int (*fn_ptr_t)(int);
    return ((fn_ptr_t)_sys_table_ptrs[273])(oldfd);
}

/**
 * Duplicates a file descriptor to a specified new descriptor.
 *
 * @param oldfd  Existing file descriptor.
 * @param newfd  Desired file descriptor number.
 *
 * @return
 *     On success: returns newfd.
 *     On failure: returns -1 and sets errno.
 *
 * @errors
 *     EBADF  - oldfd is not valid.
 *     EMFILE - newfd is out of available range.
 */
inline static int dup2(int oldfd, int newfd) {
    typedef int (*fn_ptr_t)(int, int);
    return ((fn_ptr_t)_sys_table_ptrs[274])(oldfd, newfd);
}

#define SEEK_SET 0  /* Set file offset to 'offset' bytes from the beginning */
#define SEEK_CUR 1  /* Set file offset to current position plus 'offset' */
#define SEEK_END 2  /* Set file offset to file size plus 'offset' */

/**
 * Repositions the file offset of an open file descriptor.
 *
 * @param fd
 *     The file descriptor referring to an open file. Must have been obtained 
 *     from open(), creat(), pipe(), or a similar function.
 *
 * @param offset
 *     The number of bytes to offset the file position. Its interpretation 
 *     depends on the value of 'whence':
 *       - SEEK_SET: set the file offset to 'offset' bytes from the beginning.
 *       - SEEK_CUR: set the file offset to its current location plus 'offset'.
 *       - SEEK_END: set the file offset to the size of the file plus 'offset'.
 *
 * @param whence
 *     One of SEEK_SET, SEEK_CUR, or SEEK_END, indicating how the offset should be applied.
 *
 * @return
 *     On success: returns the resulting file offset (non-negative).
 *     On failure: returns (off_t)-1 and sets errno to indicate the error.
 *
 * @errors
 *     EBADF  - 'fd' is not an open file descriptor.
 *     EINVAL - 'whence' is invalid, or resulting offset would be negative.
 *     EOVERFLOW - The resulting file offset cannot be represented in off_t.
 *     ESPIPE - 'fd' refers to a pipe, FIFO, or socket (which do not support seeking).
 *
 * @notes
 *     - The new file position affects subsequent read() and write() operations.
 *     - If the file is shared (via dup() or fork()), all descriptors that share
 *       the same open file description will see the new offset.
 *     - Seeking past the end of the file does not extend it until data is written.
 */
inline static int lseek(int fd, int offset, int whence) {
    typedef int (*fn_ptr_t)(int, int, int);
    return ((fn_ptr_t)_sys_table_ptrs[275])(fd, offset, whence);
}

inline static int linkat(int fde, const char *existing, int fdn, const char *new, int flag) {
    typedef int (*fn_ptr_t)(int, const char *, int, const char *, int);
    return ((fn_ptr_t)_sys_table_ptrs[339])(fde, existing, fdn, new, flag);
}
inline static int link(const char *existing, const char *new) {
    return linkat(AT_FDCWD, existing, AT_FDCWD, new, 0);
}

inline static int unlinkat(int dirfd, const char *pathname, int flags) {
    typedef int (*fn_ptr_t)(int, const char*, int);
    return ((fn_ptr_t)_sys_table_ptrs[306])(dirfd, pathname, flags);
}
inline static int unlink(const char *existing) {
    return unlinkat(AT_FDCWD, existing, AT_SYMLINK_NOFOLLOW);
}

inline static int symlinkat(const char *existing, int fd, const char *new) {
    typedef int (*fn_ptr_t)(const char *, int, const char *);
    return ((fn_ptr_t)_sys_table_ptrs[340])(existing, fd, new);
}
inline static int symlink(const char *existing, const char *new) {
    return symlinkat(existing, AT_FDCWD, new);
}

inline static long readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz) {
    typedef long (*fn_ptr_t)(int, const char*, char*, size_t);
    return ((fn_ptr_t)_sys_table_ptrs[302])(dirfd, pathname, buf, bufsiz);
}

inline static long readlink(const char *restrict path, char *restrict buf, size_t bufsize) {
    return readlinkat(AT_FDCWD, path, buf, bufsize);
}
/* TODO:
pid_t fork(void);
int execve(const char *pathname, char *const argv[], char *const envp[]);
*/

#ifdef __cplusplus
}
#endif

#endif /* !_UNISTD_H_ */
