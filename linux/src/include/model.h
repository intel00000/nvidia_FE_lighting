/* model.h - GPU and illumination zones definition. */
#ifndef MODEL_H
#define MODEL_H

#include "felight.h"

#define MAX_GPU_INDEX (NVAPI_MAX_PHYSICAL_GPUS - 1)
#define MAX_ZONE_INDEX (NV_GPU_CLIENT_ILLUM_ZONE_NUM_ZONES_MAX - 1)

typedef struct
{
    NvU32 index;
    char name[NVAPI_SHORT_STRING_MAX];
    int has_ids;
    NvU32 bus_id, device_id, subsystem_id, revision_id, ext_device_id;
    int has_uuid;
    char uuid[41]; /* "GPU-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" */
    int has_info;
    NvU32 rt_cores, tensor_cores;
    int external;
} gpu_t;

typedef struct
{
    NvU32 index;
    int present_in_info, present_in_control;
    NV_GPU_CLIENT_ILLUM_ZONE_TYPE type;
    NV_GPU_CLIENT_ILLUM_ZONE_LOCATION location;
    NvU8 device_index, prov_index;
    int has_mode_mask;
    NvU32 ctrl_mode_mask;
    NV_GPU_CLIENT_ILLUM_CTRL_MODE ctrl_mode;
    NvU8 r, g, b, w, brightness;
    int piecewise;
    NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR pw;
    NvU8 ep_r[2], ep_g[2], ep_b[2], ep_w[2], ep_brightness[2];
} zone_t;

const char *zone_type_key(NV_GPU_CLIENT_ILLUM_ZONE_TYPE t);
const char *zone_type_label(NV_GPU_CLIENT_ILLUM_ZONE_TYPE t);
int zone_type_from_key(const char *key, NV_GPU_CLIENT_ILLUM_ZONE_TYPE *out);
const char *location_key(NV_GPU_CLIENT_ILLUM_ZONE_LOCATION l);
const char *location_label(NV_GPU_CLIENT_ILLUM_ZONE_LOCATION l);
const char *ctrl_mode_key(NV_GPU_CLIENT_ILLUM_CTRL_MODE m);
const char *cycle_key(NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_TYPE c);
int zone_has_color(NV_GPU_CLIENT_ILLUM_ZONE_TYPE t);
int zone_has_white(NV_GPU_CLIENT_ILLUM_ZONE_TYPE t);
void zone_read_control(zone_t *z, const NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_V1 *c);

#endif
