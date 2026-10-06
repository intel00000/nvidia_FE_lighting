/* felight.h - common definitions: version, file names, limits and exit codes. */
#ifndef FELIGHT_H
#define FELIGHT_H

#include "nvapi_linux_compat.h"

#include "nvapi.h"

#define FELIGHT_VERSION "0.1.0"
#define APP_DIR_NAME "nvidia-fe-lighting"
#define STARTUP_FILE_NAME "startup.conf"
#define MAX_DELAY_SECONDS 86400

enum
{
    EXIT_USAGE = 1,
    EXIT_NO_LIBRARY = 2,
    EXIT_NVAPI = 3,
    EXIT_GPU_MISMATCH = 4,
    EXIT_VERIFY = 5,
    EXIT_PARTIAL = 6,
    EXIT_FILE = 7,
    EXIT_SERVICE = 8,
};

#endif
