/* control.c - converting a zone's state to and from NvAPI's zone control records. */
#include "mock.h"

#include <string.h>

static void piecewise_to_nv(const mock_control_t *c, NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR *pw)
{
    pw->cycleType = (NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_TYPE)c->cycle;
    pw->grpCount = c->group_count;
    pw->riseTimems = c->rise_ms;
    pw->fallTimems = c->fall_ms;
    pw->ATimems = c->a_ms;
    pw->BTimems = c->b_ms;
    pw->grpIdleTimems = c->idle_ms;
    pw->phaseOffsetms = c->phase_ms;
}

static void piecewise_from_nv(const NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR *pw, mock_control_t *c)
{
    c->cycle = pw->cycleType;
    c->group_count = pw->grpCount;
    c->rise_ms = pw->riseTimems;
    c->fall_ms = pw->fallTimems;
    c->a_ms = pw->ATimems;
    c->b_ms = pw->BTimems;
    c->idle_ms = pw->grpIdleTimems;
    c->phase_ms = pw->phaseOffsetms;
}

void control_to_nv(NvU32 type, const mock_control_t *c, NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_V1 *out)
{
    memset(out, 0, sizeof *out);
    out->type = (NV_GPU_CLIENT_ILLUM_ZONE_TYPE)type;
    out->ctrlMode = (NV_GPU_CLIENT_ILLUM_CTRL_MODE)c->mode;
    int pw = c->mode == NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR;
    switch (type)
    {
    case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGB:
        if (pw)
        {
            for (int j = 0; j < 2; j++)
            {
                NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGB_PARAMS *e = &out->data.rgb.data.piecewiseLinearRGB.rgbParams[j];
                e->colorR = c->ep[j][0];
                e->colorG = c->ep[j][1];
                e->colorB = c->ep[j][2];
                e->brightnessPct = c->ep[j][4];
            }
            piecewise_to_nv(c, &out->data.rgb.data.piecewiseLinearRGB.piecewiseLinearData);
        }
        else
        {
            NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGB_PARAMS *m = &out->data.rgb.data.manualRGB.rgbParams;
            m->colorR = c->r;
            m->colorG = c->g;
            m->colorB = c->b;
            m->brightnessPct = c->brightness;
        }
        break;
    case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGBW:
        if (pw)
        {
            for (int j = 0; j < 2; j++)
            {
                NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGBW_PARAMS *e = &out->data.rgbw.data.piecewiseLinearRGBW.rgbwParams[j];
                e->colorR = c->ep[j][0];
                e->colorG = c->ep[j][1];
                e->colorB = c->ep[j][2];
                e->colorW = c->ep[j][3];
                e->brightnessPct = c->ep[j][4];
            }
            piecewise_to_nv(c, &out->data.rgbw.data.piecewiseLinearRGBW.piecewiseLinearData);
        }
        else
        {
            NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGBW_PARAMS *m = &out->data.rgbw.data.manualRGBW.rgbwParams;
            m->colorR = c->r;
            m->colorG = c->g;
            m->colorB = c->b;
            m->colorW = c->w;
            m->brightnessPct = c->brightness;
        }
        break;
    case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_COLOR_FIXED:
        if (pw)
        {
            for (int j = 0; j < 2; j++)
                out->data.colorFixed.data.piecewiseLinearColorFixed.colorFixedParams[j].brightnessPct = c->ep[j][4];
            piecewise_to_nv(c, &out->data.colorFixed.data.piecewiseLinearColorFixed.piecewiseLinearData);
        }
        else
            out->data.colorFixed.data.manualColorFixed.colorFixedParams.brightnessPct = c->brightness;
        break;
    case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_SINGLE_COLOR:
        if (pw)
        {
            for (int j = 0; j < 2; j++)
                out->data.singleColor.data.piecewiseLinearSingleColor.singleColorParams[j].brightnessPct = c->ep[j][4];
            piecewise_to_nv(c, &out->data.singleColor.data.piecewiseLinearSingleColor.piecewiseLinearData);
        }
        else
            out->data.singleColor.data.manualSingleColor.singleColorParams.brightnessPct = c->brightness;
        break;
    default:
        break;
    }
}

