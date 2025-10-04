#ifndef _ERRNO_H_
#define _ERRNO_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Standard POSIX error codes */
#define EACCES   13  /* Permission denied */
#define EEXIST   17  /* File exists and O_CREAT|O_EXCL was used */
#define ENOENT    2  /* File does not exist */
#define ENOTDIR  20  /* A path component is not a directory */
#define EISDIR   21  /* Tried to open a directory for writing */
#define EMFILE   24  /* Process limit of open files reached */
#define ENFILE   23  /* System-wide limit of open files reached */
#define EINVAL   22  /* Invalid argument or flags */
#define EIO       5  /* Input/output error */

/* You can define other standard POSIX errors here as needed */
int* __errno_location();
#define errno (*__errno_location())

#ifdef __cplusplus
}
#endif

#endif /* _ERRNO_H_ */
