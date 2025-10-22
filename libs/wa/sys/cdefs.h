/// TODO: think about it
/// W/A for MUSL

/// for MUSL:
#ifndef hidden
#define	hidden	__attribute__((__visibility__("hidden")))
#endif
#ifndef weak
#define	weak	__attribute__((weak))
#endif


#include_next <sys/cdefs.h>
