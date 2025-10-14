#ifndef _BITS_ALLTYPES_H
#define _BITS_ALLTYPES_H

#include <stddef.h>
#include <sys/stat.h>

/// TODO:
typedef struct FILE_s {
	int fildes;
} FILE;
typedef unsigned int ssize_t;
typedef __builtin_va_list __gnuc_va_list;
typedef __builtin_va_list __isoc_va_list;

#endif // _BITS_ALLTYPES_H
