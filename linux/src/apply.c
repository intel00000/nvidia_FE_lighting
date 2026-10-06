/* apply.c - applying a profile to a GPU. */
#include "apply.h"
#include "device.h"

#include <stdio.h>
#include <string.h>

/* Apply a profile. verify_gpu: check the profile's GPU identity (its UUID, or with drivers
 * that report none its PCI IDs) against the GPU. */
int apply_profile(const profile_t *p, long gpu_override, int verify_gpu, void (*say)(const char *, ...))
{
    static zone_t zones[NV_GPU_CLIENT_ILLUM_ZONE_NUM_ZONES_MAX];
    int rc = nv_open(&g_nv);
    if (rc)
        return rc;
    long gpu = gpu_override >= 0 ? gpu_override : (p->has_gpu_index ? (long)p->gpu_index : 0);
    gpu_t g;
    memset(&g, 0, sizeof g);
    if (verify_gpu && p->bad_identity)
    {
        say("profile has an unreadable GPU identity; nothing applied. Re-save it or use --no-verify-gpu.\n");
        return EXIT_GPU_MISMATCH;
    }
    long by_uuid = -1;
    if (verify_gpu && p->has_gpu_uuid && gpu_override < 0)
    {
        /* The UUID belongs to the card itself: it follows the card to another slot or index, and
         * another card, even of the same model, never matches. */
        int reported = 0;
        for (NvU32 i = 0; i < g_nv.gpu_count && by_uuid < 0; i++)
        {
            gpu_t cand;
            if (read_gpu(&g_nv, i, &cand) || !cand.has_uuid)
                continue;
            reported = 1;
            if (strcmp(cand.uuid, p->gpu_uuid) == 0)
            {
                by_uuid = (long)i;
                g = cand;
            }
        }
        if (reported && by_uuid < 0)
        {
            say("GPU mismatch: no GPU has the saved UUID %s; nothing applied. Save the settings again on the current GPU.\n", p->gpu_uuid);
            return EXIT_GPU_MISMATCH;
        }
    }
    if (by_uuid >= 0)
    {
        if (by_uuid != gpu)
            say("GPU moved from index %ld to %ld; using index %ld.\n", gpu, by_uuid, by_uuid);
        gpu = by_uuid;
        say("GPU %ld: %s\n", gpu, g.name);
        say("GPU identity verified (UUID %s).\n", g.uuid);
    }
    else if (verify_gpu && p->has_gpu_ids && gpu_override < 0)
    {
        /* Prefer the saved index, but if the card moved (another GPU added or removed, or a
         * different slot) locate it by its PCI identity instead of giving up. */
        long found = -1;
        int matches = 0;
        for (NvU32 i = 0; i < g_nv.gpu_count; i++)
        {
            gpu_t cand;
            if (read_gpu(&g_nv, i, &cand) || !cand.has_ids)
                continue;
            if (cand.bus_id == p->bus_id && cand.device_id == p->device_id && cand.subsystem_id == p->subsystem_id)
            {
                matches++;
                if (found < 0 || (long)i == gpu)
                {
                    found = (long)i;
                    g = cand;
                }
            }
        }
        if (matches == 0)
        {
            say("GPU mismatch: no GPU has the saved identity (bus %u, device 0x%08x, subsystem 0x%08x); nothing applied. Save the settings again on the current GPU.\n",
                p->bus_id, p->device_id, p->subsystem_id);
            return EXIT_GPU_MISMATCH;
        }
        if (matches > 1 && found != gpu)
        {
            say("GPU mismatch: %d GPUs share the saved identity and none is at index %ld; nothing applied.\n", matches, gpu);
            return EXIT_GPU_MISMATCH;
        }
        if (found != gpu)
            say("GPU moved from index %ld to %ld; using index %ld.\n", gpu, found, found);
        gpu = found;
        say("GPU %ld: %s\n", gpu, g.name);
        say("GPU identity verified (bus %u, device 0x%08x, subsystem 0x%08x).\n", g.bus_id, g.device_id, g.subsystem_id);
    }
    else
    {
        if (!check_gpu_index(&g_nv, gpu))
        {
            say("GPU index %ld out of range (%u GPU(s) detected); nothing applied.\n", gpu, g_nv.gpu_count);
            return EXIT_GPU_MISMATCH;
        }
        if ((rc = read_gpu(&g_nv, (NvU32)gpu, &g)))
            return rc;
        say("GPU %ld: %s\n", gpu, g.name);
        if (verify_gpu && p->has_gpu_uuid && g.has_uuid)
        {
            /* --gpu N was given explicitly: that GPU must be the saved card. */
            if (strcmp(g.uuid, p->gpu_uuid) != 0)
            {
                say("GPU mismatch: GPU %ld is not the card the profile was saved for (UUID %s); nothing applied. Use --no-verify-gpu to apply anyway.\n",
                    gpu, p->gpu_uuid);
                return EXIT_GPU_MISMATCH;
            }
            say("GPU identity verified (UUID %s).\n", g.uuid);
        }
        else if (verify_gpu && p->has_gpu_ids)
        {
            /* --gpu N was given explicitly: that GPU must still carry the saved identity. */
            if (!g.has_ids || g.bus_id != p->bus_id || g.device_id != p->device_id || g.subsystem_id != p->subsystem_id)
            {
                say("GPU mismatch: GPU %ld is not the card the profile was saved for (bus %u, device 0x%08x, subsystem 0x%08x); nothing applied. Use --no-verify-gpu to apply anyway.\n",
                    gpu, p->bus_id, p->device_id, p->subsystem_id);
                return EXIT_GPU_MISMATCH;
            }
            say("GPU identity verified (bus %u, device 0x%08x, subsystem 0x%08x).\n", g.bus_id, g.device_id, g.subsystem_id);
        }
        else if (verify_gpu)
        {
            say("profile has no GPU identity; verification skipped.\n");
        }
    }
    NvU32 n = 0;
    if ((rc = read_zones(&g_nv, (NvU32)gpu, 0, zones, &n)))
        return rc;
    say("%u zone(s) detected, %d in profile.\n", n, p->nzones);
    int applied = 0, failed = 0, last_error = 0;
    for (int i = 0; i < p->nzones; i++)
    {
        const profile_zone_t *pz = &p->zones[i];
        if (pz->index >= n || !zones[pz->index].present_in_control)
        {
            say("zone %u: not present on this GPU, skipped.\n", pz->index);
            failed++;
            continue;
        }
        if (zones[pz->index].type != pz->type)
        {
            say("zone %u: profile says %s but the GPU reports %s, skipped.\n", pz->index, zone_type_key(pz->type), zone_type_key(zones[pz->index].type));
            failed++;
            continue;
        }
        set_opts_t o = {0};
        o.verify = 1;
        o.has_rgb = pz->has_rgb && zone_has_color(pz->type);
        o.r = pz->r;
        o.g = pz->g;
        o.b = pz->b;
        o.has_white = pz->has_white && zone_has_white(pz->type);
        o.w = pz->w;
        o.has_brightness = pz->has_brightness;
        o.brightness = pz->brightness;
        int zrc = set_zone(&g_nv, (NvU32)gpu, pz->index, &o, 1);
        if (zrc == 0)
        {
            applied++;
            char what[96] = "";
            size_t len = 0;
            if (o.has_rgb)
                len += (size_t)snprintf(what + len, sizeof what - len, " rgb=%d,%d,%d", pz->r, pz->g, pz->b);
            if (o.has_white)
                len += (size_t)snprintf(what + len, sizeof what - len, " white=%d", pz->w);
            if (o.has_brightness)
                len += (size_t)snprintf(what + len, sizeof what - len, " brightness=%d%%", pz->brightness);
            say("zone %u (%s):%s applied.\n", pz->index, zone_type_key(pz->type), what[0] ? what : " nothing to set,");
        }
        else
        {
            failed++;
            last_error = zrc;
            say("zone %u: apply failed (exit code %d).\n", pz->index, zrc);
        }
    }
    say("applied %d/%d zone(s).\n", applied, p->nzones);
    if (applied == 0)
        return last_error ? last_error : EXIT_GPU_MISMATCH;
    if (failed)
        return EXIT_PARTIAL;
    return 0;
}
