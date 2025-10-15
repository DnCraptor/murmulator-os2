#include "complex_impl.h"

// FIXME: Hull et al. "Implementing the complex arcsine and arccosine functions using exception handling" 1997
#ifndef M_PI_2
#define M_PI_2          1.57079632679489661923  /* pi/2 */
#endif

/* acos(z) = pi/2 - asin(z) */

double complex cacos(double complex z)
{
	z = casin(z);
	return CMPLX(M_PI_2 - creal(z), -cimag(z));
}
