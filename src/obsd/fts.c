#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fts.h>
#include "sys_table.h"
#include "cmd.h"

#undef PATH_MAX
#define PATH_MAX FF_MAX_LFN

static void __libc() strip_trailing_slashes(char *p) {
    if (!p) return;
    size_t n = strlen(p);
    while (n > 1 && p[n-1] == '/') { p[--n] = '\0'; }
}

static void __libc() add_root_tail(FTS *fts, FTSENT *ent) {
    if (!fts->fts_root) {
        fts->fts_root = ent;
    } else {
        FTSENT *t = fts->fts_root;
        while (t->fts_next) t = t->fts_next;
        t->fts_next = ent;
    }
}

static void __libc() fts_free_all(FTS *fts) {
    if (!fts) return;
    FTSENT *e = fts->fts_root;
    while (e) {
        FTSENT *next = e->fts_next;
        if (e->fts_accpath) vPortFree(e->fts_accpath);
        if (e->fts_path) vPortFree(e->fts_path);
        vPortFree(e);
        e = next;
    }
    /* init node (fts_cur) может быть отдельно выделен */
    if (fts->fts_cur) {
        if (fts->fts_cur->fts_info == FTS_INIT) {
            vPortFree(fts->fts_cur);
        }
    }
    vPortFree(fts);
}

static void __libc() sort_roots_if_needed(FTS *fts, size_t root_count) {
    if (!fts->fts_compar || root_count <= 1) return;
    int changed;
    do {
        changed = 0;
        FTSENT **pp = &fts->fts_root;
        while (*pp && (*pp)->fts_next) {
            FTSENT *x = *pp;
            FTSENT *y = x->fts_next;
            /* вызов сравнителя: он ожидает const FTSENT ** */
            int r = fts->fts_compar((const FTSENT **)&x, (const FTSENT **)&y);
            if (r > 0) {
                /* swap x и y в списке */
                x->fts_next = y->fts_next;
                y->fts_next = x;
                *pp = y;
                changed = 1;
                /* после swap pp всё ещё указывает на y (новый), не двигаем pp */
            } else {
                pp = &((*pp)->fts_next);
            }
        }
    } while (changed);
}

