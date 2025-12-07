#include "internal/stdio_impl.h"
#include "internal/__stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmd.h"

static void* allocator(void) {
    return pvPortMalloc(sizeof(void*));
}

void* __new_ctx(void) {
    cmd_ctx_t* res = (cmd_ctx_t*)pvPortCalloc(1, sizeof(cmd_ctx_t));
    res->pallocs = new_list_v(allocator, vPortFree, 0);
    return res;
}

/// TODO: __delete_ctx

void* __malloc2(void* ctx, size_t sz) {
    if (!ctx || !((cmd_ctx_t*)ctx)->pallocs)
        return pvPortMalloc(sz);
    void* res = pvPortMalloc(sz);
    list_push_back(((cmd_ctx_t*)ctx)->pallocs, res);
    return res;
}

void* __calloc2(void* ctx, size_t n, size_t sz) {
    if (!ctx || !((cmd_ctx_t*)ctx)->pallocs)
        return pvPortCalloc(n, sz);
    void* res = pvPortCalloc(n, sz);
    list_push_back(((cmd_ctx_t*)ctx)->pallocs, res);
    return res;
}

void* __realloc2(void* ctx, void* p, size_t sz) {
    if (!ctx || !((cmd_ctx_t*)ctx)->pallocs)
        return pvPortRealloc(p, sz);
    list_t* pa = ((cmd_ctx_t*)ctx)->pallocs;
    list_erase_node(pa, list_lookup(pa, p));
    void* res = pvPortRealloc(p, sz);
    list_push_back(pa, res);
    return res;
}

void __free2(void* ctx, void* p) {
    if (!ctx || !((cmd_ctx_t*)ctx)->pallocs)
        return vPortFree(p);
    list_t* pa = ((cmd_ctx_t*)ctx)->pallocs;
    list_erase_node(pa, list_lookup(pa, p));
    return vPortFree(p);
}

void __free_ctx(void* ctx) {
    if (!ctx || !((cmd_ctx_t*)ctx)->pallocs)
        return;
    delete_list(((cmd_ctx_t*)ctx)->pallocs);
    ((cmd_ctx_t*)ctx)->pallocs = 0;
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

