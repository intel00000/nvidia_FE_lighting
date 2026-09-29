/* output.h - text and JSON output of GPUs and zones. */
#ifndef OUTPUT_H
#define OUTPUT_H

#include "model.h"

#include <stdio.h>

void json_string(FILE *f, const char *s);
void print_gpu_json(FILE *f, const gpu_t *g, const zone_t *zones, NvU32 nzones, int error);
void print_gpu_text(FILE *f, const gpu_t *g, const zone_t *zones, NvU32 nzones);

#endif
