/* output.c - text and JSON output of GPUs and zones. */
#include "output.h"

#include <stdio.h>

void json_string(FILE *f, const char *s)
{
    fputc('"', f);
    for (; *s; s++)
    {
        unsigned char ch = (unsigned char)*s;
        // escape double quotes and backslashes
        if (ch == '"' || ch == '\\')
        {
            fputc('\\', f);
            fputc(ch, f);
        }
        // control characters (0x00-0x1F)
        else if (ch < 0x20 || ch > 0x7E)
            fprintf(f, "\\u%04x", ch);
        // printable ASCII characters (0x20-0x7E)
        else
            fputc(ch, f);
    }
    fputc('"', f);
}

static void print_zone_json(FILE *f, const zone_t *z)
{
    fprintf(f, "{\"index\":%u,\"type\":\"%s\",\"type_label\":\"%s\",\"location\":\"%s\",\"location_label\":\"%s\","
               "\"present_in_info\":%s,\"present_in_control\":%s,\"device_index\":",
            z->index, zone_type_key(z->type), zone_type_label(z->type), location_key(z->location), location_label(z->location),
            z->present_in_info ? "true" : "false", z->present_in_control ? "true" : "false");

    if (z->present_in_info)
        fprintf(f, "%u", z->device_index);
    else
        fprintf(f, "null");

    fprintf(f, ",\"ctrl_mode\":\"%s\",\"ctrl_mode_mask\":", z->present_in_control ? ctrl_mode_key(z->ctrl_mode) : "unknown");

    if (z->has_mode_mask)
        fprintf(f, "%u", z->ctrl_mode_mask);
    else
        fprintf(f, "null");

    fprintf(f, ",\"has_color\":%s,\"has_white\":%s", zone_has_color(z->type) ? "true" : "false", zone_has_white(z->type) ? "true" : "false");

    if (!z->present_in_control)
        fprintf(f, ",\"r\":0,\"g\":0,\"b\":0,\"w\":0,\"brightness\":0");
    else if (!z->piecewise)
        fprintf(f, ",\"r\":%u,\"g\":%u,\"b\":%u,\"w\":%u,\"brightness\":%u", z->r, z->g, z->b, z->w, z->brightness);
    else
    {
        fprintf(f, ",\"r\":%u,\"g\":%u,\"b\":%u,\"w\":%u,\"brightness\":%u", z->ep_r[0], z->ep_g[0], z->ep_b[0], z->ep_w[0], z->ep_brightness[0]);
        fprintf(f, ",\"piecewise\":{\"cycle\":\"%s\",\"group_count\":%u,\"rise_ms\":%u,\"fall_ms\":%u,\"a_ms\":%u,\"b_ms\":%u,\"idle_ms\":%u,\"phase_offset_ms\":%u,\"endpoints\":[",
                cycle_key(z->pw.cycleType), z->pw.grpCount, z->pw.riseTimems, z->pw.fallTimems, z->pw.ATimems, z->pw.BTimems, z->pw.grpIdleTimems, z->pw.phaseOffsetms);
        for (int j = 0; j < 2; j++)
            fprintf(f, "%s{\"r\":%u,\"g\":%u,\"b\":%u,\"w\":%u,\"brightness\":%u}", j ? "," : "", z->ep_r[j], z->ep_g[j], z->ep_b[j], z->ep_w[j], z->ep_brightness[j]);
        fprintf(f, "]}");
    }
    fprintf(f, "}");
}

void print_gpu_json(FILE *f, const gpu_t *g, const zone_t *zones, NvU32 nzones, int error)
{
    fprintf(f, "{\"index\":%u,\"name\":", g->index);

    json_string(f, g->name);

    if (error)
        fprintf(f, ",\"error\":\"illumination zones unavailable (exit code %d)\"", error);
    if (g->has_ids)
        fprintf(f, ",\"bus_id\":%u,\"device_id\":\"0x%08x\",\"subsystem_id\":\"0x%08x\",\"revision_id\":\"0x%x\",\"ext_device_id\":\"0x%08x\"",
                g->bus_id, g->device_id, g->subsystem_id, g->revision_id, g->ext_device_id);
    if (g->has_uuid)
    {
        fprintf(f, ",\"uuid\":");
        json_string(f, g->uuid);
    }
    if (g->has_info)
        fprintf(f, ",\"rt_cores\":%u,\"tensor_cores\":%u,\"external\":%s", g->rt_cores, g->tensor_cores, g->external ? "true" : "false");

    fprintf(f, ",\"zones\":[");

    for (NvU32 i = 0; i < nzones; i++)
    {
        if (i)
            fputc(',', f);
        print_zone_json(f, &zones[i]);
    }
    fprintf(f, "]}");
}

void print_gpu_text(FILE *f, const gpu_t *g, const zone_t *zones, NvU32 nzones)
{
    fprintf(f, "GPU %u: %s", g->index, g->name);

    if (g->has_ids)
        fprintf(f, "  (bus %u, device 0x%08x, subsystem 0x%08x)", g->bus_id, g->device_id, g->subsystem_id);
    if (g->has_uuid)
        fprintf(f, "  %s", g->uuid);

    fputc('\n', f);

    if (nzones == 0)
        fprintf(f, "  no illumination zones\n");

    for (NvU32 i = 0; i < nzones; i++)
    {
        const zone_t *z = &zones[i];
        fprintf(f, "  zone %u: %-12s @ %-9s mode=%-16s", z->index, zone_type_label(z->type), location_label(z->location),
                z->present_in_control ? ctrl_mode_key(z->ctrl_mode) : "unknown");
        if (!z->present_in_control)
        {
            fputc('\n', f);
            continue;
        }
        if (z->piecewise)
        {
            fprintf(f, " cycle=%s endpoints=[", cycle_key(z->pw.cycleType));
            for (int j = 0; j < 2; j++)
            {
                if (zone_has_color(z->type))
                    fprintf(f, "%s(%u,%u,%u", j ? " " : "", z->ep_r[j], z->ep_g[j], z->ep_b[j]);
                else
                    fprintf(f, "%s(", j ? " " : "");
                if (zone_has_white(z->type))
                    fprintf(f, ",w=%u", z->ep_w[j]);
                fprintf(f, "%s%u%%)", zone_has_color(z->type) ? " " : "", z->ep_brightness[j]);
            }
            fprintf(f, "]\n");
        }
        else
        {
            if (zone_has_color(z->type))
                fprintf(f, " rgb=%u,%u,%u", z->r, z->g, z->b);
            if (zone_has_white(z->type))
                fprintf(f, " white=%u", z->w);
            fprintf(f, " brightness=%u%%\n", z->brightness);
        }
    }
}
