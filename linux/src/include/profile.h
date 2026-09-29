/* profile.h - profile files: save zone settings for one GPU. */
#ifndef PROFILE_H
#define PROFILE_H

#include "model.h"

#define MAX_PROFILE_ZONES NV_GPU_CLIENT_ILLUM_ZONE_NUM_ZONES_MAX

typedef struct
{
    NvU32 index;
    NV_GPU_CLIENT_ILLUM_ZONE_TYPE type;
    int has_rgb, has_white, has_brightness;
    int r, g, b, w, brightness;
} profile_zone_t;

typedef struct
{
    int delay_seconds;
    int has_gpu_index;
    NvU32 gpu_index;
    int has_gpu_ids;
    int bad_identity; /* a gpu_* line was present but unreadable */
    NvU32 bus_id, device_id, subsystem_id;
    int has_gpu_uuid;
    char gpu_uuid[41];
    char gpu_name[NVAPI_SHORT_STRING_MAX];
    profile_zone_t zones[MAX_PROFILE_ZONES];
    int nzones;
} profile_t;

int profile_save(const char *path, const gpu_t *g, const zone_t *zones, NvU32 nzones, int delay_seconds);
int profile_load(const char *path, profile_t *p);

#endif
