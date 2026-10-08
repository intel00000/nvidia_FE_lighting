/* read the rig file that describes the mocked cards, zones and faults. */
#include "mock.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    const char *path;
    int lineno;
    char *err;
    size_t errlen;
} reader_t;

__attribute__((format(printf, 2, 3))) static int fail(reader_t *r, const char *fmt, ...)
{
    int n = snprintf(r->err, r->errlen, "%s:%d: ", r->path, r->lineno);
    if (n >= 0 && (size_t)n < r->errlen)
    {
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(r->err + n, r->errlen - (size_t)n, fmt, ap);
        va_end(ap);
    }
    return -1;
}

static int number(const char *s, NvU32 max, NvU32 *out)
{
    char *end = NULL;
    errno = 0;
    unsigned long v = strtoul(s, &end, 0);
    if (!*s || *s == '-' || *s == '+' || *end || errno || v > max)
        return 0;
    *out = (NvU32)v;
    return 1;
}

static int named(const mock_name_t *names, const char *s, NvU32 *out)
{
    return value_of(names, s, out) || number(s, 0xFFFFFFFFu, out);
}

/* Exactly n comma-separated numbers. */
static int numbers(const char *s, NvU32 *out, int n, NvU32 max)
{
    for (int i = 0; i < n; i++)
    {
        const char *end = strchr(s, ',');
        size_t len = end ? (size_t)(end - s) : strlen(s);
        char tok[32];
        if ((end == NULL) != (i == n - 1) || len >= sizeof tok)
            return 0;
        memcpy(tok, s, len);
        tok[len] = 0;
        if (!number(tok, max, &out[i]))
            return 0;
        s = end ? end + 1 : s + len;
    }
    return 1;
}

static int flags(const mock_name_t *names, const char *s, unsigned *out)
{
    char buf[128];
    snprintf(buf, sizeof buf, "%s", s);
    char *save = NULL;
    for (char *tok = strtok_r(buf, ",", &save); tok; tok = strtok_r(NULL, ",", &save))
    {
        NvU32 v;
        if (!value_of(names, tok, &v))
            return 0;
        *out |= v;
    }
    return 1;
}

static int hex_digit(char c)
{
    return isdigit((unsigned char)c) ? c - '0' : tolower((unsigned char)c) - 'a' + 10;
}

static int parse_uuid(const char *s, NvU8 *out)
{
    static const char pattern[] = "GPU-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx";
    if (strlen(s) != sizeof pattern - 1 || strncmp(s, "GPU-", 4) != 0)
        return 0;
    int n = 0;
    for (size_t i = 4; pattern[i]; i++)
    {
        if (pattern[i] == '-')
        {
            if (s[i] != '-')
                return 0;
            continue;
        }
        if (!isxdigit((unsigned char)s[i]) || !isxdigit((unsigned char)s[i + 1]))
            return 0;
        out[n++] = (NvU8)(hex_digit(s[i]) * 16 + hex_digit(s[i + 1]));
        i++;
    }
    return 1;
}

/* A name runs to the end of the line; \\ and \xNN are its only escapes. */
static int parse_name(const char *s, char *out, size_t len)
{
    size_t n = 0;
    for (; *s; s++)
    {
        int c = (unsigned char)*s;
        if (c == '\\')
        {
            if (s[1] == '\\')
                s++;
            else if (s[1] == 'x' && isxdigit((unsigned char)s[2]) && isxdigit((unsigned char)s[3]))
            {
                c = hex_digit(s[2]) * 16 + hex_digit(s[3]);
                s += 3;
            }
            else
                return 0;
        }
        if (n + 1 >= len)
            return 0;
        out[n++] = (char)c;
    }
    out[n] = 0;
    return 1;
}

