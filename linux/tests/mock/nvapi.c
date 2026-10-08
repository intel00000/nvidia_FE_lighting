/* nvapi.c - the NvAPI functions felight resolves, serving the rig in $MOCK_NVAPI_STATE.
 * Successful zone writes are saved back to that file, and NvAPI_Initialize and
 * NvAPI_GPU_ClientIllumZonesSetControl calls are appended to $MOCK_NVAPI_LOG. */
#include "mock.h"
#include "nvapi_interface.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HANDLE_BASE 0x1000

const int felight_mock_nvapi = 1;

static mock_rig_t rig;
static int loaded;
static int initialize_calls;

static void load(void)
{
    if (loaded)
        return;
    const char *path = getenv("MOCK_NVAPI_STATE");
    char err[512] = "MOCK_NVAPI_STATE is not set";
    if (!path || rig_load(path, &rig, err, sizeof err) != 0)
    {
        fprintf(stderr, "mock nvapi: %s\n", err);
        exit(99);
    }
    loaded = 1;
}

__attribute__((format(printf, 1, 2))) static void log_call(const char *fmt, ...)
{
    const char *path = getenv("MOCK_NVAPI_LOG");
    FILE *f = path ? fopen(path, "a") : NULL;
    if (!f)
        return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fputc('\n', f);
    fclose(f);
}

static NvPhysicalGpuHandle handle_of(int card)
{
    return (NvPhysicalGpuHandle)(uintptr_t)(HANDLE_BASE + card);
}

static mock_card_t *card_of(NvPhysicalGpuHandle h, int *number)
{
    uintptr_t v = (uintptr_t)h;
    if (v < HANDLE_BASE || v >= HANDLE_BASE + (uintptr_t)rig.ncards)
        return NULL;
    if (number)
        *number = (int)(v - HANDLE_BASE);
    return &rig.cards[v - HANDLE_BASE];
}

static int zone_count(const mock_card_t *c, int reported)
{
    return reported < 0 ? c->nzones : reported;
}

static NvAPI_Status initialize(void)
{
    initialize_calls++;
    if (rig.initialize_failures == MOCK_ALWAYS || initialize_calls <= rig.initialize_failures)
    {
        log_call("Initialize failed");
        return NVAPI_NVIDIA_DEVICE_NOT_FOUND;
    }
    log_call("Initialize");
    return NVAPI_OK;
}

static NvAPI_Status unload(void)
{
    return NVAPI_OK;
}

static NvAPI_Status error_message(NvAPI_Status st, NvAPI_ShortString text)
{
    static const struct
    {
        NvAPI_Status st;
        const char *text;
    } names[] = {
        {NVAPI_ERROR, "NVAPI_ERROR"},
        {NVAPI_INVALID_ARGUMENT, "NVAPI_INVALID_ARGUMENT"},
        {NVAPI_NVIDIA_DEVICE_NOT_FOUND, "NVAPI_NVIDIA_DEVICE_NOT_FOUND"},
        {NVAPI_INCOMPATIBLE_STRUCT_VERSION, "NVAPI_INCOMPATIBLE_STRUCT_VERSION"},
        {NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE, "NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE"},
        {NVAPI_NOT_SUPPORTED, "NVAPI_NOT_SUPPORTED"},
    };
    for (size_t i = 0; i < sizeof names / sizeof names[0]; i++)
        if (names[i].st == st)
        {
            snprintf(text, NVAPI_SHORT_STRING_MAX, "%s", names[i].text);
            return NVAPI_OK;
        }
    snprintf(text, NVAPI_SHORT_STRING_MAX, "status %d", (int)st);
    return NVAPI_OK;
}

static NvAPI_Status enum_gpus(NvPhysicalGpuHandle *handles, NvU32 *count)
{
    if (rig.enumerate_fails)
        return NVAPI_NVIDIA_DEVICE_NOT_FOUND;
    int n = rig.norder < 0 ? rig.ncards : rig.norder;
    for (int i = 0; i < n; i++)
        handles[i] = handle_of(rig.norder < 0 ? i : rig.order[i]);
    *count = (NvU32)n;
    return NVAPI_OK;
}

static NvAPI_Status full_name(NvPhysicalGpuHandle h, NvAPI_ShortString name)
{
    const mock_card_t *c = card_of(h, NULL);
    if (!c)
        return NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE;
    if (c->fails & CARD_FAIL_NAME)
        return NVAPI_ERROR;
    snprintf(name, NVAPI_SHORT_STRING_MAX, "%s", c->name);
    return NVAPI_OK;
}

static NvAPI_Status pci_ids(NvPhysicalGpuHandle h, NvU32 *device, NvU32 *subsystem, NvU32 *revision, NvU32 *ext)
{
    const mock_card_t *c = card_of(h, NULL);
    if (!c)
        return NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE;
    if (!c->has_pci)
        return NVAPI_NOT_SUPPORTED;
    *device = c->device_id;
    *subsystem = c->subsystem_id;
    *revision = c->revision_id;
    *ext = c->ext_device_id;
    return NVAPI_OK;
}

