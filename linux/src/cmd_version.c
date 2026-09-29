/* cmd_version.c - get the felight and driver version */
#include "commands.h"
#include "nvapi_loader.h"

#include <dlfcn.h>
#include <link.h>
#include <stdio.h>

int cmd_version(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    char drv[64];
    driver_version(drv, sizeof drv);
    printf("felight %s\n", FELIGHT_VERSION);
    printf("driver: %s\n", drv[0] ? drv : "unknown");
    void *lib = dlopen("libnvidia-api.so.1", RTLD_NOW | RTLD_LOCAL);
    if (lib)
    {
        struct link_map *lm = NULL;
        printf("nvapi library: %s\n", (dlinfo(lib, RTLD_DI_LINKMAP, &lm) == 0 && lm) ? lm->l_name : "libnvidia-api.so.1");
        dlclose(lib);
    }
    else
    {
        printf("nvapi library: not found\n");
    }
    return 0;
}
