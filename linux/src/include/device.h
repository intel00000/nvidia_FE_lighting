/* device.h - reading GPUs and zones from the driver. */
#ifndef DEVICE_H
#define DEVICE_H

#include "model.h"
#include "nvapi_loader.h"

int check_gpu_index(nvapi_t *nv, long idx);
int read_gpu(nvapi_t *nv, NvU32 index, gpu_t *g);
int read_zones(nvapi_t *nv, NvU32 gpu, int use_default, zone_t *zones, NvU32 *count);

#endif
