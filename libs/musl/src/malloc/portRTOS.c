#include <stdlib.h>

#undef malloc
#undef calloc
#undef realloc
#undef free

void* malloc(size_t sz) {
    return pvPortMalloc(sz);
}

void* calloc(size_t sz0, size_t sz1) {
    return pvPortCalloc(sz0, sz1);
}

void free(void* p) {
    return vPortFree(p);
}

void* realloc(void *ptr, size_t new_size) {
    return pvPortRealloc(ptr, new_size);
}

/// TODO:
void* memalign(size_t alignment, size_t size) {
    return pvPortMalloc(size);
}

/// TODO:
void* aligned_alloc(size_t alignment, size_t size) {
    return pvPortMalloc(size);
}
