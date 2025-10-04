#include <FreeRTOS.h>
#include <task.h>

#include "sys/fcntl.h"
#include "sys/stat.h"
#include "errno.h"

#include "ff.h"
#define NULL 0
#include "../../api/m-os-api-c-list.h"

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

int __open(const char *pathname, int flags, mode_t mode) {
    if (!pathname) {
        errno = ENOTDIR;
        return -1;
    }
    // TODO: /dev/... , /proc/...
    if (!pfiles_list) pfiles_list = new_list_v(alloc_file, dealloc_file, 0);
    FIL* pf = (FIL*)alloc_file();
    list_push_back(pfiles_list, pf);
    BYTE ff_mode = map_flags_to_ff_mode(flags);
    FRESULT fr = f_open(pf, pathname, ff_mode);
    if (fr != FR_OK) {
        errno = map_ff_fresult_to_errno(fr);
        return -1;
    }
    errno = 0;
    return (int)pf; // just address (always less than 0x30000000) as descriptor
}
