/* MOS runtime floating-point conversions.
 * Reuse the existing MOS musl-derived scanf floating-point parser (API 336).
 * NOTE: This is a bootstrap adapter, not a fully conforming strtod:
 * scanf and strtod differ on incomplete exponents and errno handling.
 */
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

double strtod(const char *restrict s, char **restrict endptr)
{
    double result = 0.0;
    int consumed = 0;
    /* %n is handled by MOS's musl-derived vsscanf implementation. */
    if (sscanf(s, "%lf%n", &result, &consumed) != 1) {
        if (endptr) *endptr = (char *)s;
        return 0.0;
    }
    if (endptr) *endptr = (char *)s + consumed;
    return result;
}

/* Bootstrap conversions.  ARM EABI GCC uses binary64 for long double.
 * Full strtof/strtold conformance remains future runtime work.
 * strtof via double can double-round at exceptional halfway cases.
 */
float strtof(const char *restrict s, char **restrict endptr)
{
    return (float)strtod(s, endptr);
}

long double strtold(const char *restrict s, char **restrict endptr)
{
    _Static_assert(sizeof(long double) == sizeof(double),
                   "MOS runtime strtold requires binary64 long double");
    return (long double)strtod(s, endptr);
}
