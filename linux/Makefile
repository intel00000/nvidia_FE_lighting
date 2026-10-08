# Builds the Linux command-line tool `felight` from the sources in src/.
#
# Only the public NvAPI headers are needed at build time. Nothing from the NVIDIA driver is
# linked: felight dlopen()s libnvidia-api.so.1 at runtime, so this builds on any Linux box
# (including CI runners without a GPU) and needs neither CUDA nor the driver installed.
#
# The headers come from the NvApiWrapper/nvapi git submodule (run `git submodule update --init`).
# A top-level nvapi/ checkout is accepted as a fallback for local builds.
#
# `make install` puts felight and fe-lighting-startup in BINDIR, the GUI (fe_lighting_gui.py and
# the fe_lighting package) in DATADIR, and a fe-lighting-gui symlink to it in BINDIR.
#
# `make check` builds test copies of felight that can only load the mock NvAPI library in
# tests/mock, and runs tests/run.py on them. It needs Python 3 but no GPU or NVIDIA driver.

NVAPI_HEADER := $(firstword $(wildcard ../NvApiWrapper/nvapi/nvapi.h ../nvapi/nvapi.h))
NVAPI_DIR    := $(patsubst %/nvapi.h,%,$(NVAPI_HEADER))

CC       ?= gcc
CFLAGS   ?= -O2 -g -Wall -Wextra
CPPFLAGS += -D_GNU_SOURCE -isystem $(NVAPI_DIR) -Isrc/include
LDLIBS   += -ldl

PREFIX  ?= $(HOME)/.local
BINDIR  ?= $(PREFIX)/bin
DATADIR ?= $(PREFIX)/share/nvidia-fe-lighting

SRCS := src/main.c \
        src/diag.c \
        src/nvapi_loader.c \
        src/cmd_version.c \
        src/model.c \
        src/device.c \
        src/parse.c \
        src/output.c \
        src/cmd_query.c \
        src/cmd_set.c \
        src/profile.c \
        src/cmd_profile.c \
        src/apply.c \
        src/cmd_startup.c \
        src/service.c \
        src/cmd_service.c
HDRS := $(wildcard src/include/*.h)

# Test builds never log in DEBUG style, because the tests compare felight's output.
TEST_OUT    := build/tests
TEST_CFLAGS := -UDEBUG
TEST_WRAP   := -Wl,--wrap=dlopen -Wl,--wrap=sleep -Wl,--wrap=usleep
MOCK_SRCS   := $(wildcard tests/mock/*.c)
MOCK_HDRS   := $(wildcard tests/mock/*.h)
SHIM_SRCS   := tests/shim/wrap.c
CHECK_ARGS  ?=

.PHONY: all clean install uninstall check-headers check

all: felight

check-headers:
	@test -n "$(NVAPI_HEADER)" || { echo "error: nvapi.h not found; run 'git submodule update --init NvApiWrapper/nvapi' from the repo root" >&2; exit 1; }

felight: $(SRCS) $(HDRS) | check-headers
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $(SRCS) $(LDLIBS)

install: felight
	install -d $(DESTDIR)$(BINDIR) $(DESTDIR)$(DATADIR)/fe_lighting
	install -m 0755 felight $(DESTDIR)$(BINDIR)/felight
	install -m 0755 fe-lighting-startup.sh $(DESTDIR)$(BINDIR)/fe-lighting-startup
	install -m 0644 fe_lighting/*.py fe_lighting/style.css $(DESTDIR)$(DATADIR)/fe_lighting/
	install -m 0755 fe_lighting_gui.py $(DESTDIR)$(DATADIR)/fe_lighting_gui.py
	ln -sf $(DATADIR)/fe_lighting_gui.py $(DESTDIR)$(BINDIR)/fe-lighting-gui

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/felight $(DESTDIR)$(BINDIR)/fe-lighting-startup $(DESTDIR)$(BINDIR)/fe-lighting-gui
	rm -f $(DESTDIR)$(DATADIR)/fe_lighting_gui.py
	rm -rf $(DESTDIR)$(DATADIR)/fe_lighting

check: $(TEST_OUT)/felight $(TEST_OUT)/libnvidia-api.so.1
	python3 -B tests/run.py $(TEST_OUT) $(CHECK_ARGS)

$(TEST_OUT)/felight: $(SRCS) $(HDRS) $(SHIM_SRCS) | check-headers
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(TEST_CFLAGS) $(CPPFLAGS) $(TEST_WRAP) -o $@ $(SRCS) $(SHIM_SRCS) $(LDLIBS)

$(TEST_OUT)/libnvidia-api.so.1: $(MOCK_SRCS) $(MOCK_HDRS) | check-headers
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(TEST_CFLAGS) $(CPPFLAGS) -fPIC -shared -Wl,-z,nodelete -o $@ $(MOCK_SRCS)

clean:
	rm -rf felight build