/* 1: a control key, 0: another key, -1: a bad value. */
static int control_key(reader_t *r, mock_control_t *c, const char *key, const char *value)
{
    NvU32 v[8];
    if (strcmp(key, "mode") == 0)
    {
        if (!named(mode_names, value, &c->mode))
            return fail(r, "bad mode '%s'", value);
    }
    else if (strcmp(key, "rgbw") == 0)
    {
        if (!numbers(value, v, 4, 255))
            return fail(r, "bad rgbw '%s' (R,G,B,W)", value);
        c->r = (NvU8)v[0];
        c->g = (NvU8)v[1];
        c->b = (NvU8)v[2];
        c->w = (NvU8)v[3];
    }
    else if (strcmp(key, "br") == 0)
    {
        if (!number(value, 255, v))
            return fail(r, "bad br '%s'", value);
        c->brightness = (NvU8)v[0];
    }
    else if (strcmp(key, "ep0") == 0 || strcmp(key, "ep1") == 0)
    {
        if (!numbers(value, v, 5, 255))
            return fail(r, "bad %s '%s' (R,G,B,W,BRIGHTNESS)", key, value);
        for (int i = 0; i < 5; i++)
            c->ep[key[2] - '0'][i] = (NvU8)v[i];
    }
    else if (strcmp(key, "pw") == 0)
    {
        char cycle[32];
        const char *rest = strchr(value, ',');
        size_t len = rest ? (size_t)(rest - value) : 0;
        if (!rest || len >= sizeof cycle)
            return fail(r, "bad pw '%s' (CYCLE,GROUPS,RISE,FALL,A,B,IDLE,PHASE)", value);
        memcpy(cycle, value, len);
        cycle[len] = 0;
        if (!named(cycle_names, cycle, &c->cycle) || !numbers(rest + 1, v, 7, 65535) || v[0] > 255)
            return fail(r, "bad pw '%s' (CYCLE,GROUPS,RISE,FALL,A,B,IDLE,PHASE)", value);
        c->group_count = (NvU8)v[0];
        c->rise_ms = (NvU16)v[1];
        c->fall_ms = (NvU16)v[2];
        c->a_ms = (NvU16)v[3];
        c->b_ms = (NvU16)v[4];
        c->idle_ms = (NvU16)v[5];
        c->phase_ms = (NvU16)v[6];
    }
    else
        return 0;
    return 1;
}

static int zone_key(reader_t *r, mock_zone_t *z, const char *key, const char *value)
{
    NvU32 v;
    if (strcmp(key, "type") == 0)
    {
        if (!named(zone_type_names, value, &z->type))
            return fail(r, "bad zone type '%s'", value);
    }
    else if (strcmp(key, "loc") == 0)
    {
        if (!named(location_names, value, &z->location))
            return fail(r, "bad location '%s'", value);
    }
    else if (strcmp(key, "dev") == 0 || strcmp(key, "prov") == 0)
    {
        if (!number(value, 255, &v))
            return fail(r, "bad %s '%s'", key, value);
        *(key[0] == 'd' ? &z->device : &z->provider) = (NvU8)v;
    }
    else if (strcmp(key, "fault") == 0)
    {
        if (!flags(zone_fault_names, value, &z->faults))
            return fail(r, "bad fault '%s' (set, stuck)", value);
    }
    else
    {
        int rc = control_key(r, &z->active, key, value);
        if (rc == 0 && key[0] == 'd')
            rc = control_key(r, &z->stored, key + 1, value);
        if (rc == 0)
            return fail(r, "unknown zone key '%s'", key);
        return rc < 0 ? -1 : 0;
    }
    return 0;
}

