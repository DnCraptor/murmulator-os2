// /usr/include/unistd.h
#ifndef _UNISTD_H_
#define _UNISTD_H_


#ifdef __cplusplus
extern "C" {
#endif


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
int __close(int fildes);
inline static int close(int fildes) {
    return __close(fildes);
}

#include <stddef.h>

int __read(int fildes, void *buf, size_t count);
int __write(int fildes, const void *buf, size_t count);

/* TODO:
off_t lseek(int fd, off_t offset, int whence);
pid_t fork(void);
int execve(const char *pathname, char *const argv[], char *const envp[]);
*/

#ifdef __cplusplus
}
#endif

#endif /* !_UNISTD_H_ */
