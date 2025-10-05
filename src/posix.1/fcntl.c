#include <FreeRTOS.h>
#include <task.h>

#include "sys/fcntl.h"
#include "sys/stat.h"
#include "errno.h"
#include "unistd.h"

#include "ff.h"
#define NULL 0
#include "../../api/m-os-api-c-list.h"


#include <time.h>
#include <stdint.h>

static int is_leap_year(int year) {
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

static const int days_in_month[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

// TODO: optimize it
time_t simple_mktime(struct tm *t) {
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

static time_t fatfs_to_time_t(WORD fdate, WORD ftime) {
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

// TODO: per process context
static list_t* pfiles_list = 0;

static void* alloc_file(void) {
    return pvPortMalloc(sizeof(FIL));
}
static void dealloc_file(void* p) {
    return vPortFree(p);
}

static BYTE map_flags_to_ff_mode(int flags) {
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

static int map_ff_fresult_to_errno(FRESULT fr) {
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

static FIL* list_lookup_first_closed(list_t* lst) {
    node_t* i = lst->first;
    size_t n = 0;
    while (i) {
        FIL* fp = (FIL*)i->data;
        if (fp->obj.fs == 0) { // closed
            return i;
        }
        i = i->next;
        ++n;
    }
    return NULL;
}

int __open(const char *path, int flags, mode_t mode) {
    if (!path) {
        errno = ENOTDIR;
        return -1;
    }
    // TODO: /dev/... , /proc/..., symlink
    FILINFO fno;
    FRESULT fr = f_stat(path, &fno);
    if (fr != FR_OK) {
        errno = map_ff_fresult_to_errno(fr);
        return -1;
    }
    if (!pfiles_list) pfiles_list = new_list_v(alloc_file, dealloc_file, 0);
    FIL* pf = list_lookup_first_closed(pfiles_list);
    if (pf == 0) {
        pf = (FIL*)alloc_file();
        list_push_back(pfiles_list, pf);
    }
    BYTE ff_mode = map_flags_to_ff_mode(flags);
    fr = f_open(pf, path, ff_mode);
    if (fr != FR_OK) {
        errno = map_ff_fresult_to_errno(fr);
        return -1;
    }
    pf->ctime = fatfs_to_time_t(fno.fdate, fno.ftime);
    errno = 0;
    return (int)pf; // just address (always less than 0x30000000) as descriptor
}

int __close(int fd) {
    if (fd <= 0) {
        goto e;
    }
    node_t* n = list_lookup(pfiles_list, (void*)fd);
    if (n == 0) {
        goto e;
    }
    FIL* fp = (FIL*)n->data;
    if (fp->obj.fs == 0) { // closed
        goto e;
    }
    errno = map_ff_fresult_to_errno(f_close(fp));
    return errno == 0 ? 0 : -1;
e:
    errno = -1;
    return EBADF;
}

int __stat(const char *path, struct stat *buf) {
    if (!buf) {
        errno = EFAULT;
        return -1;
    }
    FILINFO fno;
    FRESULT fr = f_stat(path, &fno);
    if (fr != FR_OK) {
        errno = map_ff_fresult_to_errno(fr);
        return -1;
    }
    buf->st_dev   = 0;                    // no device differentiation in FAT
    buf->st_ino   = 0;                    // FAT has no inode numbers
    buf->st_mode  = 0;
    buf->st_nlink = 1;                    // FAT does not support hard links
    buf->st_uid   = 0;                    // no UID/GID in FAT
    buf->st_gid   = 0;
    buf->st_rdev  = 0;                    // no special device info
    buf->st_size  = fno.fsize;            // file size in bytes

    // set file type
    if (fno.fattrib & AM_DIR) {
        buf->st_mode |= S_IFDIR;
    } else {
        buf->st_mode |= S_IFREG;
    }

    // FAT does not track UNIX-style permissions; set all to readable/writable/exec
    buf->st_mode |= S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH;

    // timestamps (FatFs has date/time in local format)
    buf->st_atime = 0;                     // FAT does not track last access reliably
    // convert FAT timestamps to time_t
    buf->st_atime = 0; // FAT does not reliably track last access
    buf->st_mtime = fatfs_to_time_t(fno.fdate, fno.ftime);
    buf->st_ctime = buf->st_mtime;
    errno = 0;
    return 0;
}

int __fstat(int fildes, struct stat *buf) {
    if (!buf) {
        errno = EFAULT;
        return -1;
    }
    if (fildes <= 0) {
        goto e;
    }
    node_t* n = list_lookup(pfiles_list, (void*)fildes);
    if (n == 0) {
        // TODO: directories
        goto e;
    }
    FIL* fp = (FIL*)n->data;
    if (fp->obj.fs == 0) { // closed
        goto e;
    }
    buf->st_dev   = 0;                    // no device differentiation in FAT
    buf->st_ino   = 0;                    // FAT has no inode numbers
    buf->st_mode  = 0;
    buf->st_nlink = 1;                    // FAT does not support hard links
    buf->st_uid   = 0;                    // no UID/GID in FAT
    buf->st_gid   = 0;
    buf->st_rdev  = 0;                    // no special device info
    buf->st_size  = f_size(fp);           // file size in bytes

    // FAT does not track UNIX-style permissions; set all to readable/writable/exec
    buf->st_mode |= S_IFREG | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH;

    // timestamps (FatFs has date/time in local format)
    buf->st_atime = 0;                     // FAT does not track last access reliably
    // convert FAT timestamps to time_t
    buf->st_mtime = fp->ctime;
    buf->st_ctime = fp->ctime;
    errno = 0;
    return 0;
e:
    errno = -1;
    return EBADF;
}

int __lstat(const char *path, struct stat *buf) {
    // no symlinl supported on FatFS
    return __stat(path, buf);
}
