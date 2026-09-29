/* nvapi_loader.c - load libnvidia-api.so.1 and resolves NvAPI functions via nvapi_QueryInterface. */
#include "nvapi_loader.h"
#include "nvapi_interface.h"
#include "diag.h"

#include <ctype.h>
#include <dlfcn.h>
#include <link.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* The struct layouts must match what the driver expects. */
_Static_assert(sizeof(NV_GPU_CLIENT_ILLUM_ZONE_INFO_PARAMS) == 4552, "unexpected ZONE_INFO_PARAMS size");
_Static_assert(sizeof(NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS) == 6476, "unexpected ZONE_CONTROL_PARAMS size");
_Static_assert(sizeof(NV_GPU_CLIENT_ILLUM_DEVICE_INFO_PARAMS) == 4424, "unexpected DEVICE_INFO_PARAMS size");

nvapi_t g_nv;

static unsigned int nv_interface_id(const char *name)
{
    size_t n = sizeof(nvapi_interface_table) / sizeof(nvapi_interface_table[0]);
    for (size_t i = 0; i < n; i++)
        if (strcmp(nvapi_interface_table[i].func, name) == 0)
            return nvapi_interface_table[i].id;
    return 0;
}

/* Returns NULL when the driver does not implement the interface. */
static void *nv_resolve(nvapi_t *nv, const char *name)
{
    unsigned int id = nv_interface_id(name);
    if (!id)
    {
        log_err("felight: internal error: '%s' is not in nvapi_interface.h\n", name);
        exit(EXIT_NVAPI);
    }
    return nv->qi(id);
}

const char *nv_strerror(nvapi_t *nv, NvAPI_Status st)
{
    static NvAPI_ShortString msg;
    if (nv->GetErrorMessage && nv->GetErrorMessage(st, msg) == NVAPI_OK && msg[0])
        return msg;
    snprintf(msg, sizeof msg, "NvAPI status %d", (int)st);
    return msg;
}

/* OpenRGB pauses ~30 ms after every NvAPI illumination call; the FE controller has been seen to
 * drop back-to-back writes, so we do the same. */
void nv_pace(void)
{
    usleep(30 * 1000);
}

int nv_open(nvapi_t *nv)
{
    static const char *const names[] = {"libnvidia-api.so.1", "libnvidia-api.so", NULL};
    if (nv->initialized)
        return 0;
    for (int i = 0; names[i] && !nv->lib; i++)
    {
        nv->lib = dlopen(names[i], RTLD_NOW | RTLD_LOCAL);
        if (nv->lib)
            snprintf(nv->libpath, sizeof nv->libpath, "%s", names[i]);
    }
    if (!nv->lib)
    {
        log_err("felight: cannot load libnvidia-api.so.1 (%s).\n"
                "         It ships with the NVIDIA driver since release 525; is the driver installed?\n",
                dlerror());
        return EXIT_NO_LIBRARY;
    }
    struct link_map *lm = NULL;
    if (dlinfo(nv->lib, RTLD_DI_LINKMAP, &lm) == 0 && lm && lm->l_name && lm->l_name[0])
        snprintf(nv->libpath, sizeof nv->libpath, "%s", lm->l_name);

    *(void **)&nv->qi = dlsym(nv->lib, "nvapi_QueryInterface");
    if (!nv->qi)
    {
        log_err("felight: %s has no nvapi_QueryInterface; driver too old?\n", nv->libpath);
        return EXIT_NO_LIBRARY;
    }
#define RESOLVE(field, name) *(void **)&nv->field = nv_resolve(nv, name)
    RESOLVE(Initialize, "NvAPI_Initialize");
    RESOLVE(Unload, "NvAPI_Unload");
    RESOLVE(GetErrorMessage, "NvAPI_GetErrorMessage");
    RESOLVE(EnumPhysicalGPUs, "NvAPI_EnumPhysicalGPUs");
    RESOLVE(GPU_GetFullName, "NvAPI_GPU_GetFullName");
    RESOLVE(GPU_GetPCIIdentifiers, "NvAPI_GPU_GetPCIIdentifiers");
    RESOLVE(GPU_GetBusId, "NvAPI_GPU_GetBusId");
    RESOLVE(GPU_GetGPUInfo, "NvAPI_GPU_GetGPUInfo");
    RESOLVE(GPU_GetUUID, "NvAPI_GPU_GetUUID");
    RESOLVE(IllumDevicesGetInfo, "NvAPI_GPU_ClientIllumDevicesGetInfo");
    RESOLVE(IllumZonesGetInfo, "NvAPI_GPU_ClientIllumZonesGetInfo");
    RESOLVE(IllumZonesGetControl, "NvAPI_GPU_ClientIllumZonesGetControl");
    RESOLVE(IllumZonesSetControl, "NvAPI_GPU_ClientIllumZonesSetControl");
#undef RESOLVE
    if (!nv->Initialize || !nv->EnumPhysicalGPUs || !nv->GPU_GetFullName ||
        !nv->IllumZonesGetInfo || !nv->IllumZonesGetControl || !nv->IllumZonesSetControl)
    {
        log_err("felight: %s does not implement the illumination interface.\n", nv->libpath);
        return EXIT_NO_LIBRARY;
    }
    NvAPI_Status st = nv->Initialize();
    if (st != NVAPI_OK)
    {
        log_err("felight: NvAPI_Initialize failed: %s\n", nv_strerror(nv, st));
        return EXIT_NVAPI;
    }
    nv->initialized = 1;
    st = nv->EnumPhysicalGPUs(nv->gpus, &nv->gpu_count);
    if (st != NVAPI_OK)
    {
        log_err("felight: NvAPI_EnumPhysicalGPUs failed: %s\n", nv_strerror(nv, st));
        return EXIT_NVAPI;
    }
    return 0;
}

void nv_close(nvapi_t *nv)
{
    if (nv->initialized && nv->Unload)
        nv->Unload();
    nv->initialized = 0;
    if (nv->lib)
        dlclose(nv->lib);
    nv->lib = NULL;
}

void driver_version(char *buf, size_t len)
{
    buf[0] = 0;
    FILE *f = fopen("/proc/driver/nvidia/version", "r");
    if (!f)
        return;
    char line[512];
    if (fgets(line, sizeof line, f))
    {
        /* "NVRM version: NVIDIA UNIX Open Kernel Module for x86_64  610.57.04  Release Build ..." */
        char *p = strstr(line, "Module for");
        if (!p)
            p = line;
        char *tok = strtok(p, " \t\n");
        while (tok)
        {
            // if start with a digit and contains a dot, assume it's the version string
            if (isdigit((unsigned char)tok[0]) && strchr(tok, '.'))
            {
                snprintf(buf, len, "%s", tok);
                break;
            }
            tok = strtok(NULL, " \t\n");
        }
    }
    fclose(f);
}
