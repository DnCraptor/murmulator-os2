#include "internal/stdio_impl.h"
#include "internal/__stdlib.h"
#include "FreeRTOS.h"
#include "task.h"

void* __malloc(size_t sz) {
    return pvPortMalloc(sz);
}

void* __calloc(size_t n, size_t sz) {
    return pvPortCalloc(n, sz);
}

void* __realloc(void* p, size_t sz) {
    return pvPortRealloc(p, sz);
}

void __free(void* p) {
    return vPortFree(p);
}
