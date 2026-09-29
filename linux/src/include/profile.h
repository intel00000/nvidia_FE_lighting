/* profile.h - profile files: save zone settings for one GPU. */
#ifndef PROFILE_H
#define PROFILE_H

#include "model.h"

int profile_save(const char *path, const gpu_t *g, const zone_t *zones, NvU32 nzones, int delay_seconds);

#endif
