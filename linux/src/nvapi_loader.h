/* nvapi_loader.h - the NvAPI functions felight resolves from libnvidia-api.so.1. */
#ifndef NVAPI_LOADER_H
#define NVAPI_LOADER_H

#include "felight.h"

#include <stddef.h>

typedef struct
{
    void *lib;
    char libpath[256];
    void *(*qi)(unsigned int);
    NvAPI_Status (*Initialize)(void);
    NvAPI_Status (*Unload)(void);
    NvAPI_Status (*GetErrorMessage)(NvAPI_Status, NvAPI_ShortString);
    NvAPI_Status (*EnumPhysicalGPUs)(NvPhysicalGpuHandle *, NvU32 *);
    NvAPI_Status (*GPU_GetFullName)(NvPhysicalGpuHandle, NvAPI_ShortString);
    NvAPI_Status (*GPU_GetPCIIdentifiers)(NvPhysicalGpuHandle, NvU32 *, NvU32 *, NvU32 *, NvU32 *);
    NvAPI_Status (*GPU_GetBusId)(NvPhysicalGpuHandle, NvU32 *);
    NvAPI_Status (*GPU_GetGPUInfo)(NvPhysicalGpuHandle, NV_GPU_INFO *);
    NvAPI_Status (*IllumDevicesGetInfo)(NvPhysicalGpuHandle, NV_GPU_CLIENT_ILLUM_DEVICE_INFO_PARAMS *);
    NvAPI_Status (*IllumZonesGetInfo)(NvPhysicalGpuHandle, NV_GPU_CLIENT_ILLUM_ZONE_INFO_PARAMS *);
    NvAPI_Status (*IllumZonesGetControl)(NvPhysicalGpuHandle, NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS *);
    NvAPI_Status (*IllumZonesSetControl)(NvPhysicalGpuHandle, NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS *);
    NvPhysicalGpuHandle gpus[NVAPI_MAX_PHYSICAL_GPUS];
    NvU32 gpu_count;
    int initialized;
} nvapi_t;

extern nvapi_t g_nv;

const char *nv_strerror(nvapi_t *nv, NvAPI_Status st);
void nv_pace(void);
int nv_open(nvapi_t *nv);
void nv_close(nvapi_t *nv);
void driver_version(char *buf, size_t len);

#endif
