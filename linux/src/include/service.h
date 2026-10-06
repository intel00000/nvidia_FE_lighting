/* service.h - systemd boot service that applies the saved settings. */
#ifndef SERVICE_H
#define SERVICE_H

#include "felight.h"

#include <stdio.h>

#define SERVICE_NAME APP_DIR_NAME ".service"
#define SERVICE_UNIT_PATH "/etc/systemd/system/" SERVICE_NAME
#define SERVICE_CONF_DIR "/etc/" APP_DIR_NAME
#define SERVICE_CONF_PATH SERVICE_CONF_DIR "/" STARTUP_FILE_NAME
#define SERVICE_BIN_DIR "/usr/local/lib/" APP_DIR_NAME
#define SERVICE_BIN_PATH SERVICE_BIN_DIR "/felight"

int service_enable(const char *self, NvU32 gpu, int delay_seconds);
int service_disable(void);
void service_print_enable_steps(FILE *f, const char *self, long gpu, long delay_seconds);
void service_print_disable_steps(FILE *f);

#endif
