#include "FreeRTOS.h"
#include "task.h"
#include "internal/__stdlib.h"
#include "sys_table.h"

#undef malloc
#undef calloc
#undef realloc
#undef free

void* __libc() malloc(size_t sz) {
    return pvPortMalloc(sz);
}

void* __libc() calloc(size_t sz0, size_t sz1) {
    return pvPortCalloc(sz0, sz1);
}

void __libc() free(void* p) {
    return vPortFree(p);
}

void* __libc() realloc(void *ptr, size_t new_size) {
    return pvPortRealloc(ptr, new_size);
}

/// TODO:
void* __libc() memalign(size_t alignment, size_t size) {
    return pvPortMalloc(size);
}

/// TODO:
void* __libc() aligned_alloc(size_t alignment, size_t size) {
    return pvPortMalloc(size);
}
