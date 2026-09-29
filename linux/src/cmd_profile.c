/* cmd_profile.c - the save and apply commands. */
#include "apply.h"
#include "commands.h"
#include "device.h"
#include "diag.h"
#include "parse.h"
#include "profile.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

int cmd_save(int argc, char **argv)
{
    static zone_t zones[NV_GPU_CLIENT_ILLUM_ZONE_NUM_ZONES_MAX];
    long gpu = -1, delay = 10;
    const char *path = NULL;
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "--gpu") == 0)
        {
            if (!parse_opt("--gpu", argv[++i], 0, MAX_GPU_INDEX, &gpu))
                return EXIT_USAGE;
        }
        else if (strcmp(argv[i], "--delay") == 0)
        {
            if (!parse_opt("--delay", argv[++i], 0, MAX_DELAY_SECONDS, &delay))
                return EXIT_USAGE;
        }
        else if (argv[i][0] == '-')
        {
            log_err("felight save: unknown option %s\n", argv[i]);
            return EXIT_USAGE;
        }
        else
            path = argv[i];
    }
    if (gpu < 0 || !path)
    {
        log_err("felight save: usage: save --gpu N [--delay S] FILE\n");
        return EXIT_USAGE;
    }
    int rc = nv_open(&g_nv);
    if (rc)
        return rc;
    if (!check_gpu_index(&g_nv, gpu))
        return EXIT_USAGE;
    gpu_t g;
    NvU32 n = 0;
    if ((rc = read_gpu(&g_nv, (NvU32)gpu, &g)))
        return rc;
    if ((rc = read_zones(&g_nv, (NvU32)gpu, 0, zones, &n)))
        return rc;
    if ((rc = profile_save(path, &g, zones, n, (int)delay)))
        return rc;
    printf("saved %u zone(s) of GPU %u to %s\n", n, (NvU32)gpu, path);
    return 0;
}

__attribute__((format(printf, 1, 2))) static void say_plain(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
}

int cmd_apply(int argc, char **argv)
{
    long gpu = -1;
    int verify_gpu = 1;
    const char *path = NULL;
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "--gpu") == 0)
        {
            if (!parse_opt("--gpu", argv[++i], 0, MAX_GPU_INDEX, &gpu))
                return EXIT_USAGE;
        }
        else if (strcmp(argv[i], "--no-verify-gpu") == 0)
            verify_gpu = 0;
        else if (argv[i][0] == '-')
        {
            log_err("felight apply: unknown option %s\n", argv[i]);
            return EXIT_USAGE;
        }
        else
            path = argv[i];
    }
    if (!path)
    {
        log_err("felight apply: usage: apply [--gpu N] [--no-verify-gpu] FILE\n");
        return EXIT_USAGE;
    }
    profile_t p;
    int rc = profile_load(path, &p);
    if (rc)
        return rc;
    if (p.nzones == 0)
    {
        log_err("felight apply: %s contains no zone lines\n", path);
        return EXIT_FILE;
    }
    return apply_profile(&p, gpu, verify_gpu, say_plain);
}
