/* mock.h - a fake libnvidia-api.so.1 for the tests. */
#ifndef MOCK_H
#define MOCK_H

#include "nvapi_linux_compat.h"

#include "nvapi.h"

#include <stddef.h>

#define MOCK_MAX_CARDS NVAPI_MAX_PHYSICAL_GPUS
#define MOCK_MAX_ZONES NV_GPU_CLIENT_ILLUM_ZONE_NUM_ZONES_MAX
#define MOCK_MAX_UNRESOLVED 16
#define MOCK_ALWAYS (-1)

enum
{
    CARD_FAIL_NAME = 1 << 0,    /* NvAPI_GPU_GetFullName */
    CARD_FAIL_INFO = 1 << 1,    /* NvAPI_GPU_ClientIllumZonesGetInfo: a card without lighting */
    CARD_FAIL_CONTROL = 1 << 2, /* NvAPI_GPU_ClientIllumZonesGetControl */
    CARD_FAIL_DEVICES = 1 << 3, /* NvAPI_GPU_ClientIllumDevicesGetInfo */
};

enum
{
    ZONE_FAULT_SET = 1 << 0,   /* SetControl fails when it would change this zone */
    ZONE_FAULT_STUCK = 1 << 1, /* SetControl succeeds but the zone keeps its old values */
};

typedef enum
{
    UUID_UNSUPPORTED, /* GetUUID returns NVAPI_NOT_SUPPORTED */
    UUID_ZERO,        /* GetUUID succeeds with an all-zero UUID */
    UUID_REPORTED,
} uuid_mode_t;

typedef struct
{
    NvU32 mode;
    NvU8 r, g, b, w, brightness;
    NvU8 ep[2][5]; /* r, g, b, w, brightness of each piecewise endpoint */
    NvU32 cycle;
    NvU8 group_count;
    NvU16 rise_ms, fall_ms, a_ms, b_ms, idle_ms, phase_ms;
} mock_control_t;

typedef struct
{
    NvU32 type; /* reserved values are allowed */
    NvU32 location;
    NvU8 device, provider;
    mock_control_t active, stored; /* stored: the card's defaults, read and written with bDefault */
    unsigned faults;
} mock_zone_t;

typedef struct
{
    char name[NVAPI_SHORT_STRING_MAX];
    unsigned fails;
    uuid_mode_t uuid_mode;
    NvU8 uuid[NVAPI_UUID_LEN];
    int has_pci;
    NvU32 bus_id, device_id, subsystem_id, revision_id, ext_device_id;
    int has_info;
    NvU32 rt_cores, tensor_cores, external;
    NvU32 mode_mask;
    int info_zones, control_zones; /* how many zones GetInfo and GetControl report, or -1 for all */
    int nzones;
    mock_zone_t zones[MOCK_MAX_ZONES];
} mock_card_t;

typedef struct
{
    int nunresolved;
    char unresolved[MOCK_MAX_UNRESOLVED][64]; /* functions nvapi_QueryInterface does not resolve */
    int initialize_failures;                  /* calls to NvAPI_Initialize that fail first, or MOCK_ALWAYS */
    int enumerate_fails;
    int norder;
    int order[MOCK_MAX_CARDS];
    int ncards;
    mock_card_t cards[MOCK_MAX_CARDS];
} mock_rig_t;

typedef struct
{
    const char *name;
    NvU32 value;
} mock_name_t;

extern const mock_name_t zone_type_names[], location_names[], mode_names[], cycle_names[];
extern const mock_name_t card_fail_names[], zone_fault_names[];

const char *name_of(const mock_name_t *names, NvU32 value);
int value_of(const mock_name_t *names, const char *name, NvU32 *value);

int rig_load(const char *path, mock_rig_t *rig, char *err, size_t errlen);
int rig_save(const char *path, const mock_rig_t *rig);

void control_to_nv(NvU32 type, const mock_control_t *c, NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_V1 *out);
void control_from_nv(NvU32 type, const NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_V1 *in, mock_control_t *c);
int control_equal(const mock_control_t *a, const mock_control_t *b);

void *nvapi_QueryInterface(unsigned int id);
extern const int felight_mock_nvapi;

#endif
