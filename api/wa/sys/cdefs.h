/// TODO: think about it
/// W/A for MUSL

/// for MUSL:
#ifndef hidden
#define	hidden	__attribute__((__visibility__("hidden")))
#endif

#include_next <sys/cdefs.h>
