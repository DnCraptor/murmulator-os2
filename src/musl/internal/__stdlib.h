#ifndef _STDLIB_H
#define _STDLIB_H

#ifdef __cplusplus
extern "C" {
#endif

void* malloc(size_t sz);
void* calloc(size_t sz0, size_t sz1);
void free(void* p);
void* realloc(void *ptr, size_t new_size);
void* memalign(size_t alignment, size_t size);
void* aligned_alloc(size_t alignment, size_t size);

#ifdef __cplusplus
}
#endif

#endif
