/* cmd_set.c - the set command. */
#include "commands.h"
#include "device.h"
#include "diag.h"
#include "parse.h"

#include <stdio.h>
#include <string.h>

int cmd_set(int argc, char **argv)
{
    set_opts_t o = {0};
    o.verify = 1;
    long gpu = -1, zone = -1, value;
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "--gpu") == 0)
        {
            if (!parse_opt("--gpu", argv[++i], 0, MAX_GPU_INDEX, &gpu))
                return EXIT_USAGE;
        }
        else if (strcmp(argv[i], "--zone") == 0)
        {
            if (!parse_opt("--zone", argv[++i], 0, MAX_ZONE_INDEX, &zone))
                return EXIT_USAGE;
        }
        else if (strcmp(argv[i], "--rgb") == 0 || strcmp(argv[i], "--color") == 0)
        {
            const char *arg = i + 1 < argc ? argv[++i] : "";
            if (!parse_rgb(arg, &o.r, &o.g, &o.b))
            {
                log_err("felight set: bad color '%s' (use R,G,B with 0-255 or #RRGGBB)\n", arg);
                return EXIT_USAGE;
            }
            o.has_rgb = 1;
        }
        else if (strcmp(argv[i], "--white") == 0)
        {
            if (!parse_opt("--white", argv[++i], 0, 255, &value))
                return EXIT_USAGE;
            o.w = (int)value;
            o.has_white = 1;
        }
        else if (strcmp(argv[i], "--brightness") == 0)
        {
            if (!parse_opt("--brightness", argv[++i], 0, 100, &value))
                return EXIT_USAGE;
            o.brightness = (int)value;
            o.has_brightness = 1;
        }
        else if (strcmp(argv[i], "--default") == 0)
            o.use_default = 1;
        else if (strcmp(argv[i], "--no-verify") == 0)
            o.verify = 0;
        else
        {
            log_err("felight set: unknown option %s\n", argv[i]);
            return EXIT_USAGE;
        }
    }
    if (gpu < 0 || zone < 0)
    {
        log_err("felight set: --gpu N and --zone Z are required\n");
        return EXIT_USAGE;
    }
    if (!o.has_rgb && !o.has_white && !o.has_brightness)
    {
        log_err("felight set: nothing to set (use --rgb, --white, --brightness)\n");
        return EXIT_USAGE;
    }
    int rc = nv_open(&g_nv);
    if (rc)
        return rc;
    if (!check_gpu_index(&g_nv, gpu))
        return EXIT_USAGE;
    rc = set_zone(&g_nv, (NvU32)gpu, (NvU32)zone, &o, 0);
    if (rc == 0)
        printf("ok\n");
    return rc;
}
