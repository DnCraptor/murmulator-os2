#include "complex_impl.h"

// FIXME
#ifndef M_PI_2
#define M_PI_2          1.57079632679489661923  /* pi/2 */
#endif

static const float float_pi_2 = M_PI_2;

float complex cacosf(float complex z)
{
	z = casinf(z);
	return CMPLXF(float_pi_2 - crealf(z), -cimagf(z));
}
