/* cmd_service.c - the service command: sets up or removes the boot service. */
#include "commands.h"
#include "diag.h"
#include "model.h"
#include "parse.h"
#include "service.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void print_steps(int enable, const char *self, long gpu, long delay)
{
    if (enable)
        service_print_enable_steps(stdout, self, gpu, delay);
    else
        service_print_disable_steps(stdout);
}

int cmd_service(int argc, char **argv)
{
    int enable = argc > 0 && strcmp(argv[0], "enable") == 0;
    int disable = argc > 0 && strcmp(argv[0], "disable") == 0;
    long gpu = -1, delay = 0;
    int print = 0;
    if (!enable && !disable)
    {
        log_err("felight service: usage: service enable --gpu N [--delay S] [--print] | service disable [--print]\n");
        return EXIT_USAGE;
    }
    for (int i = 1; i < argc; i++)
    {
        if (enable && strcmp(argv[i], "--gpu") == 0)
        {
            if (!parse_opt("--gpu", argv[++i], 0, MAX_GPU_INDEX, &gpu))
                return EXIT_USAGE;
        }
        else if (enable && strcmp(argv[i], "--delay") == 0)
        {
            if (!parse_opt("--delay", argv[++i], 0, MAX_DELAY_SECONDS, &delay))
                return EXIT_USAGE;
        }
        else if (strcmp(argv[i], "--print") == 0)
            print = 1;
        else
        {
            log_err("felight service %s: unknown option %s\n", argv[0], argv[i]);
            return EXIT_USAGE;
        }
    }
    if (enable && gpu < 0)
    {
        log_err("felight service enable: --gpu N is required\n");
        return EXIT_USAGE;
    }
    char self[PATH_MAX];
    if (!realpath("/proc/self/exe", self))
        snprintf(self, sizeof self, "felight");
    if (print)
    {
        print_steps(enable, self, gpu, delay);
        return 0;
    }
    if (geteuid() != 0)
    {
        fprintf(stderr, "felight service %s changes system files and needs root.\n", argv[0]);
        if (isatty(STDIN_FILENO) && isatty(STDERR_FILENO))
        {
            fprintf(stderr, "Run it with sudo now? [Y/n] ");
            char answer[16] = "";
            if (fgets(answer, sizeof answer, stdin) && answer[0] && strchr("yY\n", answer[0]))
            {
                char *args[argc + 4];
                args[0] = "sudo";
                args[1] = self;
                args[2] = "service";
                for (int i = 0; i < argc; i++)
                    args[3 + i] = argv[i];
                args[argc + 3] = NULL;
                execvp("sudo", args);
                log_err("felight: cannot run sudo: %s\n", strerror(errno));
            }
        }
        fprintf(stderr, "To do it yourself, run:\n");
        print_steps(enable, self, gpu, delay);
        return EXIT_USAGE;
    }
    return enable ? service_enable(self, (NvU32)gpu, (int)delay) : service_disable();
}
