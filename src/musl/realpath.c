#include "internal/stdio_impl.h"
#include "internal/__stdio.h"
#include "errno.h"
#include "unistd.h"
#include "sys/fcntl.h"
#include "sys_table.h"
#include "ff.h"
#include <string.h>
#include <limits.h>
#define SYMLOOP_MAX 10

#undef PATH_MAX
#define PATH_MAX FF_MAX_LFN

char* copy_str(const char* s); // cmd

/// TODO: by process ctx
char* __libc() getcwd(char *buf, size_t size)
{
	if (!size) {
		errno = EINVAL;
		return 0;
	}
	if (!buf) {
		buf = (char*)pvPortMalloc(2); /// TODO: <<
	}
	buf[0] = '/';
	buf[1] = 0;
//	if (buf[0] != '/') {
//		errno = ENOENT;
//		return 0;
//	}
	return buf;
}

static size_t __libc() slash_len(const char *s)
{
	const char *s0 = s;
	while (*s == '/') s++;
	return s-s0;
}

#define ALIGN (sizeof(size_t))
#define ONES ((size_t)-1/UCHAR_MAX)
#define HIGHS (ONES * (UCHAR_MAX/2+1))
#define HASZERO(x) ((x)-ONES & ~(x) & HIGHS)

/// TODO: other places?
char* __libc() __strchrnul(const char *s, int c)
{
	c = (unsigned char)c;
	if (!c) return (char *)s + strlen(s);

#ifdef __GNUC__
	typedef size_t __attribute__((__may_alias__)) word;
	const word *w;
	for (; (uintptr_t)s % ALIGN; s++)
		if (!*s || *(unsigned char *)s == c) return (char *)s;
	size_t k = ONES * c;
	for (w = (void *)s; !HASZERO(*w) && !HASZERO(*w^k); w++);
	s = (void *)w;
#endif
	for (; *s && *(unsigned char *)s != c; s++);
	return (char *)s;
}

weak_alias(__strchrnul, strchrnul);

char* __libc() __realpath(const char *restrict filename, char *restrict resolved) {
	return __realpathat(filename, resolved, AT_SYMLINK_FOLLOW);
}
char* __libc() __realpathat(const char *restrict filename, char *restrict resolved, int at) {
///	goutf("realpathat(%s, %s, %d)\n", filename, resolved ? resolved : "null", at);
	char* stack = (char*)pvPortMalloc(PATH_MAX+1);
	if (!stack) { errno = ENOMEM; return 0; }
	char* output = (char*)pvPortMalloc(PATH_MAX);
	if (!output) { vPortFree(stack); errno = ENOMEM; return 0; }
	size_t p, q, l, l0, cnt=0, nup=0;
	int check_dir=0;

	if (!filename) {
		errno = EINVAL;
		goto err;
	}
	l = strnlen(filename, PATH_MAX+1);
	if (!l) {
		errno = ENOENT;
		goto err;
	}
	if (l >= PATH_MAX) goto toolong;
	p = PATH_MAX - l;
	q = 0;
	for (int i = 0; i < l+1; ++i) {
		char c = filename[i];
		(stack+p)[i] = (c == '\\') ? '/' : c;
	}

	/* Main loop. Each iteration pops the next part from stack of
	 * remaining path components and consumes any slashes that follow.
	 * If not a link, it's moved to output; if a link, contents are
	 * pushed to the stack. */
restart:
	for (; ; p += slash_len(stack + p)) {
		/* If stack starts with /, the whole component is / or //
		 * and the output state must be reset. */
		if (stack[p] == '/') {
			check_dir=0;
			nup=0;
			q=0;
			output[q++] = '/';
			p++;
			/* Initial // is special. */
			if (stack[p] == '/' && stack[p+1] != '/')
				output[q++] = '/';
			continue;
		}

		char *z = __strchrnul(stack+p, '/');
		l0 = l = z-(stack+p);

		if (!l && !check_dir) break;

		/* Skip any . component but preserve check_dir status. */
		if (l==1 && stack[p]=='.') {
			p += l;
			continue;
		}

		/* Copy next component onto output at least temporarily, to
		 * call readlink, but wait to advance output position until
		 * determining it's not a link. */
		if (q && output[q-1] != '/') {
			if (!p) goto toolong;
			stack[--p] = '/';
			l++;
		}
		if (q+l >= PATH_MAX) goto toolong;
		memcpy(output+q, stack+p, l);
		output[q+l] = 0;
		p += l;

		int up = 0;
		if (l0 == 2 && stack[p-2] == '.' && stack[p-1] == '.') {
			up = 1;
			/* Any non-.. path components we could cancel start
			 * after nup repetitions of the 3-byte string "../";
			 * if there are none, accumulate .. components to
			 * later apply to cwd, if needed. */
			if (q <= 3 * nup) {
				nup++;
				q += l;
				continue;
			}
			/* When previous components are already known to be
			 * directories, processing .. can skip readlink. */
			if (!check_dir) goto skip_readlink;
		}
		ssize_t k = 0;
		if ((at & AT_SYMLINK_NOFOLLOW) && (stack[p] == 0))
			k = __readlinkat2(AT_FDCWD, output, stack, p, true);
		if (k == p) goto toolong;
		if (!k) {
			check_dir = 0;
			if (l0) q += l;
			check_dir = stack[p];
			continue;
		}
		if (k < 0) {
		///	if (errno != EINVAL) goto err;
skip_readlink:
			check_dir = 0;
			if (up) {
				while(q && output[q-1]!='/') q--;
				if (q>1 && (q>2 || output[0]!='/')) q--;
				continue;
			}
			if (l0) q += l;
			check_dir = stack[p];
			continue;
		}
		if (++cnt == SYMLOOP_MAX) {
			errno = ELOOP;
			goto err;
		}

		/* If link contents end in /, strip any slashes already on
		 * stack to avoid /->// or //->/// or spurious toolong. */
		if (stack[k-1]=='/') while (stack[p]=='/') p++;
		p -= k;
		memmove(stack+p, stack, k);

		/* Skip the stack advancement in case we have a new
		 * absolute base path. */
		goto restart;
	}

 	output[q] = 0;

	if (output[0] != '/') {
		if (!getcwd(stack, PATH_MAX+1)) goto err;
		l = strlen(stack);
		/* Cancel any initial .. components. */
		p = 0;
		while (nup--) {
			while(l>1 && stack[l-1]!='/') l--;
			if (l>1) l--;
			p += 2;
			if (p<q) p++;
		}
		if (q-p && stack[l-1]!='/') stack[l++] = '/';
		if (l + (q-p) + 1 >= PATH_MAX) goto toolong;
		memmove(output + l, output + p, q - p + 1);
		memcpy(output, stack, l);
		q = l + q-p;
	}

	char* res = resolved ? memcpy(resolved, output, q+1) : copy_str(output);
	vPortFree(output);
	vPortFree(stack);
	return res;

toolong:
	errno = ENAMETOOLONG;
err:
	vPortFree(output);
	vPortFree(stack);
	return 0;
}
