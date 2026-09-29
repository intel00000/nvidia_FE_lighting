// diag.h
#pragma once
#include <stdio.h>

// startup log, once it is open every message is written to it with a timestamp
extern FILE *g_log;

// log_err(format, ...)
// Writes an error or warning to stderr, red on a terminal. The caller ends the message with \n.
__attribute__((format(printf, 1, 2))) void log_err(const char *format, ...);

// log_info(format, ...)
// Writes a timestamped progress line to stdout, green on a terminal. The caller ends the message with \n.
__attribute__((format(printf, 1, 2))) void log_info(const char *format, ...);

// DEBUG builds route log_err/log_info here to prefix each line with the time, the delta since the previous line and the caller
__attribute__((format(printf, 2, 3))) void log_err_ex(const char *func, const char *format, ...);
__attribute__((format(printf, 2, 3))) void log_info_ex(const char *func, const char *format, ...);
#ifdef DEBUG
#define log_err(...) log_err_ex(__func__, __VA_ARGS__)
#define log_info(...) log_info_ex(__func__, __VA_ARGS__)
#endif
