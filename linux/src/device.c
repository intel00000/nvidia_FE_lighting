/* device.c - reading GPUs and zones from the driver. */
#include "device.h"
#include "diag.h"

#include <stdio.h>
#include <string.h>

int check_gpu_index(nvapi_t *nv, long idx)
{
    if (idx < 0 || idx >= (long)nv->gpu_count)
    {
        log_err("felight: GPU index %ld out of range; %u GPU(s) detected.\n", idx, nv->gpu_count);
        return 0;
    }
    return 1;
}

int read_gpu(nvapi_t *nv, NvU32 index, gpu_t *g)
{
    memset(g, 0, sizeof *g);
    g->index = index;
    NvPhysicalGpuHandle h = nv->gpus[index];
    NvAPI_ShortString name = {0};
    NvAPI_Status st = nv->GPU_GetFullName(h, name);
    if (st != NVAPI_OK)
    {
        log_err("felight: NvAPI_GPU_GetFullName(%u) failed: %s\n", index, nv_strerror(nv, st));
        return EXIT_NVAPI;
    }
    snprintf(g->name, sizeof g->name, "%s", name);
    if (nv->GPU_GetPCIIdentifiers && nv->GPU_GetBusId)
    {
        NvU32 bus = 0;
        if (nv->GPU_GetPCIIdentifiers(h, &g->device_id, &g->subsystem_id, &g->revision_id, &g->ext_device_id) == NVAPI_OK &&
            nv->GPU_GetBusId(h, &bus) == NVAPI_OK)
        {
            g->bus_id = bus;
            g->has_ids = 1;
        }
    }
    if (nv->GPU_GetGPUInfo)
    {
        NV_GPU_INFO info;
        memset(&info, 0, sizeof info);
        info.version = NV_GPU_INFO_VER;
        if (nv->GPU_GetGPUInfo(h, &info) == NVAPI_OK)
        {
            g->has_info = 1;
            g->rt_cores = info.rayTracingCores;
            g->tensor_cores = info.tensorCores;
            g->external = info.bIsExternalGpu ? 1 : 0;
        }
    }
    return 0;
}

/* Read info + control (active or default) for every zone of a GPU. Returns 0 or an exit code. */
int read_zones(nvapi_t *nv, NvU32 gpu, int use_default, zone_t *zones, NvU32 *count)
{
    static NV_GPU_CLIENT_ILLUM_ZONE_INFO_PARAMS info;
    static NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS ctrl;
    static NV_GPU_CLIENT_ILLUM_DEVICE_INFO_PARAMS dev;
    NvPhysicalGpuHandle h = nv->gpus[gpu];
    NvAPI_Status st;

    memset(&info, 0, sizeof info);
    info.version = NV_GPU_CLIENT_ILLUM_ZONE_INFO_PARAMS_VER;
    st = nv->IllumZonesGetInfo(h, &info);
    nv_pace();
    if (st != NVAPI_OK)
    {
        log_err("felight: NvAPI_GPU_ClientIllumZonesGetInfo failed: %s\n", nv_strerror(nv, st));
        return EXIT_NVAPI;
    }
    memset(&ctrl, 0, sizeof ctrl);
    ctrl.version = NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS_VER;
    ctrl.bDefault = use_default ? 1 : 0;
    st = nv->IllumZonesGetControl(h, &ctrl);
    nv_pace();
    if (st != NVAPI_OK)
    {
        log_err("felight: NvAPI_GPU_ClientIllumZonesGetControl failed: %s\n", nv_strerror(nv, st));
        return EXIT_NVAPI;
    }
    int have_dev = 0;
    if (nv->IllumDevicesGetInfo)
    {
        memset(&dev, 0, sizeof dev);
        dev.version = NV_GPU_CLIENT_ILLUM_DEVICE_INFO_PARAMS_VER;
        have_dev = (nv->IllumDevicesGetInfo(h, &dev) == NVAPI_OK);
        nv_pace();
    }

    NvU32 n_info = info.numIllumZones > NV_GPU_CLIENT_ILLUM_ZONE_NUM_ZONES_MAX ? NV_GPU_CLIENT_ILLUM_ZONE_NUM_ZONES_MAX : info.numIllumZones;
    NvU32 n_ctrl = ctrl.numIllumZonesControl > NV_GPU_CLIENT_ILLUM_ZONE_NUM_ZONES_MAX ? NV_GPU_CLIENT_ILLUM_ZONE_NUM_ZONES_MAX : ctrl.numIllumZonesControl;
    NvU32 n = n_info > n_ctrl ? n_info : n_ctrl;
    memset(zones, 0, sizeof(zone_t) * NV_GPU_CLIENT_ILLUM_ZONE_NUM_ZONES_MAX);
    for (NvU32 i = 0; i < n; i++)
    {
        zone_t *z = &zones[i];
        z->index = i;
        if (i < n_info)
        {
            z->present_in_info = 1;
            z->type = info.zones[i].type;
            z->location = info.zones[i].zoneLocation;
            z->device_index = info.zones[i].illumDeviceIdx;
            z->prov_index = info.zones[i].provIdx;
            if (have_dev && z->device_index < dev.numIllumDevices && z->device_index < NV_GPU_CLIENT_ILLUM_DEVICE_NUM_DEVICES_MAX)
            {
                z->has_mode_mask = 1;
                z->ctrl_mode_mask = dev.devices[z->device_index].ctrlModeMask;
            }
        }
        else
        {
            z->location = NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_INVALID;
        }
        if (i < n_ctrl)
            zone_read_control(z, &ctrl.zones[i]);
    }
    *count = n;
    return 0;
}
