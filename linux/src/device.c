/* device.c - reading GPUs and zones from the driver, and writing illumination zone. */
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

static int clamp(int v, int lo, int hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

/* Get-modify-set on one zone. The zone is forced into MANUAL mode (the Windows tool could not
 * do that, which left zones stuck in piecewise mode). Fields that are not given keep their
 * current manual value. */
int set_zone(nvapi_t *nv, NvU32 gpu, NvU32 zone, const set_opts_t *o, int quiet)
{
    static NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS ctrl;
    NvPhysicalGpuHandle h = nv->gpus[gpu];
    NvAPI_Status st;

    memset(&ctrl, 0, sizeof ctrl);
    ctrl.version = NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS_VER;
    ctrl.bDefault = o->use_default ? 1 : 0;
    st = nv->IllumZonesGetControl(h, &ctrl);
    nv_pace();
    if (st != NVAPI_OK)
    {
        log_err("felight: NvAPI_GPU_ClientIllumZonesGetControl failed: %s\n", nv_strerror(nv, st));
        return EXIT_NVAPI;
    }
    if (zone >= ctrl.numIllumZonesControl || zone >= NV_GPU_CLIENT_ILLUM_ZONE_NUM_ZONES_MAX)
    {
        log_err("felight: GPU %u has %u zone(s); zone %u does not exist.\n", gpu, ctrl.numIllumZonesControl, zone);
        return EXIT_USAGE;
    }
    NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_V1 *c = &ctrl.zones[zone];
    zone_t cur;
    memset(&cur, 0, sizeof cur);
    zone_read_control(&cur, c);
    int base_r = cur.piecewise ? cur.ep_r[0] : cur.r;
    int base_g = cur.piecewise ? cur.ep_g[0] : cur.g;
    int base_b = cur.piecewise ? cur.ep_b[0] : cur.b;
    int base_w = cur.piecewise ? cur.ep_w[0] : cur.w;
    int base_br = cur.piecewise ? cur.ep_brightness[0] : cur.brightness;

    int r = o->has_rgb ? clamp(o->r, 0, 255) : base_r;
    int g = o->has_rgb ? clamp(o->g, 0, 255) : base_g;
    int b = o->has_rgb ? clamp(o->b, 0, 255) : base_b;
    int w = o->has_white ? clamp(o->w, 0, 255) : base_w;
    int br = o->has_brightness ? clamp(o->brightness, 0, 100) : base_br;

    if (o->has_rgb && !zone_has_color(c->type))
    {
        log_err("felight: zone %u is %s; it has no color, only brightness.\n", zone, zone_type_label(c->type));
        return EXIT_USAGE;
    }
    if (o->has_white && !zone_has_white(c->type))
    {
        log_err("felight: zone %u is %s; it has no white channel.\n", zone, zone_type_label(c->type));
        return EXIT_USAGE;
    }
    if (c->ctrlMode != NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL && !quiet)
        log_err("felight: zone %u was in %s mode; switching it to manual.\n", zone, ctrl_mode_key(c->ctrlMode));

    c->ctrlMode = NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL;
    memset(&c->data, 0, sizeof c->data);
    switch (c->type)
    {
    case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGB:
    {
        NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGB_PARAMS *p = &c->data.rgb.data.manualRGB.rgbParams;
        p->colorR = (NvU8)r;
        p->colorG = (NvU8)g;
        p->colorB = (NvU8)b;
        p->brightnessPct = (NvU8)br;
        break;
    }
    case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGBW:
    {
        NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGBW_PARAMS *p = &c->data.rgbw.data.manualRGBW.rgbwParams;
        p->colorR = (NvU8)r;
        p->colorG = (NvU8)g;
        p->colorB = (NvU8)b;
        p->colorW = (NvU8)w;
        p->brightnessPct = (NvU8)br;
        break;
    }
    case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_COLOR_FIXED:
        c->data.colorFixed.data.manualColorFixed.colorFixedParams.brightnessPct = (NvU8)br;
        break;
    case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_SINGLE_COLOR:
        c->data.singleColor.data.manualSingleColor.singleColorParams.brightnessPct = (NvU8)br;
        break;
    default:
        log_err("felight: zone %u has unsupported type %s.\n", zone, zone_type_label(c->type));
        return EXIT_USAGE;
    }

    st = nv->IllumZonesSetControl(h, &ctrl);
    nv_pace();
    if (st != NVAPI_OK)
    {
        log_err("felight: NvAPI_GPU_ClientIllumZonesSetControl failed: %s\n", nv_strerror(nv, st));
        return EXIT_NVAPI;
    }
    if (!o->verify)
        return 0;

    memset(&ctrl, 0, sizeof ctrl);
    ctrl.version = NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS_VER;
    ctrl.bDefault = o->use_default ? 1 : 0;
    st = nv->IllumZonesGetControl(h, &ctrl);
    nv_pace();
    if (st != NVAPI_OK)
    {
        log_err("felight: read-back failed: %s\n", nv_strerror(nv, st));
        return EXIT_VERIFY;
    }
    zone_t after;
    memset(&after, 0, sizeof after);
    zone_read_control(&after, &ctrl.zones[zone]);
    int ok = !after.piecewise && after.brightness == br;
    if (zone_has_color(c->type))
        ok = ok && after.r == r && after.g == g && after.b == b;
    if (zone_has_white(c->type))
        ok = ok && after.w == w;
    if (!ok)
    {
        log_err("felight: zone %u read-back mismatch: wanted r=%d g=%d b=%d w=%d brightness=%d, got mode=%s r=%u g=%u b=%u w=%u brightness=%u\n",
                zone, r, g, b, w, br, ctrl_mode_key(after.ctrl_mode), after.r, after.g, after.b, after.w, after.brightness);
        return EXIT_VERIFY;
    }
    return 0;
}
