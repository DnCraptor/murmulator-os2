#include "stdio_impl.h"
#include <errno.h>

int __fseeko_unlocked(FILE *f, off_t off, int whence)
{
	/* Fail immediately for invalid whence argument. */
	if (whence != SEEK_CUR && whence != SEEK_SET && whence != SEEK_END) {
		errno = EINVAL;
		return -1;
	}

	/* Adjust relative offset for unread data in buffer, if any. */
	if (whence == SEEK_CUR && f->rend) off -= f->rend - f->rpos;
kprintf("__fseeko_unlocked: %d, %d, %d, %d, %d\n", f->fd, f->flags, f->wpos, f->wbase, errno);
	/* Flush write buffer, and report error on failure. */
	if (f->wpos != f->wbase) {
		f->write(f, 0, 0);
kprintf("f->write(f, 0, 0) failed: %d, %d, %d, %d, %d\n", f->fd, f->flags, f->wpos, f->wbase, errno);
		if (!f->wpos) return -1;
	}
kprintf("3\n");

	/* Leave writing mode */
	f->wpos = f->wbase = f->wend = 0;

	/* Perform the underlying seek. */
	if (f->seek(f, off, whence) < 0) {
kprintf("4\n");
		return -1;
	}
kprintf("5\n");

	/* If seek succeeded, file is seekable and we discard read buffer. */
	f->rpos = f->rend = 0;
	f->flags &= ~F_EOF;
	
	return 0;
}

int __fseeko(FILE *f, off_t off, int whence)
{
	int result;
	FLOCK(f);
	result = __fseeko_unlocked(f, off, whence);
	FUNLOCK(f);
	return result;
}

int fseek(FILE *f, long off, int whence)
{
	return __fseeko(f, off, whence);
}

weak_alias(__fseeko, fseeko);
