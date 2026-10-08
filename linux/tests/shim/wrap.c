/* wrap.c - linked into the test builds of felight with -Wl,--wrap=dlopen,--wrap=sleep,--wrap=usleep.
 * Every dlopen loads $MOCK_NVAPI_LIB, so a test build never loads the driver's libnvidia-api.so.1.
 * Waits return at once; sleep() is logged instead. */
#include <dlfcn.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MOCK_MARKER "felight_mock_nvapi"

void *__real_dlopen(const char *file, int flags);
void *__wrap_dlopen(const char *file, int flags);
unsigned int __wrap_sleep(unsigned int seconds);
int __wrap_usleep(useconds_t usec);

__attribute__((noreturn, format(printf, 1, 2))) static void refuse(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fputs("test build: ", stderr);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    abort();
}

/* 1 when the file names the mock's marker symbol, 0 when it does not, -1 when it cannot be read. */
static int names_marker(const char *path)
{
    static char head[1 << 20];
    FILE *f = fopen(path, "rb");
    if (!f)
        return -1;
    size_t n = fread(head, 1, sizeof head, f);
    fclose(f);
    return memmem(head, n, MOCK_MARKER, sizeof MOCK_MARKER - 1) != NULL;
}

void *__wrap_dlopen(const char *file, int flags)
{
    (void)file;
    const char *mock = getenv("MOCK_NVAPI_LIB");
    if (!mock || !*mock)
        refuse("MOCK_NVAPI_LIB is not set\n");
    if (mock[0] != '/')
        refuse("MOCK_NVAPI_LIB must be an absolute path, not %s\n", mock);
    if (names_marker(mock) == 0)
        refuse("%s is not the mock NvAPI library\n", mock);
    void *lib = __real_dlopen(mock, flags);
    if (lib && !dlsym(lib, MOCK_MARKER))
        refuse("%s is not the mock NvAPI library\n", mock);
    return lib;
}

unsigned int __wrap_sleep(unsigned int seconds)
{
    const char *log = getenv("MOCK_NVAPI_LOG");
    FILE *f = log ? fopen(log, "a") : NULL;
    if (f)
    {
        fprintf(f, "sleep %u\n", seconds);
        fclose(f);
    }
    return 0;
}

int __wrap_usleep(useconds_t usec)
{
    (void)usec;
    return 0;
}
