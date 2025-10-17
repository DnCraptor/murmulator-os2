#include <stddef.h>
#include <dynlink.h>

#ifndef __ARM_ARCH_6M__

ptrdiff_t __tlsdesc_static()
{
	return 0;
}

weak_alias(__tlsdesc_static, __tlsdesc_dynamic);

#endif
