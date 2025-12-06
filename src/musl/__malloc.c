#include "internal/stdio_impl.h"
#include "internal/__stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmd.h"

void* __malloc2(void* ctx, size_t sz) {
    if (!ctx || !((cmd_ctx_t*)ctx)->pallocs)
        return pvPortMalloc(sz);
    return pvPortMalloc(sz);
}

void* __calloc2(void* ctx, size_t n, size_t sz) {
    if (!ctx || !((cmd_ctx_t*)ctx)->pallocs)
        return pvPortCalloc(n, sz);
    return pvPortCalloc(n, sz);
}

void* __realloc2(void* ctx, void* p, size_t sz) {
    if (!ctx || !((cmd_ctx_t*)ctx)->pallocs)
        return pvPortRealloc(p, sz);
    return pvPortRealloc(p, sz);
}

void __free2(void* ctx, void* p) {
    if (!ctx || !((cmd_ctx_t*)ctx)->pallocs)
        return vPortFree(p);
    return vPortFree(p);
}

void* __malloc(size_t sz) {
    return __malloc2(get_cmd_ctx(), sz);
}

void* __calloc(size_t n, size_t sz) {
    return __calloc2(get_cmd_ctx(), n, sz);
}

void* __realloc(void* p, size_t sz) {
    return __realloc2(get_cmd_ctx(), p, sz);
}

void __free(void* p) {
    return __free2(get_cmd_ctx(), p);
}

