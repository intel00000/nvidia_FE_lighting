/* profile.c - writing profile files. */
#include "profile.h"
#include "diag.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

int profile_save(const char *path, const gpu_t *g, const zone_t *zones, NvU32 nzones, int delay_seconds)
{
    FILE *f = fopen(path, "w");
    if (!f)
    {
        log_err("felight: cannot write %s: %s\n", path, strerror(errno));
        return EXIT_FILE;
    }
    time_t now = time(NULL);
    char stamp[64];
    strftime(stamp, sizeof stamp, "%Y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(f, "# NVIDIA lighting profile written by felight %s on %s\n", FELIGHT_VERSION, stamp);
    fprintf(f, "delay_seconds=%d\n", delay_seconds);
    fprintf(f, "gpu_index=%u\n", g->index);
    fprintf(f, "gpu_name=%s\n", g->name);
    if (g->has_ids)
    {
        fprintf(f, "gpu_bus_id=%u\n", g->bus_id);
        fprintf(f, "gpu_device_id=0x%08x\n", g->device_id);
        fprintf(f, "gpu_subsystem_id=0x%08x\n", g->subsystem_id);
    }
    if (g->has_uuid)
        fprintf(f, "gpu_uuid=%s\n", g->uuid);
    for (NvU32 i = 0; i < nzones; i++)
    {
        const zone_t *z = &zones[i];
        if (!z->present_in_control || z->type == NV_GPU_CLIENT_ILLUM_ZONE_TYPE_INVALID)
            continue;
        NvU8 r = z->piecewise ? z->ep_r[0] : z->r, gg = z->piecewise ? z->ep_g[0] : z->g, b = z->piecewise ? z->ep_b[0] : z->b;
        NvU8 w = z->piecewise ? z->ep_w[0] : z->w, br = z->piecewise ? z->ep_brightness[0] : z->brightness;
        fprintf(f, "zone %u %s", z->index, zone_type_key(z->type));
        if (zone_has_color(z->type))
            fprintf(f, " r=%u g=%u b=%u", r, gg, b);
        if (zone_has_white(z->type))
            fprintf(f, " w=%u", w);
        fprintf(f, " brightness=%u\n", br);
    }
    if (fclose(f) != 0)
    {
        log_err("felight: error writing %s: %s\n", path, strerror(errno));
        return EXIT_FILE;
    }
    return 0;
}
