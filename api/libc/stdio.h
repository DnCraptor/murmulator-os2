#ifndef _STDIO_H
#define _STDIO_H

#ifdef __cplusplus
extern "C" {
#endif

//#include <features.h>

#define __NEED_FILE
#define __NEED___isoc_va_list
#define __NEED_size_t

#if __STDC_VERSION__ < 201112L
#define __NEED_struct__IO_FILE
#endif

#if defined(_POSIX_SOURCE) || defined(_POSIX_C_SOURCE) \
 || defined(_XOPEN_SOURCE) || defined(_GNU_SOURCE) \
 || defined(_BSD_SOURCE)
#define __NEED_ssize_t
#define __NEED_off_t
#define __NEED_va_list
#endif

#undef off_t
#ifndef _OFF_T_DECLARED
typedef long long off_t;
typedef long long _off_t;
#define __machine_off_t_defined
#define	_OFF_T_DECLARED
#endif

#include <stddef.h>

#ifndef _OWN_IO_FILE 
struct _IO_FILE { char __x; };
#endif
typedef struct _IO_FILE FILE;

#undef NULL
#if __cplusplus >= 201103L
#define NULL nullptr
#elif defined(__cplusplus)
#define NULL 0L
#else
#define NULL ((void*)0)
#endif

#undef EOF
#define EOF (-1)

#undef SEEK_SET
#undef SEEK_CUR
#undef SEEK_END
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

#define BUFSIZ 1024
#define FILENAME_MAX 4096
#define FOPEN_MAX 1000
#define TMP_MAX 10000
#define L_tmpnam 20

typedef union _G_fpos64_t {
	char __opaque[16];
	long long __lldata;
	double __align;
} fpos_t;

inline static FILE* fopen(const char *__restrict filename, const char *__restrict mode) {
    typedef FILE* (*fn_ptr_t)(const char *__restrict, const char *__restrict);
    return ((fn_ptr_t)_sys_table_ptrs[313])(filename, mode);
}
inline static int fclose(FILE* f) {
    typedef int (*fn_ptr_t)(FILE*);
    return ((fn_ptr_t)_sys_table_ptrs[314])(f);
}
inline static int fflush(FILE* f) {
    typedef int (*fn_ptr_t)(FILE*);
    return ((fn_ptr_t)_sys_table_ptrs[315])(f);
}
inline static size_t fread(void *__restrict b, size_t n1, size_t n2, FILE *__restrict f) {
    typedef size_t (*fn_ptr_t)(void *__restrict, size_t, size_t, FILE *__restrict);
    return ((fn_ptr_t)_sys_table_ptrs[316])(b, n1, n2, f);
}
inline static size_t fwrite(const void *__restrict b, size_t n1, size_t n2, FILE *__restrict f) {
    typedef size_t (*fn_ptr_t)(const void *__restrict, size_t, size_t, FILE *__restrict);
    return ((fn_ptr_t)_sys_table_ptrs[312])(b, n1, n2, f);
}
inline static int fputc(int c, FILE *f) {
    typedef int (*fn_ptr_t)(int, FILE *);
    return ((fn_ptr_t)_sys_table_ptrs[317])(c, f);
}
inline static int putc(int c, FILE *f) {
    typedef int (*fn_ptr_t)(int, FILE *);
    return ((fn_ptr_t)_sys_table_ptrs[317])(c, f);
}
inline static void rewind(FILE *f) {
    typedef void (*fn_ptr_t)(FILE *);
    return ((fn_ptr_t)_sys_table_ptrs[318])(f);
}
inline static int fseek(FILE *f, long off, int whence) {
    typedef int (*fn_ptr_t)(FILE*, long, int);
    return ((fn_ptr_t)_sys_table_ptrs[319])(f, off, whence);
}
inline static int fgetc(FILE *f) {
    typedef int (*fn_ptr_t)(FILE *);
    return ((fn_ptr_t)_sys_table_ptrs[320])(f);
}
inline static int getc(FILE *f) {
    typedef int (*fn_ptr_t)(FILE *);
    return ((fn_ptr_t)_sys_table_ptrs[320])(f);
}
inline static int ungetc(int c, FILE *f) {
    typedef int (*fn_ptr_t)(int, FILE *);
    return ((fn_ptr_t)_sys_table_ptrs[321])(c, f);
}
inline static char* fgets(char *restrict s, int n, FILE *restrict f) {
    typedef char* (*fn_ptr_t)(char *restrict s, int n, FILE *restrict f);
    return ((fn_ptr_t)_sys_table_ptrs[322])(s, n, f);
}
inline static int fseeko(FILE *f, off_t off, int whence) {
    typedef int (*fn_ptr_t)(FILE*, off_t, int);
    return ((fn_ptr_t)_sys_table_ptrs[323])(f, off, whence);
}
inline static off_t ftello(FILE* f) {
    typedef off_t (*fn_ptr_t)(FILE*);
    return ((fn_ptr_t)_sys_table_ptrs[324])(f);
}
inline static long ftell(FILE* f) {
	off_t pos = ftello(f);
	if (pos > 0x7FFFFFFF) { // LONG_MAX
		errno = EOVERFLOW;
		return -1;
	}
	return pos;
}

inline static int fgetpos(FILE *__restrict f, fpos_t *__restrict pos) {
    typedef int (*fn_ptr_t)(FILE *__restrict, fpos_t *__restrict);
    return ((fn_ptr_t)_sys_table_ptrs[325])(f, pos);
}

inline static int fsetpos(FILE* f, const fpos_t* pos) {
    typedef int (*fn_ptr_t)(FILE*, const fpos_t*);
    return ((fn_ptr_t)_sys_table_ptrs[326])(f, pos);
}

inline static int feof(FILE* f) {
    typedef int (*fn_ptr_t)(FILE*);
    return ((fn_ptr_t)_sys_table_ptrs[327])(f);
}
inline static int ferror(FILE* f) {
    typedef int (*fn_ptr_t)(FILE*);
    return ((fn_ptr_t)_sys_table_ptrs[328])(f);
}
inline static void clearerr(FILE* f) {
    typedef void (*fn_ptr_t)(FILE*);
    ((fn_ptr_t)_sys_table_ptrs[329])(f);
}
inline static FILE* freopen(const char *__restrict fn, const char *__restrict m, FILE *__restrict f) {
    typedef FILE* (*fn_ptr_t)(const char *__restrict, const char *__restrict, FILE *__restrict);
    ((fn_ptr_t)_sys_table_ptrs[330])(fn, m, f);
}

inline static FILE *const stdin() {
    typedef FILE *const (*fn_ptr_t)();
    ((fn_ptr_t)_sys_table_ptrs[331])();
}
inline static FILE *const stdout() {
    typedef FILE *const (*fn_ptr_t)();
    ((fn_ptr_t)_sys_table_ptrs[332])();
}
inline static FILE *const stderr() {
    typedef FILE *const (*fn_ptr_t)();
    ((fn_ptr_t)_sys_table_ptrs[333])();
}
inline static int dup3(int oldfd, int newfd, int flags) {
    typedef int (*fn_ptr_t)(int, int, int);
    return ((fn_ptr_t)_sys_table_ptrs[334])(oldfd,  newfd, flags);
}

#define stdin  (stdin())
#define stdout (stdout())
#define stderr (stderr())
/*
int remove(const char *);
int rename(const char *, const char *);

int getchar(void);
int putchar(int);

char *fgets(char *__restrict, int, FILE *__restrict);
#if __STDC_VERSION__ < 201112L
char *gets(char *);
#endif

int fputs(const char *__restrict, FILE *__restrict);
int puts(const char *);

int printf0(const char *__restrict, ...);
*/

typedef void (*__goutf_ptr_t)(const char *__restrict str, ...) __attribute__ ((__format__ (__printf__, 1, 2)));
#define printf(...) (((__goutf_ptr_t)_sys_table_ptrs[308])(__VA_ARGS__))

/*
int fprintf(FILE *__restrict, const char *__restrict, ...);
int sprintf(char *__restrict, const char *__restrict, ...);
int snprintf(char *__restrict, size_t, const char *__restrict, ...);

int vprintf(const char *__restrict, __isoc_va_list);
int vfprintf(FILE *__restrict, const char *__restrict, __isoc_va_list);
int vsprintf(char *__restrict, const char *__restrict, __isoc_va_list);
int vsnprintf(char *__restrict, size_t, const char *__restrict, __isoc_va_list);

int scanf(const char *__restrict, ...);
int fscanf(FILE *__restrict, const char *__restrict, ...);
int sscanf(const char *__restrict, const char *__restrict, ...);
int vscanf(const char *__restrict, __isoc_va_list);
int vfscanf(FILE *__restrict, const char *__restrict, __isoc_va_list);
int vsscanf(const char *__restrict, const char *__restrict, __isoc_va_list);

void perror(const char *);

int setvbuf(FILE *__restrict, char *__restrict, int, size_t);
void setbuf(FILE *__restrict, char *__restrict);

char *tmpnam(char *);
FILE *tmpfile(void);

#if defined(_POSIX_SOURCE) || defined(_POSIX_C_SOURCE) \
 || defined(_XOPEN_SOURCE) || defined(_GNU_SOURCE) \
 || defined(_BSD_SOURCE)
FILE *fmemopen(void *__restrict, size_t, const char *__restrict);
FILE *open_memstream(char **, size_t *);
FILE *fdopen(int, const char *);
FILE *popen(const char *, const char *);
int pclose(FILE *);
int fileno(FILE *);
int dprintf(int, const char *__restrict, ...);
int vdprintf(int, const char *__restrict, __isoc_va_list);
void flockfile(FILE *);
int ftrylockfile(FILE *);
void funlockfile(FILE *);
int getc_unlocked(FILE *);
int getchar_unlocked(void);
int putc_unlocked(int, FILE *);
int putchar_unlocked(int);
ssize_t getdelim(char **__restrict, size_t *__restrict, int, FILE *__restrict);
ssize_t getline(char **__restrict, size_t *__restrict, FILE *__restrict);
int renameat(int, const char *, int, const char *);
char *ctermid(char *);
#define L_ctermid 20
#endif

#if defined(_GNU_SOURCE)
#define RENAME_NOREPLACE (1 << 0)
#define RENAME_EXCHANGE  (1 << 1)
#define RENAME_WHITEOUT  (1 << 2)

int renameat2(int, const char *, int, const char *, unsigned);
#endif

#if defined(_XOPEN_SOURCE) || defined(_GNU_SOURCE) \
 || defined(_BSD_SOURCE)
#define P_tmpdir "/tmp"
char *tempnam(const char *, const char *);
#endif

#if defined(_GNU_SOURCE) || defined(_BSD_SOURCE)
#define L_cuserid 20
char *cuserid(char *);
void setlinebuf(FILE *);
void setbuffer(FILE *, char *, size_t);
int fgetc_unlocked(FILE *);
int fputc_unlocked(int, FILE *);
int fflush_unlocked(FILE *);
size_t fread_unlocked(void *, size_t, size_t, FILE *);
size_t fwrite_unlocked(const void *, size_t, size_t, FILE *);
void clearerr_unlocked(FILE *);
int feof_unlocked(FILE *);
int ferror_unlocked(FILE *);
int fileno_unlocked(FILE *);
int getw(FILE *);
int putw(int, FILE *);
char *fgetln(FILE *, size_t *);
int asprintf(char **, const char *, ...);
int vasprintf(char **, const char *, __isoc_va_list);
#endif

#ifdef _GNU_SOURCE
char *fgets_unlocked(char *, int, FILE *);
int fputs_unlocked(const char *, FILE *);

typedef ssize_t (cookie_read_function_t)(void *, char *, size_t);
typedef ssize_t (cookie_write_function_t)(void *, const char *, size_t);
typedef int (cookie_seek_function_t)(void *, off_t *, int);
typedef int (cookie_close_function_t)(void *);

typedef struct _IO_cookie_io_functions_t {
	cookie_read_function_t *read;
	cookie_write_function_t *write;
	cookie_seek_function_t *seek;
	cookie_close_function_t *close;
} cookie_io_functions_t;

FILE *fopencookie(void *, const char *, cookie_io_functions_t);
#endif

#if defined(_LARGEFILE64_SOURCE)
#define tmpfile64 tmpfile
#define fopen64 fopen
#define freopen64 freopen
#define fseeko64 fseeko
#define ftello64 ftello
#define fgetpos64 fgetpos
#define fsetpos64 fsetpos
#define fpos64_t fpos_t
#define off64_t off_t
#endif
*/
#ifdef __cplusplus
}
#endif

#endif
