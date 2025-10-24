#ifndef __STDIO_H
#define __STDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#define TYPEDEF typedef
#define STRUCT struct

TYPEDEF __builtin_va_list va_list;
TYPEDEF __builtin_va_list __isoc_va_list;
#define FILE void

int __printf(const char *__restrict, ...);
int __vprintf(const char *__restrict, __isoc_va_list);
int __vfprintf(FILE *restrict f, const char *restrict fmt, va_list ap);
size_t __fwritex(const unsigned char *restrict s, size_t l, FILE *restrict f);
size_t __fwrite(const void *restrict src, size_t size, size_t nmemb, FILE *restrict f);
FILE* __fopen(const char *restrict filename, const char *restrict mode);
int __fclose(FILE *f);
int __fflush(FILE *f);
size_t __fread(void *__restrict b, size_t n1, size_t n2, FILE *__restrict f);
int __fputc(int c, FILE *f);
void __rewind(FILE *f);
int __fseek(FILE *f, long off, int whence);
int __fseeko(FILE *f, long long off, int whence);
int __fgetc(FILE *f);
int __ungetc(int c, FILE *f);
char* __fgets(char *restrict s, int n, FILE *restrict f);
long __ftell(FILE *);
long long __ftello(FILE *);
int __fgetpos(FILE *__restrict, fpos_t *__restrict);
int __fsetpos(FILE *, const fpos_t *);
int __feof(FILE *);
int __ferror(FILE *);
void __clearerr(FILE *);
FILE* __freopen(const char *__restrict, const char *__restrict, FILE *__restrict);
FILE *const __stdin();
FILE *const __stdout();
FILE *const __stderr();

#undef FILE

#ifdef __cplusplus
}
#endif

#endif // __STDIO_H
