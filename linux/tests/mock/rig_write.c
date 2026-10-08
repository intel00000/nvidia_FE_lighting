/* writing the rig back after SetControl, in a canonical form the tests compare. */
#include "mock.h"

#include <stdio.h>

static void write_named(FILE *f, const mock_name_t *names, NvU32 value)
{
    const char *name = name_of(names, value);
    if (name)
        fputs(name, f);
    else
        fprintf(f, "%u", value);
}

static void write_flags(FILE *f, const char *key, const mock_name_t *names, unsigned value)
{
    if (!value)
        return;
    fprintf(f, " %s=", key);
    const char *sep = "";
    for (; names->name; names++)
        if (value & names->value)
        {
            fprintf(f, "%s%s", sep, names->name);
            sep = ",";
        }
}

static void write_control(FILE *f, const char *prefix, const mock_control_t *c)
{
    if (c->mode != NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL)
    {
        fprintf(f, " %smode=", prefix);
        write_named(f, mode_names, c->mode);
    }
    if (c->r || c->g || c->b || c->w)
        fprintf(f, " %srgbw=%u,%u,%u,%u", prefix, c->r, c->g, c->b, c->w);
    if (c->brightness)
        fprintf(f, " %sbr=%u", prefix, c->brightness);
    for (int j = 0; j < 2; j++)
    {
        const NvU8 *e = c->ep[j];
        if (e[0] || e[1] || e[2] || e[3] || e[4])
            fprintf(f, " %sep%d=%u,%u,%u,%u,%u", prefix, j, e[0], e[1], e[2], e[3], e[4]);
    }
    if (c->cycle || c->group_count || c->rise_ms || c->fall_ms || c->a_ms || c->b_ms || c->idle_ms || c->phase_ms)
    {
        fprintf(f, " %spw=", prefix);
        write_named(f, cycle_names, c->cycle);
        fprintf(f, ",%u,%u,%u,%u,%u,%u,%u", c->group_count, c->rise_ms, c->fall_ms, c->a_ms, c->b_ms, c->idle_ms, c->phase_ms);
    }
}

static void write_zone(FILE *f, const mock_zone_t *z)
{
    fputs("zone type=", f);
    write_named(f, zone_type_names, z->type);
    if (z->location != NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_GPU_TOP_0)
    {
        fputs(" loc=", f);
        write_named(f, location_names, z->location);
    }
    if (z->device)
        fprintf(f, " dev=%u", z->device);
    if (z->provider)
        fprintf(f, " prov=%u", z->provider);
    write_control(f, "", &z->active);
    write_control(f, "d", &z->stored);
    write_flags(f, "fault", zone_fault_names, z->faults);
    fputc('\n', f);
}

static void write_name(FILE *f, const char *name)
{
    for (const unsigned char *s = (const unsigned char *)name; *s; s++)
    {
        if (*s == '\\')
            fputs("\\\\", f);
        else if (*s < 0x20 || *s == 0x7f)
            fprintf(f, "\\x%02x", *s);
        else
            fputc(*s, f);
    }
}

static void write_card(FILE *f, const mock_card_t *c)
{
    fputs("card uuid=", f);
    if (c->uuid_mode == UUID_REPORTED)
    {
        const NvU8 *u = c->uuid;
        fprintf(f, "GPU-%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", u[0], u[1], u[2], u[3], u[4], u[5], u[6], u[7], u[8], u[9],
                u[10], u[11], u[12], u[13], u[14], u[15]);
    }
    else
        fputs(c->uuid_mode == UUID_ZERO ? "zero" : "unsupported", f);
    if (c->has_pci)
        fprintf(f, " pci=%u,0x%08x,0x%08x,0x%x,0x%x", c->bus_id, c->device_id, c->subsystem_id, c->revision_id, c->ext_device_id);
    if (c->has_info)
        fprintf(f, " info=%u,%u,%u", c->rt_cores, c->tensor_cores, c->external);
    write_flags(f, "fail", card_fail_names, c->fails);
    if (c->mode_mask != 1u << NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL)
        fprintf(f, " mask=0x%x", c->mode_mask);
    if (c->info_zones >= 0)
        fprintf(f, " info_zones=%d", c->info_zones);
    if (c->control_zones >= 0)
        fprintf(f, " control_zones=%d", c->control_zones);
    fputs(" name=", f);
    write_name(f, c->name);
    fputc('\n', f);
    for (int i = 0; i < c->nzones; i++)
        write_zone(f, &c->zones[i]);
}

static void write_options(FILE *f, const mock_rig_t *rig)
{
    if (!rig->nunresolved && !rig->initialize_failures && !rig->enumerate_fails)
        return;
    fputs("option", f);
    for (int i = 0; i < rig->nunresolved; i++)
        fprintf(f, "%s%s", i ? "," : " unresolved=", rig->unresolved[i]);
    if (rig->initialize_failures == MOCK_ALWAYS)
        fputs(" initialize_failures=always", f);
    else if (rig->initialize_failures)
        fprintf(f, " initialize_failures=%d", rig->initialize_failures);
    if (rig->enumerate_fails)
        fputs(" fail=enumerate", f);
    fputc('\n', f);
}

int rig_save(const char *path, const mock_rig_t *rig)
{
    char tmp[4096];
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE *f = fopen(tmp, "w");
    if (!f)
        return -1;
    write_options(f, rig);
    if (rig->norder >= 0)
    {
        fputs("order", f);
        for (int i = 0; i < rig->norder; i++)
            fprintf(f, " %d", rig->order[i]);
        fputc('\n', f);
    }
    for (int i = 0; i < rig->ncards; i++)
        write_card(f, &rig->cards[i]);
    if (fclose(f) != 0 || rename(tmp, path) != 0)
    {
        remove(tmp);
        return -1;
    }
    return 0;
}
