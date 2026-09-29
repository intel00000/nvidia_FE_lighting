/*
 * felight - NVIDIA Founders Edition illumination control for Linux.
 *
 * The NVIDIA Linux driver has shipped libnvidia-api.so.1 since release 525. It exports a
 * single symbol, nvapi_QueryInterface, which is the same entry point nvapi64.dll uses on
 * Windows. This tool dlopen()s that library at runtime and resolves the illumination
 * functions by the interface IDs published in nvapi_interface.h, so it only needs the
 * public NvAPI headers to build and links against nothing but libc.
 *
 * NOTE: NVIDIA documents NvAPI as Windows-only; libnvidia-api.so.1 is undocumented on
 * Linux and could change between driver releases. OpenRGB has used the same path since 2024.
 *
 * Subcommands:
 *   list [--json]                                   all GPUs and their zones
 *   get --gpu N [--default] [--json]                zones of one GPU
 *   set --gpu N --zone Z [--rgb R,G,B | --color #RRGGBB] [--white W] [--brightness B]
 *                        [--no-verify] [--default]
 *   save --gpu N [--delay S] FILE                   write the current zone state as a profile
 *   apply [--gpu N] [--no-verify-gpu] FILE          apply a profile now
 *   version
 *
 * Exit codes: 0 ok, 1 usage, 2 library/driver unavailable, 3 NvAPI error,
 *             4 GPU/zone mismatch (nothing applied), 5 write verification failed,
 *             6 profile partially applied, 7 file error.
 */
#include "commands.h"
#include "diag.h"
#include "nvapi_loader.h"

#include <stdio.h>
#include <string.h>

typedef struct
{
    const char *name;
    int (*run)(int argc, char **argv);
    const char *help;
} command_t;

static const command_t commands[] = {
    {"list", cmd_list, "  list [--json]                                   all GPUs and their zones\n"},
    {"get", cmd_get, "  get --gpu N [--default] [--json]                zones of one GPU (--default: stored defaults)\n"},
    {"set", cmd_set,
     "  set --gpu N --zone Z [--rgb R,G,B | --color #RRGGBB] [--white W] [--brightness B]\n"
     "                       [--no-verify] [--default]  write one zone (manual mode; --default writes\n"
     "                                                  the card's stored defaults instead of the active set)\n"},
    {"save", cmd_save, "  save --gpu N [--delay S] FILE                   write the current state as a profile file\n"},
    {"apply", cmd_apply, "  apply [--gpu N] [--no-verify-gpu] FILE          apply a profile file now\n"},
    {"version", cmd_version, "  version\n"},
};

static void usage(FILE *f)
{
    fprintf(f,
            "felight %s - NVIDIA Founders Edition illumination control for Linux\n"
            "\n"
            "usage: felight <command> [options]\n",
            FELIGHT_VERSION);
    for (size_t i = 0; i < sizeof commands / sizeof commands[0]; i++)
        fputs(commands[i].help, f);
    fputs("\n"
          "exit codes: 0 ok, 1 usage, 2 library/driver missing, 3 NvAPI error,\n"
          "            4 GPU/zone mismatch (nothing applied), 5 write verification failed,\n"
          "            6 profile partially applied, 7 file error\n",
          f);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        usage(stderr);
        return EXIT_USAGE;
    }
    const char *cmd = argv[1];
    if (strcmp(cmd, "--version") == 0)
        cmd = "version";
    const command_t *c = NULL;
    for (size_t i = 0; i < sizeof commands / sizeof commands[0]; i++)
        if (strcmp(cmd, commands[i].name) == 0)
        {
            c = &commands[i];
            break;
        }
    int rc;
    if (c)
        rc = c->run(argc - 2, argv + 2);
    else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0)
    {
        usage(stdout);
        rc = 0;
    }
    else
    {
        log_err("felight: unknown command '%s'\n\n", cmd);
        usage(stderr);
        rc = EXIT_USAGE;
    }
    nv_close(&g_nv);
    if (g_log)
        fclose(g_log);
    return rc;
}
