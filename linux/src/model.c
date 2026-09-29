/* model.c - NvAPI illumination enum values mapping and zone control data decoding. */
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

/* Read the manual parameters (and piecewise data, if any) of one zone into the model. */
void zone_read_control(zone_t *z, const NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_V1 *c)
{
    z->present_in_control = 1;
    z->ctrl_mode = c->ctrlMode;
    z->piecewise = (c->ctrlMode == NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR);
    if (!z->present_in_info)
        z->type = c->type;
    const NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR *pw = NULL;
    switch (c->type)
    {
    case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGB:
        if (!z->piecewise)
        {
            const NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGB_PARAMS *p = &c->data.rgb.data.manualRGB.rgbParams;
            z->r = p->colorR;
            z->g = p->colorG;
            z->b = p->colorB;
            z->brightness = p->brightnessPct;
        }
        else
        {
            for (int j = 0; j < 2; j++)
            {
                const NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGB_PARAMS *p = &c->data.rgb.data.piecewiseLinearRGB.rgbParams[j];
                z->ep_r[j] = p->colorR;
                z->ep_g[j] = p->colorG;
                z->ep_b[j] = p->colorB;
                z->ep_brightness[j] = p->brightnessPct;
            }
            pw = &c->data.rgb.data.piecewiseLinearRGB.piecewiseLinearData;
        }
        break;
    case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGBW:
        if (!z->piecewise)
        {
            const NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGBW_PARAMS *p = &c->data.rgbw.data.manualRGBW.rgbwParams;
            z->r = p->colorR;
            z->g = p->colorG;
            z->b = p->colorB;
            z->w = p->colorW;
            z->brightness = p->brightnessPct;
        }
        else
        {
            for (int j = 0; j < 2; j++)
            {
                const NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGBW_PARAMS *p = &c->data.rgbw.data.piecewiseLinearRGBW.rgbwParams[j];
                z->ep_r[j] = p->colorR;
                z->ep_g[j] = p->colorG;
                z->ep_b[j] = p->colorB;
                z->ep_w[j] = p->colorW;
                z->ep_brightness[j] = p->brightnessPct;
            }
            pw = &c->data.rgbw.data.piecewiseLinearRGBW.piecewiseLinearData;
        }
        break;
    case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_COLOR_FIXED:
        if (!z->piecewise)
        {
            z->brightness = c->data.colorFixed.data.manualColorFixed.colorFixedParams.brightnessPct;
        }
        else
        {
            for (int j = 0; j < 2; j++)
                z->ep_brightness[j] = c->data.colorFixed.data.piecewiseLinearColorFixed.colorFixedParams[j].brightnessPct;
            pw = &c->data.colorFixed.data.piecewiseLinearColorFixed.piecewiseLinearData;
        }
        break;
    case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_SINGLE_COLOR:
        if (!z->piecewise)
        {
            z->brightness = c->data.singleColor.data.manualSingleColor.singleColorParams.brightnessPct;
        }
        else
        {
            for (int j = 0; j < 2; j++)
                z->ep_brightness[j] = c->data.singleColor.data.piecewiseLinearSingleColor.singleColorParams[j].brightnessPct;
            pw = &c->data.singleColor.data.piecewiseLinearSingleColor.piecewiseLinearData;
        }
        break;
    default:
        break;
    }
    if (pw)
        z->pw = *pw;
}
