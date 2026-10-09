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
    res->umask = 022;
    return res;
}

/// TODO: __delete_ctx

/* Reserve tracking metadata separately: allocation failure must leave the
 * list intact and, for realloc, must preserve the original allocation. */
static void track_allocation(list_t* pa, node_t* node, void* data) {
    node->prev = pa->last;
    node->next = NULL;
    node->data = data;
    if (pa->last) pa->last->next = node;
    else pa->first = node;
    pa->last = node;
    ++pa->size;
}

void* __malloc2(void* ctx, size_t sz) {
    if (!ctx || !((cmd_ctx_t*)ctx)->pallocs)
        return pvPortMalloc(sz);
    void* res = pvPortMalloc(sz);
    if (!res) return NULL;
    node_t* node = (node_t*)pvPortMalloc(sizeof(node_t));
    if (!node) {
        vPortFree(res);
        return NULL;
    }
    track_allocation(((cmd_ctx_t*)ctx)->pallocs, node, res);
    return res;
}

void* __calloc2(void* ctx, size_t n, size_t sz) {
    if (!ctx || !((cmd_ctx_t*)ctx)->pallocs)
        return pvPortCalloc(n, sz);
    void* res = pvPortCalloc(n, sz);
    if (!res) return NULL;
    node_t* node = (node_t*)pvPortMalloc(sizeof(node_t));
    if (!node) {
        vPortFree(res);
        return NULL;
    }
    track_allocation(((cmd_ctx_t*)ctx)->pallocs, node, res);
    return res;
}

void* __realloc2(void* ctx, void* p, size_t sz) {
    if (!ctx || !((cmd_ctx_t*)ctx)->pallocs)
        return pvPortRealloc(p, sz);
    list_t* pa = ((cmd_ctx_t*)ctx)->pallocs;
    node_t* n = list_lookup(pa, p);
    if (sz == 0) {
        if (n) list_erase_node(pa, n);
        else vPortFree(p);
        return NULL;
    }
    node_t* reserved = NULL;
    if (!n) {
        reserved = (node_t*)pvPortMalloc(sizeof(node_t));
        if (!reserved) return NULL;
    }
    void* res = pvPortRealloc(p, sz);
    if (!res) {
        if (reserved) vPortFree(reserved);
        return NULL;
    }
    if (n) n->data = res;
    else track_allocation(pa, reserved, res);
    return res;
}

void __free2(void* ctx, void* p) {
    if (!ctx || !((cmd_ctx_t*)ctx)->pallocs)
        return vPortFree(p);
    list_t* pa = ((cmd_ctx_t*)ctx)->pallocs;
    node_t* n = list_lookup(pa, p);
    if (!n) {
        vPortFree(p);
    } else {
        list_erase_node(pa, n);
    }
}

void __free_ctx(void* ctx) {
    if (!ctx)
        return;
    cmd_ctx_t* c = (cmd_ctx_t*)ctx;
    if (c->pallocs) {
        delete_list(c->pallocs);
        c->pallocs = 0;
    }
    if (c->pid) {
        pids->p[c->pid] = 0;
        if (c->pid > 1) {
            for (size_t i = 0; i < pids->size; ++i) {
                cmd_ctx_t* c2 = (cmd_ctx_t*)pids->p[i];
                if (!c2) continue;
                if (c2->ppid == c->pid) {
                    c2->parent_task = 0; // like old "detached" style
                    c2->ppid = 1; // assign to init proc
                }
            }
        }
    }
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

char* __copy_str(const char* s) {
    char* res = (char*)__malloc(strlen(s) + 1);
    strcpy(res, s);
    return res;
}