static int card_key(reader_t *r, mock_card_t *c, const char *key, const char *value)
{
    NvU32 v[5];
    if (strcmp(key, "uuid") == 0)
    {
        if (strcmp(value, "unsupported") == 0)
            c->uuid_mode = UUID_UNSUPPORTED;
        else if (strcmp(value, "zero") == 0)
            c->uuid_mode = UUID_ZERO;
        else if (parse_uuid(value, c->uuid))
            c->uuid_mode = UUID_REPORTED;
        else
            return fail(r, "bad uuid '%s' (GPU-..., unsupported or zero)", value);
    }
    else if (strcmp(key, "pci") == 0)
    {
        if (!numbers(value, v, 5, 0xFFFFFFFFu))
            return fail(r, "bad pci '%s' (BUS,DEVICE,SUBSYSTEM,REVISION,EXT)", value);
        c->has_pci = 1;
        c->bus_id = v[0];
        c->device_id = v[1];
        c->subsystem_id = v[2];
        c->revision_id = v[3];
        c->ext_device_id = v[4];
    }
    else if (strcmp(key, "info") == 0)
    {
        if (!numbers(value, v, 3, 0xFFFFFFFFu) || v[2] > 1)
            return fail(r, "bad info '%s' (RT_CORES,TENSOR_CORES,EXTERNAL)", value);
        c->has_info = 1;
        c->rt_cores = v[0];
        c->tensor_cores = v[1];
        c->external = v[2];
    }
    else if (strcmp(key, "fail") == 0)
    {
        if (!flags(card_fail_names, value, &c->fails))
            return fail(r, "bad fail '%s' (name, info, control, devices)", value);
    }
    else if (strcmp(key, "mask") == 0)
    {
        if (!number(value, 0xFFFFFFFFu, &c->mode_mask))
            return fail(r, "bad mask '%s'", value);
    }
    else if (strcmp(key, "info_zones") == 0 || strcmp(key, "control_zones") == 0)
    {
        if (!number(value, MOCK_MAX_ZONES, v))
            return fail(r, "bad %s '%s'", key, value);
        *(key[0] == 'i' ? &c->info_zones : &c->control_zones) = (int)v[0];
    }
    else
        return fail(r, "unknown card key '%s'", key);
    return 0;
}

static int option_key(reader_t *r, mock_rig_t *rig, const char *key, const char *value)
{
    NvU32 v;
    if (strcmp(key, "unresolved") == 0)
    {
        char buf[512];
        snprintf(buf, sizeof buf, "%s", value);
        char *save = NULL;
        for (char *tok = strtok_r(buf, ",", &save); tok; tok = strtok_r(NULL, ",", &save))
        {
            if (rig->nunresolved >= MOCK_MAX_UNRESOLVED || strlen(tok) >= sizeof rig->unresolved[0])
                return fail(r, "too many or too long unresolved names");
            snprintf(rig->unresolved[rig->nunresolved++], sizeof rig->unresolved[0], "%s", tok);
        }
    }
    else if (strcmp(key, "initialize_failures") == 0)
    {
        if (strcmp(value, "always") == 0)
            rig->initialize_failures = MOCK_ALWAYS;
        else if (number(value, 1000, &v))
            rig->initialize_failures = (int)v;
        else
            return fail(r, "bad initialize_failures '%s' (a number or always)", value);
    }
    else if (strcmp(key, "fail") == 0 && strcmp(value, "enumerate") == 0)
        rig->enumerate_fails = 1;
    else
        return fail(r, "unknown option '%s=%s'", key, value);
    return 0;
}

/* The KEY=VALUE words of an option, card or zone line; on a card line, name= takes the rest of the line. */
static int each_key(reader_t *r, char *s, mock_rig_t *rig, const char *what)
{
    mock_card_t *card = rig->ncards ? &rig->cards[rig->ncards - 1] : NULL;
    while (*s)
    {
        while (*s == ' ' || *s == '\t')
            s++;
        if (!*s)
            break;
        if (strcmp(what, "card") == 0 && strncmp(s, "name=", 5) == 0)
        {
            if (!parse_name(s + 5, card->name, sizeof card->name))
                return fail(r, "bad name (use \\\\ and \\xNN escapes, at most %d bytes)", NVAPI_SHORT_STRING_MAX - 1);
            break;
        }
        char *end = s + strcspn(s, " \t");
        int last = *end == 0;
        *end = 0;
        char *eq = strchr(s, '=');
        if (!eq)
            return fail(r, "expected KEY=VALUE, got '%s'", s);
        *eq = 0;
        int rc;
        if (strcmp(what, "option") == 0)
            rc = option_key(r, rig, s, eq + 1);
        else if (strcmp(what, "card") == 0)
            rc = card_key(r, card, s, eq + 1);
        else
            rc = zone_key(r, &card->zones[card->nzones - 1], s, eq + 1);
        if (rc)
            return rc;
        s = last ? end : end + 1;
    }
    return 0;
}

