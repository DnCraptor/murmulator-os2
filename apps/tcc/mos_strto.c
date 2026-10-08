/* MOS TinyCC compatibility: integer strto* conversions.
 * TODO(kernel): export these standard functions through MOS syscall table;
 * then replace these app-local implementations with libc inline proxies.
 */
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

static unsigned long long parse_unsigned(const char *s, char **endptr,
                                         int base, unsigned long long limit,
                                         int *negative, int *overflow)
{
    const unsigned char *p = (const unsigned char *)s;
    const unsigned char *start = p;
    unsigned long long value = 0;
    int any = 0;
    while (*p == ' ' || (*p >= 9 && *p <= 13)) ++p;
    *negative = 0;
    if (*p == '-' || *p == '+') { *negative = (*p == '-'); ++p; }
    if (base != 0 && (base < 2 || base > 36)) {
        errno = EINVAL;
        if (endptr) *endptr = (char *)s;
        return 0;
    }
    if ((base == 0 || base == 16) && p[0] == '0' &&
        (p[1] == 'x' || p[1] == 'X')) {
        unsigned char c = p[2];
        if ((c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
            p += 2;
            base = 16;
        }
    }
    if (!base) base = (*p == '0') ? 8 : 10;
    *overflow = 0;
    for (;;) {
        unsigned c = *p;
        unsigned digit;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'z') digit = c - 'a' + 10;
        else if (c >= 'A' && c <= 'Z') digit = c - 'A' + 10;
        else break;
        if (digit >= (unsigned)base) break;
        any = 1;
        if (value > (limit - digit) / (unsigned)base) *overflow = 1;
        if (!*overflow) value = value * (unsigned)base + digit;
        ++p;
    }
    if (endptr) *endptr = (char *)(any ? p : start);
    if (*overflow) { errno = ERANGE; return limit; }
    return value;
}

unsigned long long strtoull(const char *s, char **endptr, int base)
{
    int negative, overflow;
    unsigned long long v = parse_unsigned(s, endptr, base, ULLONG_MAX,
                                           &negative, &overflow);
    return negative && !overflow ? 0ULL - v : v;
}

long long strtoll(const char *s, char **endptr, int base)
{
    int negative, overflow;
    unsigned long long limit = (unsigned long long)LLONG_MAX + 1ULL;
    unsigned long long v = parse_unsigned(s, endptr, base,
                                           limit, &negative, &overflow);
    if (!negative && v > (unsigned long long)LLONG_MAX) overflow = 1;
    if (overflow) { errno = ERANGE; return negative ? LLONG_MIN : LLONG_MAX; }
    if (negative && v == limit) return LLONG_MIN;
    return negative ? -(long long)v : (long long)v;
}

unsigned long strtoul(const char *s, char **endptr, int base)
{
    int negative, overflow;
    unsigned long long v = parse_unsigned(s, endptr, base, ULONG_MAX,
                                           &negative, &overflow);
    return negative && !overflow ? 0UL - (unsigned long)v : (unsigned long)v;
}

long strtol(const char *s, char **endptr, int base)
{
    int negative, overflow;
    unsigned long long limit = (unsigned long long)LONG_MAX + 1ULL;
    unsigned long long v = parse_unsigned(s, endptr, base,
                                           limit, &negative, &overflow);
    if (!negative && v > (unsigned long long)LONG_MAX) overflow = 1;
    if (overflow) { errno = ERANGE; return negative ? LONG_MIN : LONG_MAX; }
    if (negative && v == limit) return LONG_MIN;
    return negative ? -(long)v : (long)v;
}
