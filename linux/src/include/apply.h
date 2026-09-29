/* apply.h - applying a profile to a GPU. */
#ifndef APPLY_H
#define APPLY_H

#include "profile.h"

int apply_profile(const profile_t *p, long gpu_override, int verify_gpu, void (*say)(const char *, ...));

#endif
