#include "errno.h"
#include "unistd.h"
#include "spawn.h"
#include "cmd.h"
#include "sys_table.h"

pid_t __fork(void) {
    // unsupported, use posix_spawn
    errno = ENOSYS;
    return -1;
}

pid_t __getpid(void) {
    const TaskHandle_t th = xTaskGetCurrentTaskHandle();
    return (pid_t)(intptr_t)th;
}

// TODO: -> .h
void exec_sync(cmd_ctx_t* ctx);

// TODO: replace old version with const key
void set_ctx_kv(cmd_ctx_t* ctx, const char* key, const char* val)
{
    if (!ctx || !key || !val) return;

    for (size_t i = 0; i < ctx->vars_num; ++i) {
        if (strcmp(key, ctx->vars[i].key) == 0) {

            /* replace existing value */
            vPortFree(ctx->vars[i].value);
            ctx->vars[i].value = copy_str(val);
            return;
        }
    }

    /* append new entry */
    vars_t* newv = pvPortMalloc(sizeof(vars_t) * (ctx->vars_num + 1));

    if (ctx->vars_num > 0) {
        memcpy(newv, ctx->vars, sizeof(vars_t) * ctx->vars_num);
        vPortFree(ctx->vars);
    }

    newv[ctx->vars_num].key   = copy_str(key);
    newv[ctx->vars_num].value = copy_str(val);

    ctx->vars = newv;
    ctx->vars_num++;
}

static cmd_ctx_t* prep_ctx(
    cmd_ctx_t* parent,
    const char *path,
    char *const argv[],
    char *const envp[]
) {
    cmd_ctx_t* child = pvPortCalloc(1, sizeof(cmd_ctx_t));
    if (!child) return NULL;

    /* --- copy argv --- */
    if (argv) {
        int argc = 0;
        while (argv[argc]) argc++;

        child->argc = argc;
        child->argv = pvPortCalloc(argc + 1, sizeof(char*));

        for (int i = 0; i < argc; i++)
            child->argv[i] = copy_str(argv[i]);
    }

    /* --- set orig_cmd --- */
    if (path)
        child->orig_cmd = copy_str(path);

    /* --- copy IO descriptors (do NOT steal parent’s descriptors!) --- */
    child->std_in  = parent->std_in;
    child->std_out = parent->std_out;
    child->std_err = parent->std_err;

    /* --- build environment --- */
    if (envp) {
        /* use provided envp */
        int n = 0;
        while (envp[n]) n++;

        child->vars_num = n;
        child->vars = pvPortCalloc(n, sizeof(vars_t));

        for (int i = 0; i < n; i++) {
            char *entry = envp[i];
            char *eq = strchr(entry, '=');

            if (!eq) continue;

            size_t keylen = eq - entry;
            char *key = pvPortMalloc(keylen + 1);
            memcpy(key, entry, keylen);
            key[keylen] = 0;

            child->vars[i].key   = key;
            child->vars[i].value = copy_str(eq + 1);
        }

    } else {
        /* inherit parent's environment */
        child->vars_num = parent->vars_num;
        child->vars = pvPortCalloc(child->vars_num, sizeof(vars_t));

        for (size_t i = 0; i < child->vars_num; i++) {
            child->vars[i].key   = copy_str(parent->vars[i].key);
            child->vars[i].value = copy_str(parent->vars[i].value);
        }
    }

    /* --- child internal fields --- */
    child->pboot_ctx   = parent->pboot_ctx;   /* or NULL if needed */
    child->parent_task = parent;
    child->stage       = 0;
    child->ret_code    = 0;

    return child;
}

static void vProcessTask(void *pv) {
    cmd_ctx_t* ctx = (cmd_ctx_t*)pv;
    #if DEBUG_APP_LOAD
    goutf("vProcessTask: %s [%p]\n", ctx->orig_cmd, ctx);
    #endif
    const TaskHandle_t th = xTaskGetCurrentTaskHandle();
    vTaskSetThreadLocalStoragePointer(th, 0, ctx);
    exec_sync(ctx);
    remove_ctx(ctx);
    #if DEBUG_APP_LOAD
    goutf("vProcessTask: [%p] <<<\n", ctx);
    #endif
    vTaskDelete( NULL );
    __unreachable();
}

int __posix_spawn(
    pid_t *pid_out,
    const char *path,
    const posix_spawn_file_actions_t *actions,
    const posix_spawnattr_t *attr,
    char *const argv[],
    char *const envp[]
) {
    cmd_ctx_t* parent = get_cmd_ctx();
    cmd_ctx_t* child  = prep_ctx(parent, path, argv, envp);
    if (!child) return ENOMEM;
// TODO:   apply_file_actions(child, actions);
    TaskHandle_t th;
    xTaskCreate(vProcessTask, child->argv[0], 1024/*x 4 = 4096*/, child, configMAX_PRIORITIES - 1, &th);

    if (pid_out)
        *pid_out = (pid_t)th;
    return 0;
}

int __execve(const char *pathname, char *const argv[], char *const envp[])
{
    if (!pathname) {
        errno = EFAULT;
        return -1;
    }

    cmd_ctx_t *ctx = get_cmd_ctx();

    /* ----------------- unload OLD program ----------------- */
    if (ctx->pboot_ctx) {
        cleanup_bootb_ctx(ctx->pboot_ctx);
        ctx->pboot_ctx = NULL;
    }

    /* ----------------- cleanup old argv ----------------- */
    if (ctx->argv) {
        for (int i = 0; i < ctx->argc; ++i)
            vPortFree(ctx->argv[i]);
        vPortFree(ctx->argv);
        ctx->argv = NULL;
    }
    ctx->argc = 0;

    /* ----------------- cleanup old orig_cmd ----------------- */
    if (ctx->orig_cmd) {
        vPortFree(ctx->orig_cmd);
        ctx->orig_cmd = NULL;
    }

    /* ----------------- cleanup old environment (if overwritten) ----------------- */
    if (envp) {
        if (ctx->vars) {
            for (size_t i = 0; i < ctx->vars_num; ++i) {
                // key может быть литералом — освобождать нельзя
            /// TODO:    if (!is_literal(ctx->vars[i].key))
              ///      vPortFree(ctx->vars[i].key);

                if (ctx->vars[i].value)
                    vPortFree(ctx->vars[i].value);
            }
            vPortFree(ctx->vars);
        }
        ctx->vars = NULL;
        ctx->vars_num = 0;

        // импорт нового envp
        for (int i = 0; envp[i]; ++i) {
            char *kv = envp[i];
            char *eq = strchr(kv, '=');
            if (!eq) continue;

            size_t klen = eq - kv;
            char *key = pvPortMalloc(klen + 1);
            memcpy(key, kv, klen);
            key[klen] = '\0';

            set_ctx_kv(ctx, key, eq + 1);

            vPortFree(key);
        }
    }

    /* ----------------- fill new argv ----------------- */
    if (argv) {
        int argc = 0;
        while (argv[argc]) argc++;

        ctx->argc = argc;
        ctx->argv = pvPortCalloc(argc + 1, sizeof(char*));
        for (int i = 0; i < argc; ++i)
            ctx->argv[i] = copy_str(argv[i]);
    }

    /* ----------------- set new orig_cmd ----------------- */
    ctx->orig_cmd = copy_str(pathname);

    /* ----------------- laod and jump into program ----------------- */
    exec_sync(ctx);
// should not be there, but if
    remove_ctx(ctx);
    #if DEBUG_APP_LOAD
    goutf("vProcessTask: [%p] <<<\n", ctx);
    #endif
    vTaskDelete( NULL );
    __unreachable();
}
