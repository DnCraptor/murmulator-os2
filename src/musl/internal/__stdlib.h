#ifndef _STDLIB_H
#define _STDLIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"

#define malloc(sz) pvPortMalloc(sz)
#define calloc(sz0, sz1) pvPortCalloc(sz0, sz1)
#define free(p) vPortFree(p)
#define realloc(ptr, new_size) pvPortRealloc(ptr, new_size)
/// TODO:
#define memalign(alignment, size) pvPortMalloc(alignment, size)
#define aligned_alloc(alignment, size) pvPortMalloc(alignment, size)

#ifdef __cplusplus
}
#endif

#endif
