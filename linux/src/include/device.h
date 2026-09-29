/* device.h - reading GPUs and zones from the driver, and writing illumination zone. */
#ifndef DEVICE_H
#define DEVICE_H

#include "model.h"
#include "nvapi_loader.h"

typedef struct
{
    int has_rgb, has_white, has_brightness;
    int r, g, b, w, brightness;
    int use_default;
    int verify;
} set_opts_t;

int check_gpu_index(nvapi_t *nv, long idx);
int read_gpu(nvapi_t *nv, NvU32 index, gpu_t *g);
int read_zones(nvapi_t *nv, NvU32 gpu, int use_default, zone_t *zones, NvU32 *count);
int set_zone(nvapi_t *nv, NvU32 gpu, NvU32 zone, const set_opts_t *o, int quiet);

#endif