FTS* __libc() fts_open(
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
    if (options & ~FTS_OPTIONMASK) {
        errno = EINVAL;
        return NULL;
    }

    FTS *fts = (FTS*)pvPortMalloc(sizeof(FTS));
    if (!fts) { errno = ENOMEM; return NULL; }

    fts->fts_root = NULL;
    fts->fts_cur = NULL;
    fts->fts_compar = compar;
    fts->fts_options = options;

    /* init virtual parent node */
    FTSENT *init = (FTSENT*)pvPortCalloc(1, sizeof(FTSENT));
    if (!init) { vPortFree(fts); errno = ENOMEM; return NULL; }
    init->fts_info = FTS_INIT;
    init->fts_level = FTS_ROOTPARENTLEVEL;
    init->fts_parent = NULL;
    init->fts_children = NULL;
    init->fts_next = NULL;
    init->fts_path = NULL;
    init->fts_accpath = NULL;
    init->fts_name = NULL;
    init->fts_statp = NULL;
    init->fts_errno = 0;
    fts->fts_cur = init;

    int (*statfunc)(const char*, struct stat*) =
        (options & FTS_PHYSICAL) ? __lstat : __stat;

    size_t root_count = 0;
    /* Перебираем argv и создаём узлы корней */
    for (size_t i = 0; path_argv[i]; ++i) {
        const char *src = path_argv[i];
        if (!src) continue;

        /* длина проверяется в copy_str */
        FTSENT *ent = (FTSENT*)pvPortCalloc(1, sizeof(FTSENT));
        if (!ent) { /* не хватает памяти — пропускаем добавление этого корня */ continue; }

        ent->fts_path = copy_str(src);
        if (!ent->fts_path) { vPortFree(ent); continue; }

        /* дублируем accpath; для простой реализации accpath == path */
        ent->fts_accpath = copy_str(ent->fts_path);
        if (!ent->fts_accpath) {
            vPortFree(ent->fts_path);
            vPortFree(ent);
            continue;
        }

        /* убираем завершающие слеши, чтобы fts_name корректно считался */
        strip_trailing_slashes(ent->fts_path);
        strip_trailing_slashes(ent->fts_accpath);

        char *slash = strrchr(ent->fts_path, '/');
        ent->fts_name = slash ? (slash + 1) : ent->fts_path;
        if (!ent->fts_name) ent->fts_name = ent->fts_path;

        ent->fts_level = FTS_ROOTLEVEL;
        ent->fts_parent = init;
        ent->fts_children = NULL;
        ent->fts_next = NULL;
        ent->fts_flags = 0;
        ent->fts_instr = FTS_NOINSTR;
        ent->fts_errno = 0;

        /* stat / lstat */
        if (!(options & FTS_NOSTAT)) {
            errno = 0;
            if (statfunc(ent->fts_path, &ent->fts_stat) < 0) {
                ent->fts_info = FTS_NS;
                ent->fts_statp = NULL;
                ent->fts_errno = errno;
            } else {
                ent->fts_statp = &ent->fts_stat;
                if (S_ISLNK(ent->fts_stat.st_mode)) {
                    /* символическая ссылка */
                    if (options & FTS_COMFOLLOW) {
                        /* когда указан FTS_COMFOLLOW — следуем за CLI-симлинком */
                        struct stat targ;
                        if (__stat(ent->fts_path, &targ) == 0) {
                            ent->fts_stat = targ;
                            ent->fts_statp = &ent->fts_stat;
                            ent->fts_flags |= FTS_SYMFOLLOW;
                            if (S_ISDIR(targ.st_mode)) ent->fts_info = FTS_D;
                            else ent->fts_info = FTS_F;
                        } else {
                            /* символическая ссылка без цели */
                            ent->fts_info = FTS_SLNONE;
                        }
                    } else {
                        ent->fts_info = FTS_SL;
                    }
                } else if (S_ISDIR(ent->fts_stat.st_mode)) {
                    ent->fts_info = FTS_D;
                } else {
                    ent->fts_info = FTS_F;
                }
            }
        } else {
            ent->fts_statp = NULL;
            ent->fts_info = FTS_NSOK;
        }

        /* вставляем в хвост списка корней (сохраняем порядок argv) */
        add_root_tail(fts, ent);
        root_count++;
    }

    if (root_count == 0) {
        /* не добавлено ни одного корня: очистка и ошибка */
        fts_free_all(fts);
        errno = ENOENT;
        return NULL;
    }

    /* назначаем init как parent для всех корней (возможно уже назначено выше) */
    for (FTSENT *ent = fts->fts_root; ent; ent = ent->fts_next)
        ent->fts_parent = init;

    /* сортируем корни, если compar задан */
    sort_roots_if_needed(fts, root_count);

    /* готово */
    return fts;
}

int __libc() fts_close(FTS *fts)
{
    if (!fts) { errno = EINVAL; return -1; }
    /* освободим всё корректно */
    FTSENT *init = fts->fts_cur;
    /* освобождаем корни + узлы */
    FTSENT *e = fts->fts_root;
    while (e) {
        FTSENT *next = e->fts_next;
        if (e->fts_accpath) vPortFree(e->fts_accpath);
        if (e->fts_path) vPortFree(e->fts_path);
        vPortFree(e);
        e = next;
    }
    /* освободим init, если он был выделен */
    if (init && init->fts_info == FTS_INIT) {
        vPortFree(init);
    }
    vPortFree(fts);
    return 0;
}

/* Вспомог: освобождение списка дочерних узлов (рекурсивно освобождает только созданные
   узлы в поле fts_children; не трогает содержимое subtree, если оно раскрыто далее).
   Используется при FTS_AGAIN или при очистке. */
static void __libc() free_child_list(FTSENT *child)
{
    while (child) {
        FTSENT *next = child->fts_next;
        if (child->fts_accpath) { vPortFree(child->fts_accpath); child->fts_accpath = NULL; }
        if (child->fts_path)    { vPortFree(child->fts_path); child->fts_path = NULL; }
        if (child->fts_name)    { vPortFree(child->fts_name); child->fts_name = NULL; }
        /* NOTE: не рекурсивно освобождаем child->fts_children здесь; они будут
           освобождены при закрытии всего FTS или при явном освобождении subtree. */
        vPortFree(child);
        child = next;
    }
}

