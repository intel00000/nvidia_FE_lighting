/* cmd_startup.c - delayed apply of a settings file. */
#include "commands.h"
#include "apply.h"
#include "diag.h"
#include "nvapi_loader.h"
#include "profile.h"

#include <string.h>
#include <unistd.h>

int cmd_startup(int argc, char **argv)
{
    int no_delay = 0;
    const char *path = NULL;
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "--no-delay") == 0)
            no_delay = 1;
        else if (argv[i][0] == '-')
        {
            log_err("felight startup: unknown option %s\n", argv[i]);
            return EXIT_USAGE;
        }
        else
            path = argv[i];
    }
    if (!path)
    {
        log_err("felight startup: usage: startup [--no-delay] FILE\n");
        return EXIT_USAGE;
    }
    log_info("felight %s startup mode, settings %s\n", FELIGHT_VERSION, path);

    profile_t p;
    int rc = profile_load(path, &p);
    if (rc)
    {
        log_info("no startup settings found; exiting.\n");
        return rc;
    }
    if (p.nzones == 0)
    {
        log_info("startup settings contain no zones; exiting.\n");
        return EXIT_FILE;
    }
    if (!no_delay && p.delay_seconds > 0)
    {
        log_info("waiting %d second(s) for the driver to settle...\n", p.delay_seconds);
        sleep((unsigned)p.delay_seconds);
    }
    for (int attempt = 1;; attempt++)
    {
        rc = nv_open(&g_nv);
        if (rc == 0 || attempt == 5)
            break;
        log_info("NvAPI not ready (attempt %d/5), retrying in 2 s...\n", attempt);
        nv_close(&g_nv);
        memset(&g_nv, 0, sizeof g_nv);
        sleep(2);
    }
    if (rc)
    {
        log_info("NvAPI unavailable; exiting.\n");
        return rc;
    }
    log_info("NvAPI ready via %s, %u GPU(s).\n", g_nv.libpath, g_nv.gpu_count);
    rc = apply_profile(&p, -1, 1, log_info);
    log_info("startup mode finished with exit code %d.\n", rc);
    return rc;
}