static NvAPI_Status bus_id(NvPhysicalGpuHandle h, NvU32 *bus)
{
    const mock_card_t *c = card_of(h, NULL);
    if (!c)
        return NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE;
    if (!c->has_pci)
        return NVAPI_NOT_SUPPORTED;
    *bus = c->bus_id;
    return NVAPI_OK;
}

static NvAPI_Status gpu_info(NvPhysicalGpuHandle h, NV_GPU_INFO *info)
{
    const mock_card_t *c = card_of(h, NULL);
    if (!c)
        return NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE;
    if (!c->has_info)
        return NVAPI_NOT_SUPPORTED;
    if (info->version != NV_GPU_INFO_VER)
        return NVAPI_INCOMPATIBLE_STRUCT_VERSION;
    info->bIsExternalGpu = c->external;
    info->rayTracingCores = c->rt_cores;
    info->tensorCores = c->tensor_cores;
    return NVAPI_OK;
}

static NvAPI_Status gpu_uuid(NvPhysicalGpuHandle h, NV_GPU_UUID *u)
{
    const mock_card_t *c = card_of(h, NULL);
    if (!c)
        return NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE;
    if (u->version != NV_GPU_UUID_VER)
        return NVAPI_INCOMPATIBLE_STRUCT_VERSION;
    if (c->uuid_mode == UUID_UNSUPPORTED)
        return NVAPI_NOT_SUPPORTED;
    memset(u->uuid, 0, sizeof u->uuid);
    if (c->uuid_mode == UUID_REPORTED)
        memcpy(u->uuid, c->uuid, sizeof u->uuid);
    return NVAPI_OK;
}

static NV_GPU_CLIENT_ILLUM_DEVICE_TYPE device_type(const mock_card_t *c, NvU32 device)
{
    for (int i = 0; i < c->nzones; i++)
        if (c->zones[i].device == device && c->zones[i].type == NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGB)
            return NV_GPU_CLIENT_ILLUM_DEVICE_TYPE_MCUV10;
        else if (c->zones[i].device == device && c->zones[i].type == NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGBW)
            return NV_GPU_CLIENT_ILLUM_DEVICE_TYPE_GPIO_PWM_RGBW_V10;
    return NV_GPU_CLIENT_ILLUM_DEVICE_TYPE_GPIO_PWM_SINGLE_COLOR_V10;
}

static NvAPI_Status devices_info(NvPhysicalGpuHandle h, NV_GPU_CLIENT_ILLUM_DEVICE_INFO_PARAMS *p)
{
    const mock_card_t *c = card_of(h, NULL);
    if (!c)
        return NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE;
    if (c->fails & CARD_FAIL_DEVICES)
        return NVAPI_ERROR;
    if (p->version != NV_GPU_CLIENT_ILLUM_DEVICE_INFO_PARAMS_VER)
        return NVAPI_INCOMPATIBLE_STRUCT_VERSION;
    NvU32 n = 0;
    for (int i = 0; i < c->nzones; i++)
        if (c->zones[i].device + 1u > n)
            n = c->zones[i].device + 1u;
    if (n > NV_GPU_CLIENT_ILLUM_DEVICE_NUM_DEVICES_MAX)
        n = NV_GPU_CLIENT_ILLUM_DEVICE_NUM_DEVICES_MAX;
    p->numIllumDevices = n;
    for (NvU32 i = 0; i < n; i++)
    {
        p->devices[i].type = device_type(c, i);
        p->devices[i].ctrlModeMask = c->mode_mask;
    }
    return NVAPI_OK;
}

static NvAPI_Status zones_info(NvPhysicalGpuHandle h, NV_GPU_CLIENT_ILLUM_ZONE_INFO_PARAMS *p)
{
    const mock_card_t *c = card_of(h, NULL);
    if (!c)
        return NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE;
    if (c->fails & CARD_FAIL_INFO)
        return NVAPI_NOT_SUPPORTED;
    if (p->version != NV_GPU_CLIENT_ILLUM_ZONE_INFO_PARAMS_VER)
        return NVAPI_INCOMPATIBLE_STRUCT_VERSION;
    int n = zone_count(c, c->info_zones);
    p->numIllumZones = (NvU32)n;
    for (int i = 0; i < n; i++)
    {
        const mock_zone_t *z = &c->zones[i];
        p->zones[i].type = (NV_GPU_CLIENT_ILLUM_ZONE_TYPE)z->type;
        p->zones[i].illumDeviceIdx = z->device;
        p->zones[i].provIdx = z->provider;
        p->zones[i].zoneLocation = (NV_GPU_CLIENT_ILLUM_ZONE_LOCATION)z->location;
    }
    return NVAPI_OK;
}

