/* cmd_query.c - the list and get commands. */
#include "commands.h"
#include "device.h"
#include "diag.h"
#include "output.h"
#include "parse.h"

#include <stdio.h>
#include <string.h>

int cmd_list(int argc, char **argv)
{
    int json = 0;
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "--json") == 0)
            json = 1;
        else
        {
            log_err("felight list: unknown option %s\n", argv[i]);
            return EXIT_USAGE;
        }
    }
    int rc = nv_open(&g_nv);
    if (rc)
        return rc;
    char drv[64];
    driver_version(drv, sizeof drv);

    /* Read everything first so a GPU without illumination support neither aborts the
    listing nor leaves half-printed JSON behind. */
    static gpu_t gpus[NVAPI_MAX_PHYSICAL_GPUS];
    static zone_t zones[NVAPI_MAX_PHYSICAL_GPUS][NV_GPU_CLIENT_ILLUM_ZONE_NUM_ZONES_MAX];
    static NvU32 counts[NVAPI_MAX_PHYSICAL_GPUS];
    static int failed[NVAPI_MAX_PHYSICAL_GPUS];
    for (NvU32 i = 0; i < g_nv.gpu_count; i++)
    {
        counts[i] = 0;
        failed[i] = read_gpu(&g_nv, i, &gpus[i]);
        if (!failed[i])
            failed[i] = read_zones(&g_nv, i, 0, zones[i], &counts[i]);
        if (failed[i])
        {
            if (!gpus[i].name[0])
                snprintf(gpus[i].name, sizeof gpus[i].name, "GPU %u", i);
            gpus[i].index = i;
            counts[i] = 0;
            log_err("felight: GPU %u (%s): illumination zones unavailable, listed without zones.\n", i, gpus[i].name);
        }
    }

    if (json)
    {
        printf("{\"library\":");
        json_string(stdout, g_nv.libpath);
        printf(",\"driver\":");
        json_string(stdout, drv);
        printf(",\"gpus\":[");
    }
    else
        printf("driver %s via %s, %u GPU(s)\n", drv[0] ? drv : "unknown", g_nv.libpath, g_nv.gpu_count);

    for (NvU32 i = 0; i < g_nv.gpu_count; i++)
    {
        if (json)
        {
            if (i)
                putchar(',');
            print_gpu_json(stdout, &gpus[i], zones[i], counts[i], failed[i]);
        }
        else
            print_gpu_text(stdout, &gpus[i], zones[i], counts[i]);
    }
    if (json)
        printf("]}\n");
    return 0;
}

int cmd_get(int argc, char **argv)
{
    static zone_t zones[NV_GPU_CLIENT_ILLUM_ZONE_NUM_ZONES_MAX];
    int json = 0, use_default = 0;
    long gpu = -1;
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "--json") == 0)
            json = 1;
        else if (strcmp(argv[i], "--default") == 0)
            use_default = 1;
        else if (strcmp(argv[i], "--gpu") == 0)
        {
            if (!parse_opt("--gpu", argv[++i], 0, MAX_GPU_INDEX, &gpu))
                return EXIT_USAGE;
        }
        else
        {
            log_err("felight get: unknown option %s\n", argv[i]);
            return EXIT_USAGE;
        }
    }
    if (gpu < 0)
    {
        log_err("felight get: --gpu N is required\n");
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
    if ((rc = read_zones(&g_nv, (NvU32)gpu, use_default, zones, &n)))
        return rc;
    if (json)
    {
        print_gpu_json(stdout, &g, zones, n, 0);
        putchar('\n');
    }
    else
        print_gpu_text(stdout, &g, zones, n);
    return 0;
}