/* Takes what in sets for a zone of this type; the other fields of c keep their values. */
void control_from_nv(NvU32 type, const NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_V1 *in, mock_control_t *c)
{
    c->mode = in->ctrlMode;
    int pw = in->ctrlMode == NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR;
    if (!pw && in->ctrlMode != NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL)
        return;
    switch (type)
    {
    case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGB:
        if (pw)
        {
            for (int j = 0; j < 2; j++)
            {
                const NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGB_PARAMS *e = &in->data.rgb.data.piecewiseLinearRGB.rgbParams[j];
                c->ep[j][0] = e->colorR;
                c->ep[j][1] = e->colorG;
                c->ep[j][2] = e->colorB;
                c->ep[j][4] = e->brightnessPct;
            }
            piecewise_from_nv(&in->data.rgb.data.piecewiseLinearRGB.piecewiseLinearData, c);
        }
        else
        {
            const NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGB_PARAMS *m = &in->data.rgb.data.manualRGB.rgbParams;
            c->r = m->colorR;
            c->g = m->colorG;
            c->b = m->colorB;
            c->brightness = m->brightnessPct;
        }
        break;
    case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGBW:
        if (pw)
        {
            for (int j = 0; j < 2; j++)
            {
                const NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGBW_PARAMS *e = &in->data.rgbw.data.piecewiseLinearRGBW.rgbwParams[j];
                c->ep[j][0] = e->colorR;
                c->ep[j][1] = e->colorG;
                c->ep[j][2] = e->colorB;
                c->ep[j][3] = e->colorW;
                c->ep[j][4] = e->brightnessPct;
            }
            piecewise_from_nv(&in->data.rgbw.data.piecewiseLinearRGBW.piecewiseLinearData, c);
        }
        else
        {
            const NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGBW_PARAMS *m = &in->data.rgbw.data.manualRGBW.rgbwParams;
            c->r = m->colorR;
            c->g = m->colorG;
            c->b = m->colorB;
            c->w = m->colorW;
            c->brightness = m->brightnessPct;
        }
        break;
    case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_COLOR_FIXED:
        if (pw)
        {
            for (int j = 0; j < 2; j++)
                c->ep[j][4] = in->data.colorFixed.data.piecewiseLinearColorFixed.colorFixedParams[j].brightnessPct;
            piecewise_from_nv(&in->data.colorFixed.data.piecewiseLinearColorFixed.piecewiseLinearData, c);
        }
        else
            c->brightness = in->data.colorFixed.data.manualColorFixed.colorFixedParams.brightnessPct;
        break;
    case NV_GPU_CLIENT_ILLUM_ZONE_TYPE_SINGLE_COLOR:
        if (pw)
        {
            for (int j = 0; j < 2; j++)
                c->ep[j][4] = in->data.singleColor.data.piecewiseLinearSingleColor.singleColorParams[j].brightnessPct;
            piecewise_from_nv(&in->data.singleColor.data.piecewiseLinearSingleColor.piecewiseLinearData, c);
        }
        else
            c->brightness = in->data.singleColor.data.manualSingleColor.singleColorParams.brightnessPct;
        break;
    default:
        break;
    }
}

int control_equal(const mock_control_t *a, const mock_control_t *b)
{
    return a->mode == b->mode && a->r == b->r && a->g == b->g && a->b == b->b && a->w == b->w && a->brightness == b->brightness &&
           memcmp(a->ep, b->ep, sizeof a->ep) == 0 && a->cycle == b->cycle && a->group_count == b->group_count && a->rise_ms == b->rise_ms &&
           a->fall_ms == b->fall_ms && a->a_ms == b->a_ms && a->b_ms == b->b_ms && a->idle_ms == b->idle_ms && a->phase_ms == b->phase_ms;
}
