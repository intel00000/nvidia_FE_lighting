/* model.c - NvAPI illumination enum values mapping. */
#include "model.h"

#include <string.h>

const char *zone_type_key(NV_GPU_CLIENT_ILLUM_ZONE_TYPE t)
{
    switch (t)
    {
    case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGB:
        return "rgb";
    case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_COLOR_FIXED:
        return "color_fixed";
    case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGBW:
        return "rgbw";
    case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_SINGLE_COLOR:
        return "single_color";
    case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_INVALID:
        return "invalid";
    default:
        return "unknown";
    }
}

const char *zone_type_label(NV_GPU_CLIENT_ILLUM_ZONE_TYPE t)
{
    switch (t)
    {
    case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGB:
        return "RGB";
    case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_COLOR_FIXED:
        return "Color Fixed";
    case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGBW:
        return "RGBW";
    case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_SINGLE_COLOR:
        return "Single Color";
    case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_INVALID:
        return "Invalid";
    default:
        return "Reserved or Unknown";
    }
}

int zone_type_from_key(const char *key, NV_GPU_CLIENT_ILLUM_ZONE_TYPE *out)
{
    static const struct
    {
        const char *k;
        NV_GPU_CLIENT_ILLUM_ZONE_TYPE t;
    } map[] = {
        {"rgb", NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGB},
        {"rgbw", NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGBW},
        {"single_color", NV_GPU_CLIENT_ILLUM_ZONE_TYPE_SINGLE_COLOR},
        {"color_fixed", NV_GPU_CLIENT_ILLUM_ZONE_TYPE_COLOR_FIXED},
    };
    for (size_t i = 0; i < sizeof map / sizeof map[0]; i++)
        if (strcmp(map[i].k, key) == 0)
        {
            *out = map[i].t;
            return 1;
        }
    return 0;
}

const char *location_key(NV_GPU_CLIENT_ILLUM_ZONE_LOCATION l)
{
    switch (l)
    {
    case NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_GPU_TOP_0:
        return "gpu_top";
    case NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_GPU_FRONT_0:
        return "gpu_front";
    case NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_GPU_BACK_0:
        return "gpu_back";
    case NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_SLI_TOP_0:
        return "sli_top";
    case NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_INVALID:
        return "invalid";
    default:
        return "unknown";
    }
}

const char *location_label(NV_GPU_CLIENT_ILLUM_ZONE_LOCATION l)
{
    switch (l)
    {
    case NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_GPU_TOP_0:
        return "GPU Top";
    case NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_GPU_FRONT_0:
        return "GPU Front";
    case NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_GPU_BACK_0:
        return "GPU Back";
    case NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_SLI_TOP_0:
        return "SLI Top";
    case NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_INVALID:
        return "Invalid";
    default:
        return "Reserved or Unknown";
    }
}

const char *ctrl_mode_key(NV_GPU_CLIENT_ILLUM_CTRL_MODE m)
{
    switch (m)
    {
    case NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL:
        return "manual";
    case NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR:
        return "piecewise_linear";
    case NV_GPU_CLIENT_ILLUM_CTRL_MODE_INVALID:
        return "invalid";
    default:
        return "unknown";
    }
}

const char *cycle_key(NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_TYPE c)
{
    switch (c)
    {
    case NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_HALF_HALT:
        return "half_halt";
    case NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_FULL_HALT:
        return "full_halt";
    case NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_FULL_REPEAT:
        return "full_repeat";
    case NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_INVALID:
        return "invalid";
    default:
        return "unknown";
    }
}

int zone_has_color(NV_GPU_CLIENT_ILLUM_ZONE_TYPE t)
{
    return t == NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGB || t == NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGBW;
}

int zone_has_white(NV_GPU_CLIENT_ILLUM_ZONE_TYPE t)
{
    return t == NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGBW;
}
