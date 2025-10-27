#include <FreeRTOS.h>
#include <task.h>

#include "sys/fcntl.h"
#include "sys/stat.h"
#include "errno.h"
#include "unistd.h"

#include "ff.h"
#include "../../api/m-os-api-c-array.h"

#include <time.h>
#include <stdint.h>
#include <string.h>
#include "sys_table.h"
char* copy_str(const char* s); // cmd

/// TODO: by process ctx
void __in_hfa() __getcwd(char *buff, UINT len) {
    buff[0] = '/';
    buff[1] = 0;
}

// from libc (may be required to have own one?)
char* __realpath(const char *restrict filename, char *restrict resolved);

static int __in_hfa() is_leap_year(int year) {
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

static const int days_in_month[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

// TODO: optimize it
time_t __in_hfa() simple_mktime(struct tm *t) {
    int year = t->tm_year + 1900;
    int mon = t->tm_mon;   // 0-11
    int day = t->tm_mday;  // 1-31
    int i;
    time_t seconds = 0;

    // add seconds for years since 1970
    for (i = 1970; i < year; ++i) {
        seconds += 365 * 24*3600;
        if (is_leap_year(i)) seconds += 24*3600;
    }

    // add seconds for months this year
    for (i = 0; i < mon; ++i) {
        seconds += days_in_month[i] * 24*3600;
        if (i == 1 && is_leap_year(year)) seconds += 24*3600; // Feb in leap year
    }

    // add seconds for days
    seconds += (day-1) * 24*3600;

    // add hours, minutes, seconds
    seconds += t->tm_hour * 3600;
    seconds += t->tm_min  * 60;
    seconds += t->tm_sec;

    return seconds;
}

static time_t __in_hfa() fatfs_to_time_t(WORD fdate, WORD ftime) {
    struct tm t;
    // FAT date: bits 15–9 year since 1980, 8–5 month, 4–0 day
    t.tm_year = ((fdate >> 9) & 0x7F) + 80; // years since 1900
    t.tm_mon  = ((fdate >> 5) & 0x0F) - 1;  // months 0–11
    t.tm_mday = fdate & 0x1F;

    // FAT time: bits 15–11 hour, 10–5 min, 4–0 sec/2
    t.tm_hour = (ftime >> 11) & 0x1F;
    t.tm_min  = (ftime >> 5) & 0x3F;
    t.tm_sec  = (ftime & 0x1F) * 2;

    t.tm_isdst = 0; // no DST info in FAT
    return simple_mktime(&t); // mktime(&t);
}

typedef struct {
    char type;
    uint32_t hash;
    char* fname;
    // for 'H' links only
    uint32_t ohash;
    char* ofname;
} posix_link_t;

static posix_link_t* posix_links = 0;
static size_t posix_links_cnt = 0;

static void __in_hfa() posix_add_link(uint32_t hash, const char* path, char type, uint32_t ohash, const char* opath, bool allocated) {
    posix_link_t* lnk = (posix_link_t*)pvPortMalloc((posix_links_cnt + 1) * sizeof(posix_link_t));
    if (posix_links) {
        memcpy(lnk, posix_links, posix_links_cnt * sizeof(posix_link_t));
        vPortFree(posix_links);
        posix_links = lnk;
        lnk += posix_links_cnt;
    } else {
        posix_links = lnk;
    }
    ++posix_links_cnt;
    lnk->type = type;
    lnk->hash = hash;
    lnk->fname = allocated ? path : copy_str(path);
    if (type == 'H') {
        lnk->ohash = ohash;
        lnk->ofname = allocated ? opath : copy_str(opath);
    } else {
        lnk->ohash = 0;
        lnk->ofname = 0;
    }
}

static FRESULT __in_hfa() extfs_flush() {
    FIL* pf = (FIL*)pvPortMalloc(sizeof(FIL));
    if (!pf) { errno = ENOMEM; return -1; }
    FRESULT r = FR_OK;
    vTaskSuspendAll();
    posix_link_t* lnk = posix_links;
    if (!lnk) {
        goto ok;
    }
    r = f_open(pf, "/.extfs", FA_CREATE_ALWAYS | FA_WRITE);
    if (r != FR_OK) goto ex;
    for (uint32_t i = 0; i < posix_links_cnt; ++i, ++lnk) {
        UINT bw;
        f_write(pf, &lnk->type, 1, &bw);
        r = f_write(pf, &lnk->hash, sizeof(lnk->hash), &bw);
        if (r != FR_OK) { /// TODO: error handling
            r = FR_DISK_ERR;
            goto ex;
        }
        uint16_t sz = (uint16_t)strlen(lnk->fname);
        f_write(pf, &sz, sizeof(sz), &bw);
        f_write(pf, lnk->fname, sz, &bw);
        if (lnk->type == 'H') {
            f_write(pf, &lnk->ohash, sizeof(lnk->ohash), &bw);
            sz = (uint16_t)strlen(lnk->ofname);
            f_write(pf, &sz, sizeof(sz), &bw);
            f_write(pf, lnk->ofname, sz, &bw);
        }
    }
ex:
    f_close(pf);
ok:
    vPortFree(pf);
	xTaskResumeAll();
    return r;
}

static posix_link_t* __in_hfa() lookup_by_orig(const char* opath, uint32_t ohash) {
    posix_link_t* lnk = posix_links;
    if (!lnk) return 0;
    for (uint32_t i = 0; i < posix_links_cnt; ++i, ++lnk) {
        if (lnk->ofname && lnk->type == 'H' &&  lnk->ohash == ohash && strcmp(opath, lnk->ofname) == 0) {
            return lnk;
        }
    }
    return 0;
}

static void __in_hfa() replace_orig(const char* opath, uint32_t ohash, posix_link_t* rename_to) {
    posix_link_t* lnk = posix_links;
    if (!lnk) return;
    /// TODO: last 'H' case
    for (uint32_t i = 0; i < posix_links_cnt; ++i, ++lnk) {
        if (lnk->ofname && lnk->type == 'H' &&  lnk->ohash == ohash && strcmp(opath, lnk->ofname) == 0) {
            lnk->ohash = rename_to->hash;
            vPortFree(lnk->ofname);
            lnk->ofname = copy_str(rename_to->fname);
        }
    }
    return;
}

static bool __in_hfa() posix_unlink(const char* path, uint32_t hash, posix_link_t** rename_to) {
    posix_link_t* lnk = posix_links;
    if (!lnk) return false; // nothing
    for (uint32_t i = 0; i < posix_links_cnt; ++i, ++lnk) {
        if (lnk->hash == hash && strcmp(path, lnk->fname) == 0) {
            if (lnk->type == 'O' && rename_to) { // original removement (additional handling is required)
                *rename_to = lookup_by_orig(hash, path);
                if (*rename_to) {
                    (*rename_to)->type = 'O';
                    replace_orig(hash, path, *rename_to);
                }
            }
            vPortFree(lnk->fname);
            if (posix_links_cnt - i > 1) {
                memmove(lnk, lnk + 1, sizeof(posix_link_t) * (posix_links_cnt - i - 1));
            }
            --posix_links_cnt;
            return true; // flush is required
        }
    }
    return false; // nothing
}

static bool __in_hfa() is_symlink(const char* path, uint32_t hash) {
    vTaskSuspendAll();
    posix_link_t* lnk = posix_links;
    if (!lnk) goto err; // nothing
    for (uint32_t i = 0; i < posix_links_cnt; ++i, ++lnk) {
        if (lnk->hash == hash && strcmp(path, lnk->fname) == 0) {
            char type = lnk->type;
        	xTaskResumeAll();
            return type == 'S';
        }
    }
err:
	xTaskResumeAll();
    return false; // nothing
}
static FRESULT __in_hfa() extfs_add_link(const char* path, uint32_t hash, char type, const char *opath, uint32_t ohash) {
    FIL* pf = (FIL*)pvPortMalloc(sizeof(FIL));
    if (!pf) { errno = ENOMEM; return -1; }
    vTaskSuspendAll();
    posix_add_link(hash, path, type, opath, opath, false);
    FRESULT r = f_open(pf, "/.extfs", FA_OPEN_ALWAYS | FA_WRITE);
    if (r != FR_OK) goto ex;
    f_lseek(pf, f_size(pf));
    UINT bw;
    f_write(pf, &type, 1, &bw);
    r = f_write(pf, &hash, sizeof(hash), &bw);
    if (r != FR_OK) { /// TODO: error handling
        goto err;
    }
    uint16_t sz = (uint16_t)strlen(path);
    f_write(pf, &sz, sizeof(sz), &bw);
    f_write(pf, path, sz, &bw);
    if (type == 'H') {
        f_write(pf, &ohash, sizeof(ohash), &bw);
        sz = (uint16_t)strlen(opath);
        f_write(pf, &sz, sizeof(sz), &bw);
        f_write(pf, opath, sz, &bw);
    }
err:
    f_close(pf);
ex:
    vPortFree(pf);
	xTaskResumeAll();
    return (r == FR_OK) ? FR_OK : FR_DISK_ERR;
}

static FRESULT __in_hfa() extfs_add_hlink(const char *path, uint32_t hash, const char *opath, uint32_t ohash) {
    FRESULT r;
    vTaskSuspendAll();
    bool found_orig = false;
    posix_link_t* lnk = posix_links;
    for (uint32_t i = 0; i < posix_links_cnt; ++i, ++lnk) {
        if (lnk->hash == ohash && strcmp(opath, lnk->fname) == 0) {
            if (lnk->type != 'O') {
                r = FR_EXIST;
                goto ex;
            }
            found_orig = true;
            break;
        }
    }
    if (!found_orig) {
        r = extfs_add_link(opath, ohash, 'O', 0, 0);
        if (FR_OK != r) {
            goto ex;
        }
    }
    r = extfs_add_link(path, hash, 'H', opath, ohash);
ex:
	xTaskResumeAll();
    return r;
}

// TODO: per process context
static array_t* pfiles = 0;

typedef struct FDESC_s {
    FIL* fp;
    unsigned int flags;
} FDESC;

static void* __in_hfa() alloc_file(void) {
    FDESC* d = (FDESC*)pvPortMalloc(sizeof(FDESC));
    if (!d) return NULL;
    d->fp = (FIL*)pvPortMalloc(sizeof(FIL));
    if (!d->fp) {
        vPortFree(d);
        return NULL;
    }
    memset(d->fp, 0, sizeof(FIL));
    d->flags = 0;
    return d;
}

static void __in_hfa() dealloc_file(void* p) {
    if (!p) return;
    FDESC* d = (FDESC*)p;
    if ((intptr_t)d->fp > 2) vPortFree(d->fp);
    vPortFree(p);
}

static void __in_hfa() init_pfiles() {
    if (pfiles) return;
    FIL* pf = (FIL*)pvPortMalloc(sizeof(FIL));
    if (!pf) { errno = ENOMEM; return; }
    vTaskSuspendAll();
    pfiles = new_array_v(alloc_file, dealloc_file, NULL);
    FRESULT r = f_open(pf, "/.extfs", FA_READ);
    if (r == FR_OK) {
        UINT br;
        char type;
        uint32_t hash;
        uint16_t strsize;
        while(!f_eof(pf)) {
            if (f_read(pf, &type, 1, &br) != FR_OK || br != 1) break;
            if (f_read(pf, &hash, sizeof(hash), &br) != FR_OK || br != sizeof(hash)) break;
            if (f_read(pf, &strsize, sizeof(strsize), &br) != FR_OK || br != sizeof(strsize) || strsize < 2) break;
            char* buf = pvPortMalloc(strsize + 1);
            if (!buf) break;
            buf[strsize] = 0;
            if (f_read(pf, buf, strsize, &br) != FR_OK || strsize != br) {
                vPortFree(buf);
                break;
            }
            char* obuf = 0;
            uint32_t ohash = 0;
            if (type == 'H') {
                if (f_read(pf, &ohash, sizeof(ohash), &br) != FR_OK || br != sizeof(ohash)) goto brk2;
                if (f_read(pf, &strsize, sizeof(strsize), &br) != FR_OK || br != sizeof(strsize) || strsize < 2) goto brk1;
                obuf = (char*)pvPortMalloc(strsize + 1);
                if (obuf) {
                    obuf[strsize] = 0;
                    if (f_read(pf, obuf, strsize, &br) != FR_OK || strsize != br) {
                        brk1: vPortFree(obuf);
                        brk2: vPortFree(buf);
                        break;
                    }
                }
            }
            posix_add_link(hash, buf, type, ohash, obuf, true);
        }
        f_close(pf);
    }
    vPortFree(pf);

    // W/A for predefined file descriptors:
    FDESC* d;
    d = (FDESC*)pvPortMalloc(sizeof(FDESC));
    d->fp = (void*)STDIN_FILENO;
    d->flags = 0;
    array_push_back(pfiles, d); // 0 - stdin
    d = (FDESC*)pvPortMalloc(sizeof(FDESC));
    d->fp = (void*)STDOUT_FILENO;
    d->flags = 0;
    array_push_back(pfiles, d); // 1 - stdout
    d = (FDESC*)pvPortMalloc(sizeof(FDESC));
    d->fp = (void*)STDERR_FILENO;
    d->flags = 0;
    array_push_back(pfiles, d); // 2 - stderr
	xTaskResumeAll();
}

static BYTE __in_hfa() map_flags_to_ff_mode(int flags) {
    BYTE mode = 0;
    if (flags & O_RDWR) {
        mode |= FA_READ | FA_WRITE;
    } else if (flags & O_WRONLY) {
        mode |= FA_WRITE;
    } else { // O_RDONLY
        mode |= FA_READ;
    }
    if (flags & O_CREAT) {
        if (flags & O_EXCL) {
            mode |= FA_CREATE_NEW;      // fail if exists
        } else if (flags & O_TRUNC) {
            mode |= FA_CREATE_ALWAYS;   // create or truncate
        } else {
            mode |= FA_OPEN_ALWAYS;     // create if not exists
        }
    } else if (flags & O_TRUNC) {
        // truncate existing file without creating
        mode |= FA_CREATE_ALWAYS;
    }
    if (flags & O_APPEND) {
        mode |= FA_OPEN_APPEND; // FatFs append flag
    }
    return mode;
}

static int __in_hfa() map_ff_fresult_to_errno(FRESULT fr) {
    switch(fr) {
        case FR_OK:
            return 0;          // ok
        case FR_DISK_ERR:
        case FR_INT_ERR:
        case FR_NOT_READY:
            return EIO;        // Input/output error
        case FR_NO_FILE:
        case FR_NO_PATH:
            return ENOENT;     // File or path not found
        case FR_INVALID_NAME:
            return ENOTDIR;    // Invalid path component
        case FR_DENIED:
        case FR_WRITE_PROTECTED:
            return EACCES;     // Permission denied
        case FR_EXIST:
            return EEXIST;     // File exists
        case FR_TOO_MANY_OPEN_FILES:
            return EMFILE;     // Process limit reached
        case FR_NOT_ENABLED:
        case FR_INVALID_OBJECT:
        case FR_TIMEOUT:
        case FR_LOCKED:
        case FR_NOT_ENOUGH_CORE:
      //  case FR_TOO_MANY_OPEN_FILES: // FatFs specific
            return EINVAL;     // Generic invalid argument
        default:
            return EIO;        // I/O error
    }
}

static bool is_closed_desc(const FDESC* fd) {
    return fd && (intptr_t)fd->fp > STDERR_FILENO && fd->fp->obj.fs == 0;
}

static FIL* __in_hfa() array_lookup_first_closed(array_t* arr, size_t* pn) {
    for (size_t i = 3; i < arr->size; ++i) {
        FDESC* fd = (FDESC*)array_get_at(arr, i);
        if (!fd || !fd->fp) continue;
        if (is_closed_desc(fd)) {
            *pn = i;
            return fd->fp;
        }
    }
    return NULL;
}

/*
 * openat() — open a file relative to a directory file descriptor
 *
 * Parameters:
 *   dfd   – directory file descriptor (use AT_FDCWD for current directory)
 *   path  – pathname of the file to open
 *   flags – file status flags and access modes (see below)
 *   mode  – permissions to use if a new file is created
 *
 * Returns:
 *   On success: a new file descriptor (non-negative)
 *   On error:  -1 and errno is set appropriately
 */
int __in_hfa() __openat(int dfd, const char *path, int flags, mode_t mode) {
    // TODO: dfd
    if (!path) {
        errno = ENOTDIR;
        return -1;
    }
    // TODO: /dev/... , /proc/..., symlink
    size_t n;
    init_pfiles();
    FIL* pf = array_lookup_first_closed(pfiles, &n);
    if (!pf) {
        FDESC* fd = (FDESC*)alloc_file();
        if (!fd) { errno = ENOMEM; return -1; }
        pf = fd->fp;
        n = array_push_back(pfiles, fd);
    }
    pf->pending_descriptors = 0;
    BYTE ff_mode = map_flags_to_ff_mode(flags);
    FRESULT fr = f_open(pf, path, ff_mode);
    if (fr != FR_OK) {
        errno = map_ff_fresult_to_errno(fr);
        return -1;
    }
    FILINFO fno;
    fr = f_stat(path, &fno);
    if (fr != FR_OK) {
        errno = map_ff_fresult_to_errno(fr);
        return -1;
    }
    pf->ctime = fatfs_to_time_t(fno.fdate, fno.ftime);
    errno = 0;
    return (int)n;
}

int __in_hfa() __close(int fildes) {
    if (fildes <= STDERR_FILENO) {
        goto e;
    }
    init_pfiles();
    FDESC* fd = (FDESC*)array_get_at(pfiles, fildes);
    if (fd == 0 || is_closed_desc(fd)) {
        goto e;
    }
    FIL* fp = fd->fp;
    if ((intptr_t)fp <= STDERR_FILENO) {
        goto e;
    }
    if (fp->pending_descriptors) {
        --fp->pending_descriptors;
        errno = 0;
        return 0;
    }
    FRESULT fr = f_close(fp);
    errno = map_ff_fresult_to_errno(fr);
    return errno == 0 ? 0 : -1;
e:
    errno = EBADF;
    return -1;
}

int __in_hfa() __stat(const char *path, struct stat *buf) {
    if (!buf) {
        errno = EFAULT;
        return -1;
    }
    init_pfiles();
    // TODO: devices, links, etc...
    FILINFO fno;
    FRESULT fr = f_stat(path, &fno);
    if (fr != FR_OK) {
        errno = map_ff_fresult_to_errno(fr);
        return -1;
    }
    buf->st_dev   = 0;                    // no device differentiation in FAT
    buf->st_ino   = 0;                    // FAT has no inode numbers
    // FAT does not track UNIX-style permissions; set all to readable/writable/exec
    if (fno.fattrib & AM_DIR) {
        buf->st_mode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH | S_IFDIR;
    } else {
        buf->st_mode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH | S_IFREG;
    }
    buf->st_nlink = 1;                    // FAT does not support hard links
    buf->st_uid   = 0;                    // no UID/GID in FAT
    buf->st_gid   = 0;
    buf->st_rdev  = 0;                    // no special device info
    buf->st_size  = fno.fsize;            // file size in bytes
    // timestamps (FatFs has date/time in local format)
    buf->st_atime = 0;                     // FAT does not track last access reliably
    // convert FAT timestamps to time_t
    buf->st_atime = 0; // FAT does not reliably track last access
    buf->st_mtime = fatfs_to_time_t(fno.fdate, fno.ftime);
    buf->st_ctime = buf->st_mtime;
    errno = 0;
    return 0;
}

int __in_hfa() __fstat(int fildes, struct stat *buf) {
    if (!buf) {
        errno = EFAULT;
        return -1;
    }
    if (fildes < 0) {
        goto e;
    }
    memset(buf, 0, sizeof(struct stat));
    if (fildes == STDIN_FILENO) { // stdin
        buf->st_mode  = S_IFCHR | S_IRUSR | S_IRGRP | S_IROTH;
        goto ok;
    }
    if (fildes <= STDERR_FILENO) { // stdout/stderr
        buf->st_mode  = S_IFCHR | S_IWUSR | S_IWGRP | S_IWOTH;
        goto ok;
    }
    init_pfiles();
    FDESC* fd = (FDESC*)array_get_at(pfiles, fildes);
    if (fd == 0 || is_closed_desc(fd)) {
        goto e;
    }
    FIL* fp = fd->fp;
    buf->st_dev   = 0;                    // no device differentiation in FAT
    buf->st_ino   = 0;                    // FAT has no inode numbers
    buf->st_mode  = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH | S_IFREG;
    buf->st_nlink = 1;                    // FAT does not support hard links
    buf->st_uid   = 0;                    // no UID/GID in FAT
    buf->st_gid   = 0;
    buf->st_rdev  = 0;                    // no special device info
    buf->st_size  = f_size(fp);           // file size in bytes
    // timestamps (FatFs has date/time in local format)
    buf->st_atime = 0;                     // FAT does not track last access reliably
    // convert FAT timestamps to time_t
    buf->st_mtime = fp->ctime;
    buf->st_ctime = fp->ctime;
ok:
    errno = 0;
    return 0;
e:
    errno = EBADF;
    return -1;
}

int __in_hfa() __lstat(const char *path, struct stat *buf) {
    // no symlinl supported on FatFS
    return __stat(path, buf);
}

int __in_hfa() __read(int fildes, void *buf, size_t count) {
    if (!buf) {
        errno = EFAULT;
        return -1;
    }
    if (fildes < 0) {
        goto e;
    }
    init_pfiles();
    FDESC* fd = (FDESC*)array_get_at(pfiles, fildes);
    if (fd == 0 || is_closed_desc(fd)) {
        goto e;
    }
    FIL* fp = fd->fp;
    UINT br;
    FRESULT fr = f_read(fp, buf, count, &br);
    if (fr != FR_OK) {
        errno = map_ff_fresult_to_errno(fr);
        return -1;
    }
    errno = 0;
    return br;
e:
    errno = EBADF;
    return -1;
}

int __in_hfa() __readv(int fd, const struct iovec *iov, int iovcnt) {
    int res = 0;
    for (int i = 0; i < iovcnt; ++i, ++iov) {
        if (iov->iov_len == 0) continue;
        int sz = __read(fd, iov->iov_base, iov->iov_len);
        if (sz < 0) {
            return -1;
        }
        res += sz;
    }
    return res;
}

int __in_hfa() __write(int fildes, const void *buf, size_t count) {
    if (!buf) {
        errno = EFAULT;
        return -1;
    }
    if (fildes <= 0) {
        goto e;
    }
    init_pfiles();
    FDESC* fd = (FDESC*)array_get_at(pfiles, fildes);
    if (fd == 0 || is_closed_desc(fd)) {
        goto e;
    }
    FIL* fp = fd->fp;
    UINT br;
    FRESULT fr = f_write(fp, buf, count, &br);
    if (fr != FR_OK) {
        errno = map_ff_fresult_to_errno(fr);
        return -1;
    }
    errno = 0;
    return br;
e:
    errno = EBADF;
    return -1;
}

int __in_hfa() __writev(int fd, const struct iovec *iov, int iovcnt) {
    int res = 0;
    for (int i = 0; i < iovcnt; ++i, ++iov) {
        if (iov->iov_len == 0) continue;
        int sz = __write(fd, iov->iov_base, iov->iov_len);
        if (sz < 0) {
            return -1;
        }
        res += sz;
    }
    return res;
}

int __in_hfa() __dup(int oldfd) {
    if (oldfd < 0) {
        goto e;
    }
    init_pfiles();
    FDESC* fd = (FDESC*)array_get_at(pfiles, oldfd);
    if (fd == 0 || is_closed_desc(fd)) {
        goto e;
    }
    FIL* fp = fd->fp;
    ++fp->pending_descriptors;
    fd = (FDESC*)pvPortMalloc(sizeof(FDESC));
    if (!fd) { errno = ENOMEM; return -1; }
    fd->fp = fp;
    fd->flags = 0;
    int res = array_push_back(pfiles, fd);
    if (!res) {
        errno = ENOMEM;
        return -1;
    }
    errno = 0;
    return res;
e:
    errno = EBADF;
    return -1;
}

int __in_hfa() __dup3(int oldfd, int newfd, int flags) {
    if (oldfd < 0 || newfd < 0) goto e;
    if (oldfd == newfd) {
        errno = EINVAL;  // POSIX требует EINVAL при dup3(oldfd == newfd)
        return -1;
    }
    init_pfiles();
    FDESC* fd0 = (FDESC*)array_get_at(pfiles, oldfd);
    if (fd0 == 0 || is_closed_desc(fd0)) goto e;
    FDESC* fd1 = (FDESC*)array_get_at(pfiles, newfd);
    if (fd1 == 0) {
        if (array_resize(pfiles, newfd + 1) < 0) {
            errno = ENOMEM;
            return -1;
        }
        fd1 = (FDESC*)array_get_at(pfiles, newfd);
        if (fd1 == 0) {
            errno = ENOMEM;
            return -1;
        }
    } else {
        __close(newfd);
    }
    ++fd0->fp->pending_descriptors;
    if ((intptr_t)fd1->fp > STDERR_FILENO && !fd1->fp->pending_descriptors) { // not STD and not in use by other descriptors, so we can remove it
        vPortFree(fd1->fp);
    }
    fd1->fp = fd0->fp;
    fd1->flags = flags;
    pfiles->p[newfd] = fd1;
    errno = 0;
    return newfd;
e:
    errno = EBADF;
    return -1;
}

int __in_hfa() __dup2(int oldfd, int newfd)
{
    if (oldfd < 0 || newfd < 0) {
        errno = EBADF;
        return -1;
    }
    if (oldfd == newfd) {
        errno = 0;
        return newfd;
    }
    return __dup3(oldfd, newfd, 0);
}

/*
 * Minimal POSIX.1-like fcntl() implementation.
 *
 * Supported commands:
 *   F_GETFD   - get descriptor flags (e.g., FD_CLOEXEC)
 *   F_SETFD   - set descriptor flags
 *   F_GETFL   - get file status flags (e.g., O_APPEND, O_NONBLOCK)
 *   F_SETFL   - set file status flags
 *
 * Unimplemented commands will return -1 and set errno = EINVAL.
 */
int __in_hfa() __fcntl(int fd, int cmd, int flags) {
    if (fd < 0) goto e;
    init_pfiles();
    FDESC* fdesc = (FDESC*)array_get_at(pfiles, fd);
    if (!fdesc || is_closed_desc(fdesc)) goto e;

    int ret = 0;
    switch (cmd) {
        case F_GETFD:
            ret = fdesc->flags & FD_CLOEXEC;
            break;

        case F_SETFD: {
            fdesc->flags = (fdesc->flags & ~FD_CLOEXEC) | (flags & FD_CLOEXEC);
            break;
        }

        case F_GETFL:
            ret = fdesc->flags;
            break;

        case F_SETFL: {
            /* Only allow O_APPEND, O_NONBLOCK, etc. — silently ignore unsupported bits */
            fdesc->flags = (fdesc->flags & ~(O_APPEND | O_NONBLOCK)) | (flags & (O_APPEND | O_NONBLOCK));
            break;
        }

        default:
            errno = EINVAL;
            return -1;
    }

   
    errno = 0;
    return 0;
e:
    errno = EBADF;
    return -1;
}

int __in_hfa() __llseek(unsigned int fd,
             unsigned long offset_high,
             unsigned long offset_low,
             off_t *result,
             unsigned int whence
) {
    off_t offset = offset_low | ((off_t)offset_high << 32);
    if (fd < 0) goto e;
    init_pfiles();
    // Standard descriptors cannot be seeked
    if (fd <= STDERR_FILENO) {
        errno = ESPIPE; // Illegal seek
        return -1;
    }
    FDESC* fdesc = (FDESC*)array_get_at(pfiles, fd);
    if (!fdesc || is_closed_desc(fdesc)) goto e;

    FIL* fp = fdesc->fp;
    FSIZE_t new_pos;

    switch (whence) {
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos = f_tell(fp) + offset;
            break;
        case SEEK_END:
            new_pos = f_size(fp) + offset;
            break;
        default:
            errno = EINVAL;
            return -1;
    }

    if (new_pos < 0) {
        errno = EINVAL;
        return -1;
    }
    FRESULT fr = f_lseek(fp, new_pos);
    if (fr != FR_OK) {
        errno = map_ff_fresult_to_errno(fr);
        return -1;
    }

    errno = 0;
    *result = f_tell(fp);
    return 0;
e:
    errno = EBADF;
    return -1;
}

long __in_hfa() __lseek_p(int fd, long offset, int whence) {
    if (fd < 0) goto e;
    init_pfiles();
    // Standard descriptors cannot be seeked
    if (fd <= STDERR_FILENO) {
        errno = ESPIPE; // Illegal seek
        return -1;
    }
    FDESC* fdesc = (FDESC*)array_get_at(pfiles, fd);
    if (!fdesc || is_closed_desc(fdesc)) goto e;

    FIL* fp = fdesc->fp;
    FSIZE_t new_pos;

    switch (whence) {
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos = f_tell(fp) + offset;
            break;
        case SEEK_END:
            new_pos = f_size(fp) + offset;
            break;
        default:
            errno = EINVAL;
            return -1;
    }

    if (new_pos < 0) {
        errno = EINVAL;
        return -1;
    }

    FRESULT fr = f_lseek(fp, new_pos);
    if (fr != FR_OK) {
        errno = map_ff_fresult_to_errno(fr);
        return -1;
    }

    errno = 0;
    return f_tell(fp);
e:
    errno = EBADF;
    return -1;
}

// easy and fast FNV-1a (32-bit)
static uint32_t __in_hfa() get_hash(const char *pathname) {
    uint32_t h = 2166136261u;
    while (*pathname) {
        unsigned char c = 
        h ^= (unsigned char)*pathname++;
        h *= 16777619u;
    }
    return h;
}

int __in_hfa() __unlinkat(int dirfd, const char* _pathname, int flags) {
    // TODO: dirfd, flags
    init_pfiles();
	char* pathname = __realpath(_pathname, 0);
	if (!pathname) { errno = ENOMEM; return -1; }
    vTaskSuspendAll();
    uint32_t hash = get_hash(pathname);
    posix_link_t* rename_to = 0;
    FRESULT fr;
    if (posix_unlink(pathname, hash, &rename_to)) {
        if (rename_to) {
            f_unlink(rename_to->fname);
            fr = f_rename(pathname, rename_to->fname); if (fr != FR_OK) { goto err; }
            // new original, so cleanup 'H' related fields
            vPortFree(rename_to->ofname);
            rename_to->ofname = 0;
            rename_to->ohash = 0;
            goto ok;
        }
    }
    fr = f_unlink(pathname);
    if (fr != FR_OK) {
err:
    	xTaskResumeAll();
        vPortFree(pathname);
        errno = map_ff_fresult_to_errno(fr);
        return -1;
    }
ok:
    extfs_flush();
	xTaskResumeAll();
    vPortFree(pathname);
    errno = 0;
    return 0;
}

int __in_hfa() __rename(const char * f1, const char * f2) {
    init_pfiles();
    // TODO: links
    FRESULT fr = f_rename(f1, f2);
    if (fr != FR_OK) {
        errno = map_ff_fresult_to_errno(fr);
        return -1;
    }
    errno = 0;
    return 0;
}

int __in_hfa() __linkat(int fde, const char* _existing, int fdn, const char* _new, int flag) {
    /// TODO: fde, fdn
    init_pfiles();
	char* existing = __realpath(_existing, 0);
	if (!existing) { errno = ENOMEM; return -1; }
	char* new = __realpath(_new, 0);
	if (!new) { vPortFree(existing); errno = ENOMEM; return -1; }
    uint32_t ohash = get_hash(existing);
    uint32_t hash = get_hash(new);
    FIL* pf = (FIL*)pvPortMalloc(sizeof(FIL));
    if (!pf) { vPortFree(existing); errno = ENOMEM; return -1; }
    FRESULT fr = f_open(pf, new, FA_WRITE | FA_CREATE_NEW);
    if (fr != FR_OK) {
        vPortFree(existing);
        vPortFree(new);
        vPortFree(pf);
        errno = map_ff_fresult_to_errno(fr);
        return -1;
    }
    UINT bw;
    f_write(pf, "H", 1, &bw);
    fr = f_write(pf, existing, strlen(existing) + 1, &bw);
    f_close(pf);
    vPortFree(pf);
    if (fr != FR_OK) {
        vPortFree(existing);
        vPortFree(new);
        errno = map_ff_fresult_to_errno(fr);
        return -1;
    }
    fr = extfs_add_hlink(new, hash, existing, ohash);
    vPortFree(existing);
    vPortFree(new);
    if (fr != FR_OK) {
        errno = map_ff_fresult_to_errno(fr);
        return -1;
    }
    errno = 0;
    return 0;
}
int __in_hfa() __symlinkat(const char *existing, int fd, const char* _new) {
    /// TODO: fd
    init_pfiles();
	char* new = __realpath(_new, 0);
	if (!new) { errno = ENOMEM; return -1; }
    uint32_t hash = get_hash(new);
    FIL* pf = (FIL*)pvPortMalloc(sizeof(FIL));
    if (!pf) { vPortFree(new); errno = ENOMEM; return -1; }
    FRESULT fr = f_open(pf, new, FA_WRITE | FA_CREATE_NEW);
    if (fr != FR_OK) {
        vPortFree(new);
        vPortFree(pf);
        errno = map_ff_fresult_to_errno(fr);
        return -1;
    }
    UINT bw;
    f_write(pf, "S", 1, &bw);
    fr = f_write(pf, existing, strlen(existing) + 1, &bw);
    f_close(pf);
    vPortFree(pf);
    if (fr != FR_OK) {
        vPortFree(new);
        errno = map_ff_fresult_to_errno(fr);
        return -1;
    }
    fr = extfs_add_link(new, hash, 'S', 0, 0);
    vPortFree(new);
    if (fr != FR_OK) {
        errno = map_ff_fresult_to_errno(fr);
        return -1;
    }
    errno = 0;
    return 0;
}

long __in_hfa() __readlinkat2(int fd, const char *restrict _path, char *restrict buf, size_t bufsize, int reqursive) {
    // TODO: fd -> at
    init_pfiles();
	char* path = reqursive ? _path : __realpath(_path, 0);
	if (!path) { errno = ENOMEM; return -1; }
    if (path[0] != '/') {
        size_t sz = strlen(path) + 1;
        char* sp = (char*)pvPortMalloc(sz + 1);
        if (!sp) {
            if(!reqursive) vPortFree(path);
            errno = ENOMEM;
            return -1;
        }
        sp[0] = '/';
        memcpy(sp + 1, path, sz);
        if(!reqursive) vPortFree(path);
        path = sp;
        reqursive = false; // to cleanup allocated
    }
    if (!is_symlink(get_hash(path), path)) {
        if(!reqursive) vPortFree(path);
        errno = EINVAL;
        return -1;
    }
    FIL* pf = (FIL*)pvPortMalloc(sizeof(FIL));
    if (!pf) { vPortFree(path); errno = ENOMEM; return -1; }
    FRESULT fr = f_open(pf, path, FA_READ);
    if (fr != FR_OK) {
        if(!reqursive) vPortFree(path);
        vPortFree(pf);
        errno = map_ff_fresult_to_errno(fr);
        return -1;
    }
    if(!reqursive) vPortFree(path);
    UINT br;
    fr = f_read(pf, buf, bufsize, &br);
    f_close(pf);
    vPortFree(pf);
    if (fr != FR_OK) {
        errno = map_ff_fresult_to_errno(fr);
        return -1;
    }
    if (br < 2 || buf[0] != 'S') {
        errno = EINVAL;
        return -1;
    }
    long res = strlen(buf + 1);
    memmove(buf, buf + 1, res);
    errno = 0;
    return res;
}

long __in_hfa() __readlinkat(int fd, const char *restrict _path, char *restrict buf, size_t bufsize) {
    return __readlinkat2(fd, _path, buf, bufsize, false);
}
