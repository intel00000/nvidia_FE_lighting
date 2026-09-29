# Builds the Linux command-line tool `felight` from the sources in src/.
#
# Only the public NvAPI headers are needed at build time. Nothing from the NVIDIA driver is
# linked: felight dlopen()s libnvidia-api.so.1 at runtime, so this builds on any Linux box
# (including CI runners without a GPU) and needs neither CUDA nor the driver installed.
#
# The headers come from the NvApiWrapper/nvapi git submodule (run `git submodule update --init`).
# A top-level nvapi/ checkout is accepted as a fallback for local builds.

NVAPI_HEADER := $(firstword $(wildcard ../NvApiWrapper/nvapi/nvapi.h ../nvapi/nvapi.h))
NVAPI_DIR    := $(patsubst %/nvapi.h,%,$(NVAPI_HEADER))

CC       ?= gcc
CFLAGS   ?= -O2 -g -Wall -Wextra
CPPFLAGS += -D_GNU_SOURCE -isystem $(NVAPI_DIR) -Isrc/include
LDLIBS   += -ldl

SRCS := src/main.c \
        src/diag.c \
        src/nvapi_loader.c \
        src/cmd_version.c \
        src/model.c \
        src/device.c \
        src/parse.c \
        src/output.c
HDRS := $(wildcard src/include/*.h)

.PHONY: all clean check-headers

all: felight

check-headers:
	@test -n "$(NVAPI_HEADER)" || { echo "error: nvapi.h not found; run 'git submodule update --init NvApiWrapper/nvapi' from the repo root" >&2; exit 1; }

felight: $(SRCS) $(HDRS) | check-headers
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $(SRCS) $(LDLIBS)

clean:
	rm -f felight
