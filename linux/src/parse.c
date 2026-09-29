/* parse.c - parsers for command-line options, profile values and colors. */
#include "parse.h"
#include "diag.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
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

int parse_rgb(const char *s, int *r, int *g, int *b)
{
    if (s[0] == '#')
    {
        if (strlen(s) != 7)
            return 0;
        for (int i = 1; i < 7; i++)
            if (!isxdigit((unsigned char)s[i]))
                return 0;
        unsigned v = (unsigned)strtoul(s + 1, NULL, 16);
        *r = (int)((v >> 16) & 0xff);
        *g = (int)((v >> 8) & 0xff);
        *b = (int)(v & 0xff);
        return 1;
    }
    /* Exactly three comma-separated decimal components, each 0-255. */
    char copy[64];
    if (strlen(s) >= sizeof copy)
        return 0;
    snprintf(copy, sizeof copy, "%s", s);
    int *out[3] = {r, g, b};
    char *save = NULL;
    char *tok = strtok_r(copy, ",", &save);
    for (int i = 0; i < 3; i++)
    {
        NvU32 v;
        if (!tok || !parse_u32(tok, &v) || v > 255)
            return 0;
        *out[i] = (int)v;
        tok = strtok_r(NULL, ",", &save);
    }
    if (tok || strstr(s, ",,") || s[strlen(s) - 1] == ',')
        return 0;
    return 1;
}
