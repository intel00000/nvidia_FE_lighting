/* parse.c - parsers for command-line options and profile values. */
#include "parse.h"
#include "diag.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

char *trim(char *s)
{
    while (isspace((unsigned char)*s))
        s++;
    char *e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1]))
        *--e = 0;
    return s;
}

/* Unsigned 32-bit parser: no sign, no trailing junk, no wrap-around. Base 10 unless the
 * caller indicates otherwise. */
int parse_u32_base(const char *s, int base, NvU32 *out)
{
    while (isspace((unsigned char)*s))
        s++;
    if (*s == '-' || *s == '+')
        return 0;
    char *end = NULL;
    errno = 0;
    unsigned long v = strtoul(s, &end, base);
    if (errno || end == s || *end || v > 0xFFFFFFFFul)
        return 0;
    *out = (NvU32)v;
    return 1;
}

int parse_u32(const char *s, NvU32 *out)
{
    return parse_u32_base(s, 10, out);
}

/* Parses a numeric command-line option and range-checks it, printing a usage error on failure. */
int parse_opt(const char *opt, const char *s, long lo, long hi, long *out)
{
    NvU32 v;
    if (!s || !parse_u32(s, &v) || (long)v < lo || (long)v > hi)
    {
        log_err("felight: %s expects a number from %ld to %ld, got '%s'\n", opt, lo, hi, s ? s : "");
        return 0;
    }
    *out = (long)v;
    return 1;
}