/* Создать и прикрепить список детей для parent->fts_children.
   Возвращает pointer на первый ребёнок или NULL. */
static FTSENT* __libc() populate_children(FTSENT *parent, int options)
{
    if (!parent) return NULL;
    if (parent->fts_children) return parent->fts_children; /* уже заполнено */

    DIR *d = __opendir(parent->fts_path);
    if (!d) {
        /* пометим как DNR (unreadable) если нужно */
        parent->fts_info = FTS_DNR;
        return NULL;
    }

    struct dirent *dp;
    FTSENT *head = NULL;
    FTSENT *tail = NULL;

    while ((dp = __readdir(d)) != NULL) {
        const char *dname = dp->d_name;
        if (dname[0] == '.' && (dname[1] == '\0' || (dname[1] == '.' && dname[2] == '\0'))) {
            if (!(options & FTS_SEEDOT))
                continue;  // пропускаем . и .. как требует стандарт
        }

        /* сформировать full path */
        size_t plen = strlen(parent->fts_path);
        size_t nlen = strlen(dname);
        size_t need = plen + 1 + nlen + 1;
        if (need > PATH_MAX) {
            /* пропускаем записи с чрезмерной длиной */
            continue;
        }

        char *full = (char*)pvPortMalloc(need);
        if (!full) continue;
        /* формируем: parent + '/' + name (parent уже без trailing slash) */
        if (parent->fts_path[plen-1] == '/') snprintf(full, need, "%s%s", parent->fts_path, dname);
        else snprintf(full, need, "%s/%s", parent->fts_path, dname);

        /* выделяем узел */
        FTSENT *child = (FTSENT*)pvPortCalloc(1, sizeof(FTSENT));
        if (!child) { vPortFree(full); continue; }

        child->fts_path = full;
        child->fts_accpath = copy_str(full); /* копия для accpath */
        strip_trailing_slashes(child->fts_path);
        strip_trailing_slashes(child->fts_accpath);
        child->fts_name = copy_str(dname);
        child->fts_parent = parent;
        child->fts_children = NULL;
        child->fts_next = NULL;
        child->fts_instr = FTS_NOINSTR;
        child->fts_errno = 0;

        /* stat/lstat */
        if (!(options & FTS_NOSTAT)) {
            errno = 0;
            if (__lstat(child->fts_path, &child->fts_stat) < 0) {
                child->fts_info = FTS_NS;
                child->fts_statp = NULL;
                child->fts_errno = errno;
            } else {
                child->fts_statp = &child->fts_stat;
                if (S_ISDIR(child->fts_stat.st_mode)) child->fts_info = FTS_D;
                else if (S_ISLNK(child->fts_stat.st_mode)) child->fts_info = FTS_SL;
                else child->fts_info = FTS_F;
            }
        } else {
            child->fts_statp = NULL;
            child->fts_info = FTS_NSOK;
        }

        /* прикрепляем к списку детей через fts_next */
        if (!head) head = child;
        else tail->fts_next = child;
        tail = child;
    }
    __closedir(d);
    parent->fts_children = head;
    return head;
}

/* fts_children: возвращает список детей (fts_children) для текущего узла sp->fts_cur.
   Если уже заполнен — возвращает существующий список. Не модифицирует стек/порядок обхода. */
FTSENT* __libc() fts_children(FTS *sp, int options)
{
    if (!sp || !sp->fts_cur) { errno = EINVAL; return NULL; }

    FTSENT *parent = sp->fts_cur;
    if (parent->fts_info != FTS_D) return NULL;

    /* Если была инструкция AGAIN — очистим старый список детей и пересоздадим */
    if (parent->fts_instr == FTS_AGAIN) {
        if (parent->fts_children) {
            free_child_list(parent->fts_children);
            parent->fts_children = NULL;
        }
        parent->fts_instr = FTS_NOINSTR;
    }

    /* Ленивая инициализация списка детей */
    FTSENT *children = populate_children(parent, sp->fts_options);
    return children;
}

