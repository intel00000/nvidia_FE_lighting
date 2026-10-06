// diag.c
#include "diag.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#undef log_err
#undef log_info

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_RESET "\x1b[0m"

static struct timespec g_last_ts = {0, 0};
static int g_have_last = 0;

// "[date time]", or "[date time.ms +delta][func]" when func is given
static void make_prefix(char *buf, size_t bufsz, const char *func)
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    struct tm tm_local;
    localtime_r(&now.tv_sec, &tm_local);
    char tsbuf[32];
    strftime(tsbuf, sizeof(tsbuf), "%Y-%m-%d %H:%M:%S", &tm_local);
    if (!func)
    {
        snprintf(buf, bufsz, "[%s]", tsbuf);
        return;
    }
    double delta = 0.0;
    if (g_have_last)
        delta = (double)(now.tv_sec - g_last_ts.tv_sec) + (double)(now.tv_nsec - g_last_ts.tv_nsec) / 1e9;
    g_last_ts = now;
    g_have_last = 1;
    snprintf(buf, bufsz, "[%s.%03d +%.6fs][%s]", tsbuf, (int)(now.tv_nsec / 1000000L), delta, func);
}

// writes the message to out, colored when out is a terminal
__attribute__((format(printf, 5, 0))) static void vlog(FILE *out, const char *color, int stamped, const char *func, const char *format, va_list v)
{
    int tty = isatty(fileno(out));
    if (tty)
        fputs(color, out);
    if (stamped)
    {
        char prefix[128];
        make_prefix(prefix, sizeof(prefix), func);
        fprintf(out, "%s ", prefix);
    }
    vfprintf(out, format, v);
    if (tty)
        fputs(ANSI_COLOR_RESET, out);
    fflush(out);
}

void log_err(const char *format, ...)
{
    va_list v;
    va_start(v, format);
    vlog(stderr, ANSI_COLOR_RED, 0, NULL, format, v);
    va_end(v);
}

void log_info(const char *format, ...)
{
    va_list v;
    va_start(v, format);
    vlog(stdout, ANSI_COLOR_GREEN, 1, NULL, format, v);
    va_end(v);
}

void log_err_ex(const char *func, const char *format, ...)
{
    va_list v;
    va_start(v, format);
    vlog(stderr, ANSI_COLOR_RED, 1, func, format, v);
    va_end(v);
}

void log_info_ex(const char *func, const char *format, ...)
{
    va_list v;
    va_start(v, format);
    vlog(stdout, ANSI_COLOR_GREEN, 1, func, format, v);
    va_end(v);
}
