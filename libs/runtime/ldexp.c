/* MOS runtime binary floating-point scaling.
 * Based on the public-domain-style musl scalbn algorithm (musl MIT license).
 */
#include <stdint.h>
#include <math.h>

double ldexp(double x, int n)
{
    union { double f; uint64_t i; } u;
    double y = x;
    if (n > 1023) {
        y *= 0x1p1023;
        n -= 1023;
        if (n > 1023) {
            y *= 0x1p1023;
            n -= 1023;
            if (n > 1023) n = 1023;
        }
    } else if (n < -1022) {
        y *= 0x1p-1022;
        n += 1022;
        if (n < -1022) {
            y *= 0x1p-1022;
            n += 1022;
            if (n < -1022) n = -1022;
        }
    }
    u.i = (uint64_t)(0x3ff + n) << 52;
    return y * u.f;
}
