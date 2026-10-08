/* names a rig file uses for NvAPI values and for faults. */
#include "mock.h"

#include <string.h>

const mock_name_t zone_type_names[] = {
    {"invalid", NV_GPU_CLIENT_ILLUM_ZONE_TYPE_INVALID},
    {"rgb", NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGB},
    {"color_fixed", NV_GPU_CLIENT_ILLUM_ZONE_TYPE_COLOR_FIXED},
    {"rgbw", NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGBW},
    {"single_color", NV_GPU_CLIENT_ILLUM_ZONE_TYPE_SINGLE_COLOR},
    {NULL, 0},
};

const mock_name_t location_names[] = {
    {"gpu_top", NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_GPU_TOP_0},
    {"gpu_front", NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_GPU_FRONT_0},
    {"gpu_back", NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_GPU_BACK_0},
    {"sli_top", NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_SLI_TOP_0},
    {"invalid", NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_INVALID},
    {NULL, 0},
};

const mock_name_t mode_names[] = {
    {"manual", NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL},
    {"piecewise", NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR},
    {"invalid", NV_GPU_CLIENT_ILLUM_CTRL_MODE_INVALID},
    {NULL, 0},
};

const mock_name_t cycle_names[] = {
    {"half_halt", NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_HALF_HALT},
    {"full_halt", NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_FULL_HALT},
    {"full_repeat", NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_FULL_REPEAT},
    {"invalid", NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_INVALID},
    {NULL, 0},
};

const mock_name_t card_fail_names[] = {
    {"name", CARD_FAIL_NAME},
    {"info", CARD_FAIL_INFO},
    {"control", CARD_FAIL_CONTROL},
    {"devices", CARD_FAIL_DEVICES},
    {NULL, 0},
};

const mock_name_t zone_fault_names[] = {
    {"set", ZONE_FAULT_SET},
    {"stuck", ZONE_FAULT_STUCK},
    {NULL, 0},
};

const char *name_of(const mock_name_t *names, NvU32 value)
{
    for (; names->name; names++)
        if (names->value == value)
            return names->name;
    return NULL;
}

int value_of(const mock_name_t *names, const char *name, NvU32 *value)
{
    for (; names->name; names++)
        if (strcmp(names->name, name) == 0)
        {
            *value = names->value;
            return 1;
        }
    return 0;
}