static int order_line(reader_t *r, char *s, mock_rig_t *rig)
{
    rig->norder = 0;
    char *save = NULL;
    for (char *tok = strtok_r(s, " \t", &save); tok; tok = strtok_r(NULL, " \t", &save))
    {
        NvU32 v;
        if (rig->norder >= MOCK_MAX_CARDS || !number(tok, MOCK_MAX_CARDS - 1, &v))
            return fail(r, "bad order entry '%s'", tok);
        for (int i = 0; i < rig->norder; i++)
            if (rig->order[i] == (int)v)
                return fail(r, "card %u is twice in the order", v);
        rig->order[rig->norder++] = (int)v;
    }
    return 0;
}

static int rig_line(reader_t *r, char *line, mock_rig_t *rig)
{
    char *s = line + strspn(line, " \t");
    if (!*s || *s == '#')
        return 0;
    char *word = s;
    s += strcspn(s, " \t");
    if (*s)
        *s++ = 0;
    if (strcmp(word, "option") == 0)
        return each_key(r, s, rig, word);
    if (strcmp(word, "order") == 0)
        return order_line(r, s, rig);
    if (strcmp(word, "card") == 0)
    {
        if (rig->ncards >= MOCK_MAX_CARDS)
            return fail(r, "too many cards");
        mock_card_t *c = &rig->cards[rig->ncards++];
        memset(c, 0, sizeof *c);
        c->uuid_mode = UUID_UNSUPPORTED;
        c->mode_mask = 1u << NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL;
        c->info_zones = c->control_zones = -1;
        return each_key(r, s, rig, word);
    }
    if (strcmp(word, "zone") == 0)
    {
        if (!rig->ncards)
            return fail(r, "a zone line before the first card line");
        mock_card_t *c = &rig->cards[rig->ncards - 1];
        if (c->nzones >= MOCK_MAX_ZONES)
            return fail(r, "too many zones");
        memset(&c->zones[c->nzones++], 0, sizeof c->zones[0]);
        return each_key(r, s, rig, word);
    }
    return fail(r, "unknown line '%s'", word);
}

static int check_rig(reader_t *r, const mock_rig_t *rig)
{
    for (int i = 0; i < rig->norder; i++)
        if (rig->order[i] >= rig->ncards)
            return fail(r, "the order names card %d, but there are %d", rig->order[i], rig->ncards);
    for (int i = 0; i < rig->ncards; i++)
    {
        const mock_card_t *c = &rig->cards[i];
        if (c->info_zones > c->nzones || c->control_zones > c->nzones)
            return fail(r, "card %d reports more zones than it has", i);
    }
    return 0;
}

int rig_load(const char *path, mock_rig_t *rig, char *err, size_t errlen)
{
    reader_t r = {path, 0, err, errlen};
    memset(rig, 0, sizeof *rig);
    rig->norder = -1;
    FILE *f = fopen(path, "r");
    if (!f)
    {
        snprintf(err, errlen, "cannot open %s: %s", path, strerror(errno));
        return -1;
    }
    char line[1024];
    int rc = 0;
    while (!rc && fgets(line, sizeof line, f))
    {
        r.lineno++;
        size_t len = strlen(line);
        if (len && line[len - 1] == '\n')
            line[--len] = 0;
        else if (!feof(f))
            rc = fail(&r, "line too long");
        if (!rc)
            rc = rig_line(&r, line, rig);
    }
    fclose(f);
    return rc ? rc : check_rig(&r, rig);
}