/* Вспомог: поиск «следующего» узла в pre-order обходе, начиная от узла ent.
   Логика:
     - если у ent есть нераскрытые дети и ent не помечен SKIP — вернём первого child
     - иначе если есть сосед (fts_next) — вернём его
     - иначе идём вверх по parent до тех пор, пока не найдём родителя с соседним братом
*/
static FTSENT* __libc() next_preorder(FTS *sp, FTSENT *ent)
{
    if (!ent) return NULL;

    /* children: если директория и дети есть (или могут быть), и не SKIP */
    if (ent->fts_info == FTS_D && ent->fts_instr != FTS_SKIP) {
        /* ensure children are populated lazily (do not change ent->fts_instr here) */
        if (!ent->fts_children) {
            /* попытка заполнить; не проверяем return */
            populate_children(ent, sp->fts_options);
        }
        if (ent->fts_children) return ent->fts_children;
    }

    /* следующий sibling */
    if (ent->fts_next) return ent->fts_next;

    /* вверх по родителям */
    FTSENT *p = ent->fts_parent;
    while (p && p->fts_info != FTS_INIT) {
        if (p->fts_next) return p->fts_next;
        p = p->fts_parent;
    }

    /* нет следующего */
    return NULL;
}

/* fts_read: возвращает следующий узел в обходе. Пред-порядок.
   Поведение инструкций:
     - FTS_SKIP: при встрече не раскрывать детей (children не будет возвращён).
       Инструкция снимается после обработки.
     - FTS_AGAIN: при наличии — очистить children (если заполнены) и снять флаг;
       потом вести себя как обычно (пересоздание при следующем обращении).
*/
FTSENT* __libc() fts_read(FTS *sp)
{
    if (!sp) { errno = EINVAL; return NULL; }

    FTSENT *cur = sp->fts_cur;

    /* если cur == NULL (неинициализирован) — выставим на виртуальный init (если есть) */
    if (!cur) return NULL;

    /* если текущий — FTS_INIT (виртуальный родитель), следующая нода это первый корень */
    if (cur->fts_info == FTS_INIT) {
        /* первый корень */
        FTSENT *first = sp->fts_root;
        if (!first) return NULL;
        sp->fts_cur = first;
        return first;
    }

    /* Снимаем поведение AGAIN: если текущий помечен AGAIN — очистим children и снимем флаг.
       Реализация: если fts_cur имеет FTS_AGAIN, мы уже вернули его ранее, но обработаем на случай
       если fts_set был вызван до следующего перемещения. */
    if (cur->fts_instr == FTS_AGAIN) {
        if (cur->fts_children) {
            free_child_list(cur->fts_children);
            cur->fts_children = NULL;
        }
        cur->fts_instr = FTS_NOINSTR;
    }

    /* Если текущий имеет инструкцию SKIP — нужно не раскрывать его детей. Снимем флаг после применения. */
    if (cur->fts_instr == FTS_SKIP) {
        /* Сбросим флаг, но при этом следующий узел — sibling или вверх */
        cur->fts_instr = FTS_NOINSTR;
        FTSENT *nxt = next_preorder(sp, cur);
        sp->fts_cur = nxt;
        return cur;
    }

    /* Ищем следующий узел в pre-order (дети -> siblings -> up) */
    FTSENT *nxt = next_preorder(sp, cur);
    sp->fts_cur = nxt;
    return cur;
}

/* fts_set: устанавливает инструкцию для узла (например FTS_SKIP, FTS_AGAIN).
   Возвращает 0 при успехе, -1 при ошибке.
*/
int __libc() fts_set(FTS *sp, FTSENT *ent, int instr)
{
    if (!sp || !ent) { errno = EINVAL; return -1; }

    switch (instr) {
    case FTS_SKIP:
        /* пометить: при обработке узла fts_read не будет раскрывать children */
        ent->fts_instr = FTS_SKIP;
        return 0;
    case FTS_AGAIN:
        /* пометим: при следующем обращении children будут пересозданы (мы не трогаем их прямо сейчас) */
        ent->fts_instr = FTS_AGAIN;
        return 0;
    default:
        /* неизвестная инструкция — записываем напрямую */
        ent->fts_instr = instr;
        return 0;
    }
}