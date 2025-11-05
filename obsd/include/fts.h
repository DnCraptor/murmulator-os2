/*	$OpenBSD: fts.h,v 1.14 2012/12/05 23:19:57 deraadt Exp $	*/
/*	$NetBSD: fts.h,v 1.5 1994/12/28 01:41:50 mycroft Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)fts.h	8.3 (Berkeley) 8/14/94
 */

#ifndef	_FTS_H_
#define	_FTS_H_

typedef struct {
    struct _ftsent *fts_root;         /* корень обхода */
    struct _ftsent *fts_cur;          /* текущий элемент */
    int (*fts_compar)(const struct _ftsent **, const struct _ftsent **); /* callback сортировки */
#define	FTS_COMFOLLOW	0x0001		/* follow command line symlinks */
#define	FTS_LOGICAL	0x0002		/* logical walk */
#define	FTS_NOCHDIR	0x0004		/* don't change directories */
#define	FTS_NOSTAT	0x0008		/* don't get stat info */
#define	FTS_PHYSICAL	0x0010		/* physical walk */
#define	FTS_SEEDOT	0x0020		/* return dot and dot-dot */
#define	FTS_XDEV	0x0040		/* don't cross devices */
#define	FTS_OPTIONMASK	0x00ff		/* valid user option mask */

#define	FTS_NAMEONLY	0x1000		/* (private) child names only */
#define	FTS_STOP	0x2000		/* (private) unrecoverable error */
	int fts_options;		/* fts_open options, global flags */
} FTS;

typedef struct _ftsent {
    char *fts_path;           /* полный путь к файлу/директории */
	char *fts_accpath;        /* путь для доступа (для fchmodat/fstatat и пр.) */
    char *fts_name;           /* имя последнего компонента */
    struct stat fts_stat;     /* информация о файле */
	struct stat *fts_statp;   /* NULL если stat не запрошен, иначе &fts_stat */
    struct FTSENT *fts_parent;
    struct FTSENT *fts_children;
    struct FTSENT *fts_next;

#define	FTS_ROOTPARENTLEVEL	-1
#define	FTS_ROOTLEVEL		 0
#define	FTS_MAXLEVEL		 0x7fffffff
	int fts_level;		/* depth (-1 to N) */

#define	FTS_D		 1		/* preorder directory */
#define	FTS_DC		 2		/* directory that causes cycles */
#define	FTS_DEFAULT	 3		/* none of the above */
#define	FTS_DNR		 4		/* unreadable directory */
#define	FTS_DOT		 5		/* dot or dot-dot */
#define	FTS_DP		 6		/* postorder directory */
#define	FTS_ERR		 7		/* error; errno is set */
#define	FTS_F		 8		/* regular file */
#define	FTS_INIT	 9		/* initialized only */
#define	FTS_NS		10		/* stat(2) failed */
#define	FTS_NSOK	11		/* no stat(2) requested */
#define	FTS_SL		12		/* symbolic link */
#define	FTS_SLNONE	13		/* symbolic link without target */
	unsigned short fts_info;	/* user flags for FTSENT structure */

#define	FTS_DONTCHDIR	 0x01		/* don't chdir .. to the parent */
#define	FTS_SYMFOLLOW	 0x02		/* followed a symlink to get here */
	unsigned short fts_flags;	/* private flags for FTSENT structure */

#define	FTS_AGAIN	 1		/* read node again */
#define	FTS_FOLLOW	 2		/* follow symbolic link */
#define	FTS_NOINSTR	 3		/* no instructions */
#define	FTS_SKIP	 4		/* discard node */
	unsigned short fts_instr;	/* fts_set() instructions */

	int fts_errno;
} FTSENT;

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

inline static FTS* fts_open(
    char * const *path_argv,
    int options,
    int (*compar)(const FTSENT **, const FTSENT **)
) {
    typedef FTS* (*fn_ptr_t)(char * const *, int, int (*compar)(const FTSENT **, const FTSENT **));
    return ((fn_ptr_t)_sys_table_ptrs[357])(path_argv, options, compar);
}

inline static int fts_close(FTS* fts) {
    typedef int (*fn_ptr_t)(FTS*);
    return ((fn_ptr_t)_sys_table_ptrs[358])(fts);
}

FTSENT	*fts_children(FTS *, int);
FTSENT	*fts_read(FTS *);
int	 fts_set(FTS *, FTSENT *, int);

#endif /* !_FTS_H_ */
