#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <fts.h>
#include "sys_table.h"
char* copy_str(const char* s); // TODO: from cmd.h

FTS* __in_hfa() fts_open(
    char * const *path_argv,
    int options,
    int (*compar)(const FTSENT **, const FTSENT **)
) {
    if (!path_argv || !path_argv[0]) {
        errno = EINVAL;
        return NULL;
    }
    if ((options & (FTS_LOGICAL | FTS_PHYSICAL)) == (FTS_LOGICAL | FTS_PHYSICAL)) {
        errno = EINVAL;
        return NULL;
    }

    FTS *fts = (FTS*)pvPortMalloc(sizeof(FTS));
    if (!fts) { errno = ENOMEM; return NULL; }

    fts->fts_root = NULL;
    fts->fts_cur = NULL;
    fts->fts_compar = compar;
    fts->fts_options = options;
    size_t root_count = 0;

    if (options & ~FTS_OPTIONMASK) {
        errno = EINVAL;
        vPortFree(fts);
        return NULL;
    }
    errno = 0;

    FTSENT *init = pvPortCalloc(1, sizeof(FTSENT));
    if (!init) {
        // очистить всё
        for (FTSENT *e = fts->fts_root; e;) {
            FTSENT *next = e->fts_next;
            vPortFree(e->fts_accpath);
            vPortFree(e->fts_path);
            vPortFree(e);
            e = next;
        }
        vPortFree(fts);
        errno = ENOMEM;
        return NULL;
    }
    init->fts_info = FTS_INIT;
    init->fts_level = FTS_ROOTPARENTLEVEL;
    init->fts_parent = NULL;
    fts->fts_cur = init;

    int (*statfunc)(const char*, struct stat*) =
        (options & FTS_PHYSICAL) ? __lstat : __stat;

    for (size_t i = 0; path_argv[i]; ++i) {
        FTSENT *ent = pvPortCalloc(1, sizeof(FTSENT));
        if (!ent) continue;

        ent->fts_path = copy_str(path_argv[i]);
        if (!ent->fts_path) { vPortFree(ent); continue; }
        ent->fts_accpath = copy_str(path_argv[i]);
        if (!ent->fts_accpath) { vPortFree(ent->fts_path); vPortFree(ent); continue; }
        ent->fts_name = strrchr(ent->fts_path, '/');
        if (ent->fts_name) ent->fts_name++;
        else ent->fts_name = ent->fts_path;

        ent->fts_level = FTS_ROOTLEVEL;
        ent->fts_parent = NULL;
        ent->fts_children = NULL;
        ent->fts_next = NULL;
        ent->fts_flags = 0;
        ent->fts_instr = FTS_NOINSTR;

        if (!(options & FTS_NOSTAT)) {
            if (statfunc(ent->fts_path, &ent->fts_stat) < 0) {
                ent->fts_info = FTS_NS;
                ent->fts_statp = NULL;
                ent->fts_errno = errno;
            } else {
                ent->fts_statp = &ent->fts_stat;
                if (S_ISDIR(ent->fts_stat.st_mode))
                    ent->fts_info = FTS_D;
                else if (S_ISLNK(ent->fts_stat.st_mode))
                    ent->fts_info = FTS_SL;
                else
                    ent->fts_info = FTS_F;
            }
        } else {
            ent->fts_statp = NULL;
            ent->fts_info = FTS_NSOK;
        }

        ent->fts_next = fts->fts_root;
        fts->fts_root = ent;
        root_count++;
    }
    if (root_count == 0) {
        vPortFree(init);
        vPortFree(fts);
        errno = ENOENT;
        return NULL;
    }
    for (FTSENT *ent = fts->fts_root; ent; ent = ent->fts_next)
        ent->fts_parent = init;

    /* Сортировка корней, если задан compar */
    if (compar && root_count > 1) {
        // простой bubble-sort
        int changed;
        do {
            changed = 0;
            FTSENT **p = &fts->fts_root;
            while ((*p) && (*p)->fts_next) {
                FTSENT *a = *p;
                FTSENT *b = a->fts_next;
                if (compar((const FTSENT**)&a, (const FTSENT**)&b) > 0) {
                    a->fts_next = b->fts_next;
                    b->fts_next = a;
                    *p = b;
                    changed = 1;
                }
                p = &((*p)->fts_next);
            }
        } while (changed);
    }

    return fts;
}

int __in_hfa() fts_close(FTS *fts)
{
    if (!fts) {
        errno = EINVAL;
        return -1;
    }

    // Освобождение всей цепочки FTSENT
    FTSENT *root = fts->fts_root;
    while (root) {
        FTSENT *next = root->fts_next;

        // Рекурсивно освобождаем детей (если есть)
        FTSENT *child = root->fts_children;
        while (child) {
            FTSENT *cnext = child->fts_next;
            if (child->fts_accpath) vPortFree(child->fts_accpath);
            if (child->fts_path) vPortFree(child->fts_path);
            vPortFree(child);
            child = cnext;
        }

        if (root->fts_accpath) vPortFree(root->fts_accpath);
        if (root->fts_path) vPortFree(root->fts_path);
        vPortFree(root);
        root = next;
    }

    // Текущий элемент (инициализационный)
    if (fts->fts_cur) {
        if (fts->fts_cur->fts_accpath) vPortFree(fts->fts_cur->fts_accpath);
        if (fts->fts_cur->fts_path) vPortFree(fts->fts_cur->fts_path);
        vPortFree(fts->fts_cur);
    }

    vPortFree(fts);
    errno = 0;
    return 0;
}

#if 0
FTSENT* __in_hfa() fts_read(FTS *fts) {
    if (!fts || !fts->fts_cur) return NULL;

    FTSENT *cur = fts->fts_cur;

    /* Если каталог и нужно рекурсивно спускаться */
    if (S_ISDIR(cur->fts_stat.st_mode) &&
        !(fts->fts_options & FTS_NAMEONLY)) {

        DIR *dp = opendir(cur->fts_path);
        if (dp) {
            struct dirent *de;
            FTSENT *prev = NULL;
            while ((de = readdir(dp)) != NULL) {
                if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) {
                    if (!(fts->fts_options & FTS_SEEDOT)) continue;
                }

                FTSENT *child = (FTSENT*)pvPortMalloc(sizeof(FTSENT));
                memset(child, 0, sizeof(FTSENT));
                child->fts_name = copy_str(de->d_name);
                size_t len = strlen(cur->fts_path) + 1 + strlen(de->d_name) + 1;
                child->fts_path = pvPortMalloc(len);
                snprintf(child->fts_path, len, "%s/%s", cur->fts_path, de->d_name);
                child->fts_parent = cur;
                child->fts_level = cur->fts_level + 1;

                /* Получаем stat, если нужно */
                if (!(fts->fts_options & FTS_NOSTAT)) {
                    __lstat(child->fts_path, &child->fts_stat);
                }

                if (prev) prev->fts_next = child;
                else cur->fts_children = child;
                prev = child;
            }
            closedir(dp);
        }
    }

    fts->fts_cur = cur->fts_next ? cur->fts_next : cur->fts_children;
    return cur;
}
#endif