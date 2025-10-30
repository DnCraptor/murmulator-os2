/*	$OpenBSD: err.h,v 1.13 2015/08/31 02:53:56 guenther Exp $	*/
/*	$NetBSD: err.h,v 1.11 1994/10/26 00:55:52 cgd Exp $	*/

/*-
 * Copyright (c) 1993
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
 *	@(#)err.h	8.1 (Berkeley) 6/2/93
 */

#ifndef _ERR_H_
#define	_ERR_H_

#include <sys/cdefs.h>
#include <machine/_types.h>		/* for __va_list */
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef __dead
#define __dead __attribute__((__noreturn__))
#endif

#ifndef pledge
#define pledge(...) (0)
#endif

__BEGIN_DECLS

extern const char* __progname;

inline static  void	err(int, const char*, ...)	__attribute__((__format__ (printf, 2, 3)));
inline static  void	verr(int, const char*, __va_list ap) __attribute__((__format__ (printf, 2, 0)));

inline static  void	err(int eval, const char* fmt, ...)	{
	va_list ap;
	va_start(ap, fmt);
	verr(eval, fmt, ap);
	va_end(ap);
}
inline static  void	verr(int eval, const char* fmt, __va_list ap) {
	int sverrno = errno;
	(void)fprintf(stderr, "%s: ", __progname);
	if (fmt != NULL) {
		(void)vfprintf(stderr, fmt, ap);
		(void)fprintf(stderr, ": ");
	}
	(void)fprintf(stderr, "%s\n", strerror(sverrno));
	exit(eval);
}

inline static  void	errc(int, int, const char *, ...) __attribute__((__format__ (printf, 3, 4)));
inline static  void	verrc(int, int, const char *, __va_list) __attribute__((__format__ (printf, 3, 0)));

inline static  void	errc(int eval, int code, const char* fmt, ...)	{
	va_list ap;
	va_start(ap, fmt);
	verrc(eval, code, fmt, ap);
	va_end(ap);
}
inline static  void	verrc(int eval, int code, const char* fmt, __va_list ap) {
	(void)fprintf(stderr, "%s: ", __progname);
	if (fmt != NULL) {
		(void)vfprintf(stderr, fmt, ap);
		(void)fprintf(stderr, ": ");
	}
	(void)fprintf(stderr, "%s\n", strerror(code));
	exit(eval);
}

inline static  void	errx(int, const char*, ...)	__attribute__((__format__ (printf, 2, 3)));
inline static  void	verrx(int, const char*, __va_list ap) __attribute__((__format__ (printf, 2, 0)));

inline static void errx(int eval, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	verrx(eval, fmt, ap);
	va_end(ap);
}
inline static void verrx(int eval, const char *fmt, va_list ap)
{
	(void)fprintf(stderr, "%s: ", __progname);
	if (fmt != NULL)
		(void)vfprintf(stderr, fmt, ap);
	(void)fprintf(stderr, "\n");
	exit(eval);
}

inline static void		warn(const char *, ...) __attribute__((__format__ (printf, 1, 2)));
inline static void		vwarn(const char *, __va_list) __attribute__((__format__ (printf, 1, 0)));

inline static  void	warn(const char* fmt, ...)	{
	va_list ap;
	va_start(ap, fmt);
	vwarn(fmt, ap);
	va_end(ap);
}
inline static  void	vwarn(const char* fmt, __va_list ap) {
	int sverrno = errno;
	(void)fprintf(stderr, "%s: ", __progname);
	if (fmt != NULL) {
		(void)vfprintf(stderr, fmt, ap);
		(void)fprintf(stderr, ": ");
	}
	(void)fprintf(stderr, "%s\n", strerror(sverrno));
}

inline static void		warnc(int, const char *, ...) __attribute__((__format__ (printf, 2, 3)));
inline static void		vwarnc(int, const char *, __va_list) __attribute__((__format__ (printf, 2, 0)));

inline static  void	warnc(int code, const char* fmt, ...)	{
	va_list ap;
	va_start(ap, fmt);
	vwarnc(code, fmt, ap);
	va_end(ap);
}
inline static  void	vwarnc(int code, const char* fmt, __va_list ap) {
	(void)fprintf(stderr, "%s: ", __progname);
	if (fmt != NULL) {
		(void)vfprintf(stderr, fmt, ap);
		(void)fprintf(stderr, ": ");
	}
	(void)fprintf(stderr, "%s\n", strerror(code));
}

inline static void		warnx(const char *, ...) __attribute__((__format__ (printf, 1, 2)));
inline static void		vwarnx(const char *, __va_list) __attribute__((__format__ (printf, 1, 0)));

inline static void warnx(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vwarnx(fmt, ap);
	va_end(ap);
}
inline static void vwarnx(const char *fmt, va_list ap) {
	(void)fprintf(stderr, "%s: ", __progname);
	if (fmt != NULL)
		(void)vfprintf(stderr, fmt, ap);
	(void)fprintf(stderr, "\n");
}

__END_DECLS

#endif /* !_ERR_H_ */