static NvAPI_Status zones_get(NvPhysicalGpuHandle h, NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS *p)
{
    const mock_card_t *c = card_of(h, NULL);
    if (!c)
        return NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE;
    if (c->fails & CARD_FAIL_CONTROL)
        return NVAPI_ERROR;
    if (p->version != NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS_VER)
        return NVAPI_INCOMPATIBLE_STRUCT_VERSION;
    int n = zone_count(c, c->control_zones);
    p->numIllumZonesControl = (NvU32)n;
    for (int i = 0; i < n; i++)
    {
        const mock_zone_t *z = &c->zones[i];
        control_to_nv(z->type, p->bDefault ? &z->stored : &z->active, &p->zones[i]);
    }
    return NVAPI_OK;
}

static NvAPI_Status zones_set(NvPhysicalGpuHandle h, NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS *p)
{
    int number = 0;
    mock_card_t *c = card_of(h, &number);
    if (!c)
        return NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE;
    if (p->version != NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS_VER)
        return NVAPI_INCOMPATIBLE_STRUCT_VERSION;
    int n = zone_count(c, c->control_zones);
    if (p->numIllumZonesControl != (NvU32)n)
        return NVAPI_INVALID_ARGUMENT;
    static mock_control_t next[MOCK_MAX_ZONES];
    char changed[160] = "";
    size_t len = 0;
    int faulty = 0, stored = 0;
    for (int i = 0; i < n; i++)
    {
        const mock_zone_t *z = &c->zones[i];
        const mock_control_t *cur = p->bDefault ? &z->stored : &z->active;
        if (p->zones[i].type != (NV_GPU_CLIENT_ILLUM_ZONE_TYPE)z->type)
            return NVAPI_INVALID_ARGUMENT;
        next[i] = *cur;
        control_from_nv(z->type, &p->zones[i], &next[i]);
        if (control_equal(cur, &next[i]))
            continue;
        len += (size_t)snprintf(changed + len, sizeof changed - len, "%s%d", len ? "," : "", i);
        faulty |= (z->faults & ZONE_FAULT_SET) != 0;
    }
    const char *set = p->bDefault ? " default" : "";
    if (faulty)
    {
        log_call("SetControl card=%d%s zones=%s failed", number, set, changed);
        return NVAPI_ERROR;
    }
    for (int i = 0; i < n; i++)
    {
        mock_zone_t *z = &c->zones[i];
        mock_control_t *cur = p->bDefault ? &z->stored : &z->active;
        if (!(z->faults & ZONE_FAULT_STUCK) && !control_equal(cur, &next[i]))
        {
            *cur = next[i];
            stored = 1;
        }
    }
    log_call("SetControl card=%d%s zones=%s", number, set, len ? changed : "-");
    if (stored && rig_save(getenv("MOCK_NVAPI_STATE"), &rig) != 0)
    {
        fprintf(stderr, "mock nvapi: cannot save the rig\n");
        exit(99);
    }
    return NVAPI_OK;
}

void *nvapi_QueryInterface(unsigned int id)
{
    static const struct
    {
        const char *name;
        void *fn;
    } functions[] = {
        {"NvAPI_Initialize", (void *)initialize},
        {"NvAPI_Unload", (void *)unload},
        {"NvAPI_GetErrorMessage", (void *)error_message},
        {"NvAPI_EnumPhysicalGPUs", (void *)enum_gpus},
        {"NvAPI_GPU_GetFullName", (void *)full_name},
        {"NvAPI_GPU_GetPCIIdentifiers", (void *)pci_ids},
        {"NvAPI_GPU_GetBusId", (void *)bus_id},
        {"NvAPI_GPU_GetGPUInfo", (void *)gpu_info},
        {"NvAPI_GPU_GetUUID", (void *)gpu_uuid},
        {"NvAPI_GPU_ClientIllumDevicesGetInfo", (void *)devices_info},
        {"NvAPI_GPU_ClientIllumZonesGetInfo", (void *)zones_info},
        {"NvAPI_GPU_ClientIllumZonesGetControl", (void *)zones_get},
        {"NvAPI_GPU_ClientIllumZonesSetControl", (void *)zones_set},
    };
    load();
    const char *name = NULL;
    for (size_t i = 0; i < sizeof nvapi_interface_table / sizeof nvapi_interface_table[0] && !name; i++)
        if (nvapi_interface_table[i].id == id)
            name = nvapi_interface_table[i].func;
    if (!name)
        return NULL;
    for (int i = 0; i < rig.nunresolved; i++)
        if (strcmp(rig.unresolved[i], name) == 0)
            return NULL;
    for (size_t i = 0; i < sizeof functions / sizeof functions[0]; i++)
        if (strcmp(functions[i].name, name) == 0)
            return functions[i].fn;
    return NULL;
}
