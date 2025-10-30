#ifndef	_STRING_H
#define	_STRING_H

#ifdef __cplusplus
extern "C" {
#endif

#undef NULL
#if __cplusplus >= 201103L
#define NULL nullptr
#elif defined(__cplusplus)
#define NULL 0L
#else
#define NULL ((void*)0)
#endif

#define __NEED_size_t
#if defined(_POSIX_SOURCE) || defined(_POSIX_C_SOURCE) \
 || defined(_XOPEN_SOURCE) || defined(_GNU_SOURCE) \
 || defined(_BSD_SOURCE)
#define __NEED_locale_t
#endif

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

#include <stddef.h>

/*
int memcmp (const void *, const void *, size_t);
void *memchr (const void *, int, size_t);

char *strncat (char *__restrict, const char *__restrict, size_t);

int strcoll (const char *, const char *);
size_t strxfrm (char *__restrict, const char *__restrict, size_t);

char *strchr (const char *, int);
char *strrchr (const char *, int);

size_t strcspn (const char *, const char *);
size_t strspn (const char *, const char *);
char *strpbrk (const char *, const char *);
char *strstr (const char *, const char *);
char *strtok (char *__restrict, const char *__restrict);
*/
void* memset(void* p, int v, size_t sz) {
    typedef void* (*fn)(void *, int, size_t);
    return ((fn)_sys_table_ptrs[142])(p, v, sz);
}

void* memcpy(void *__restrict dst, const void *__restrict src, size_t sz) {
    typedef void* (*fn)(void *, const void*, size_t);
    return ((fn)_sys_table_ptrs[167])(dst, src, sz);
}

char* strcpy(char* t, const char * s) {
    typedef char* (*fn_ptr_t)(char*, const char*);
    return ((fn_ptr_t)_sys_table_ptrs[60])(t, s);
}

void* memmove(void* dst, const void* src, size_t sz) {
    typedef void* (*fn)(void *, const void*, size_t);
    return ((fn)_sys_table_ptrs[232])(dst, src, sz);
}

char* strcat(char* t, const char * s) {
    typedef char* (*fn_ptr_t)(char*, const char*);
    return ((fn_ptr_t)_sys_table_ptrs[252])(t, s);
}

inline static size_t strlen(const char * s) {
    typedef size_t (*fn_ptr_t)(const char * s);
    return ((fn_ptr_t)_sys_table_ptrs[62])(s);
}

inline static char* strncpy(char* t, const char * s, size_t sz) {
    typedef char* (*fn_ptr_t)(char*, const char*, size_t);
    return ((fn_ptr_t)_sys_table_ptrs[63])(t, s, sz);
}

inline static int strcmp(const char * s1, const char * s2) {
    typedef int (*fn_ptr_t)(const char*, const char*);
    return ((fn_ptr_t)_sys_table_ptrs[108])(s1, s2);
}

inline static int strncmp(const char * s1, const char * s2, size_t sz) {
    typedef int (*fn_ptr_t)(const char*, const char*, size_t);
    return ((fn_ptr_t)_sys_table_ptrs[109])(s1, s2, sz);
}

inline static char* strerror (int code) {
    typedef char* (*fn_ptr_t)(int);
    return ((fn_ptr_t)_sys_table_ptrs[353])(code);
}

#ifdef __cplusplus
}
#endif

#endif // _STRING_H
