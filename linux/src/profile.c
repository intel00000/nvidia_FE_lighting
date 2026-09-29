/* profile.c - writing and reading profile files. */
#include "profile.h"
#include "diag.h"
#include "parse.h"

#include <ctype.h>
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

/* "GPU-" and 8-4-4-4-12 hex digits, stored lowercase as nvidia-smi prints it. */
static int parse_uuid(const char *s, char *out)
{
    static const char pattern[] = "GPU-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx";
    if (strlen(s) != sizeof pattern - 1 || strncmp(s, "GPU-", 4) != 0)
        return 0;
    for (size_t i = 4; pattern[i]; i++)
    {
        if (pattern[i] == '-' ? s[i] != '-' : !isxdigit((unsigned char)s[i]))
            return 0;
        out[i] = (char)tolower((unsigned char)s[i]);
    }
    memcpy(out, "GPU-", 4);
    out[sizeof pattern - 1] = 0;
    return 1;
}

int profile_load(const char *path, profile_t *p)
{
    memset(p, 0, sizeof *p);
    p->delay_seconds = 10;
    FILE *f = fopen(path, "r");
    if (!f)
    {
        log_err("felight: cannot open %s: %s\n", path, strerror(errno));
        return EXIT_FILE;
    }
    char line[512];
    int lineno = 0;
    while (fgets(line, sizeof line, f))
    {
        lineno++;
        if (!strchr(line, '\n') && !feof(f))
        {
            /* Longer than the buffer: skip the rest of the physical line instead of parsing it as new lines. */
            int ch;
            while ((ch = fgetc(f)) != EOF && ch != '\n')
            {
            }
            log_err("felight: %s:%d: line too long, ignored\n", path, lineno);
            continue;
        }
        char *hash = strchr(line, '#');
        if (hash)
            *hash = 0;
        char *s = trim(line);
        if (!*s)
            continue;
        if (strncmp(s, "zone", 4) == 0 && isspace((unsigned char)s[4]))
        {
            if (p->nzones >= MAX_PROFILE_ZONES)
            {
                log_err("felight: %s:%d: too many zones\n", path, lineno);
                continue;
            }
            profile_zone_t *z = &p->zones[p->nzones];
            memset(z, 0, sizeof *z);
            char *save = NULL;
            char *tok = strtok_r(s + 4, " \t", &save);
            if (!tok || !parse_u32(tok, &z->index))
            {
                log_err("felight: %s:%d: bad zone index\n", path, lineno);
                continue;
            }
            tok = strtok_r(NULL, " \t", &save);
            if (!tok || !zone_type_from_key(tok, &z->type))
            {
                log_err("felight: %s:%d: bad zone type '%s'\n", path, lineno, tok ? tok : "");
                continue;
            }
            int bad = 0;
            while ((tok = strtok_r(NULL, " \t", &save)))
            {
                char *eq = strchr(tok, '=');
                if (!eq)
                {
                    bad = 1;
                    break;
                }
                *eq = 0;
                NvU32 v;
                if (!parse_u32(eq + 1, &v))
                {
                    bad = 1;
                    break;
                }
                int is_brightness = strcmp(tok, "brightness") == 0;
                if (v > (is_brightness ? 100u : 255u))
                {
                    bad = 1;
                    break;
                }
                if (strcmp(tok, "r") == 0)
                {
                    z->r = (int)v;
                    z->has_rgb = 1;
                }
                else if (strcmp(tok, "g") == 0)
                {
                    z->g = (int)v;
                    z->has_rgb = 1;
                }
                else if (strcmp(tok, "b") == 0)
                {
                    z->b = (int)v;
                    z->has_rgb = 1;
                }
                else if (strcmp(tok, "w") == 0)
                {
                    z->w = (int)v;
                    z->has_white = 1;
                }
                else if (is_brightness)
                {
                    z->brightness = (int)v;
                    z->has_brightness = 1;
                }
                else
                {
                    bad = 1;
                    break;
                }
            }
            if (bad)
            {
                log_err("felight: %s:%d: bad zone parameter (r/g/b/w are 0-255, brightness is 0-100)\n", path, lineno);
                continue;
            }
            p->nzones++;
            continue;
        }
        char *eq = strchr(s, '=');
        if (!eq)
        {
            log_err("felight: %s:%d: expected key=value\n", path, lineno);
            continue;
        }
        *eq = 0;
        char *key = trim(s), *val = trim(eq + 1);
        NvU32 v;
        if (strcmp(key, "delay_seconds") == 0)
        {
            if (parse_u32(val, &v) && v <= MAX_DELAY_SECONDS)
                p->delay_seconds = (int)v;
            else
                log_err("felight: %s:%d: bad delay_seconds, using %d\n", path, lineno, p->delay_seconds);
        }
        else if (strcmp(key, "gpu_index") == 0)
        {
            if (parse_u32(val, &v) && v <= MAX_GPU_INDEX)
            {
                p->gpu_index = v;
                p->has_gpu_index = 1;
            }
            else
            {
                p->bad_identity = 1;
                log_err("felight: %s:%d: bad gpu_index\n", path, lineno);
            }
        }
        else if (strcmp(key, "gpu_bus_id") == 0)
        {
            if (parse_u32(val, &v))
            {
                p->bus_id = v;
                p->has_gpu_ids |= 1;
            }
            else
            {
                p->bad_identity = 1;
                log_err("felight: %s:%d: bad gpu_bus_id\n", path, lineno);
            }
        }
        /* The PCI IDs are written as 0x%08x, so accept hex (and plain decimal) here. */
        else if (strcmp(key, "gpu_device_id") == 0)
        {
            if (parse_u32_base(val, 0, &v))
            {
                p->device_id = v;
                p->has_gpu_ids |= 2;
            }
            else
            {
                p->bad_identity = 1;
                log_err("felight: %s:%d: bad gpu_device_id\n", path, lineno);
            }
        }
        else if (strcmp(key, "gpu_subsystem_id") == 0)
        {
            if (parse_u32_base(val, 0, &v))
            {
                p->subsystem_id = v;
                p->has_gpu_ids |= 4;
            }
            else
            {
                p->bad_identity = 1;
                log_err("felight: %s:%d: bad gpu_subsystem_id\n", path, lineno);
            }
        }
        else if (strcmp(key, "gpu_uuid") == 0)
        {
            if (parse_uuid(val, p->gpu_uuid))
                p->has_gpu_uuid = 1;
            else
            {
                p->bad_identity = 1;
                log_err("felight: %s:%d: bad gpu_uuid\n", path, lineno);
            }
        }
        else if (strcmp(key, "gpu_name") == 0)
        {
            snprintf(p->gpu_name, sizeof p->gpu_name, "%s", val);
        }
        else
            log_err("felight: %s:%d: unknown key '%s' ignored\n", path, lineno, key);
    }
    fclose(f);
    p->has_gpu_ids = (p->has_gpu_ids == 7);
    return 0;
}
