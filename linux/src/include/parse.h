/* parse.h - parsers for command-line options and profile values. */
#ifndef PARSE_H
#define PARSE_H

#include "felight.h"

char *trim(char *s);
int parse_u32_base(const char *s, int base, NvU32 *out);
int parse_u32(const char *s, NvU32 *out);
int parse_opt(const char *opt, const char *s, long lo, long hi, long *out);

#endif
