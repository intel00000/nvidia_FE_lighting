# NVIDIA FE Lighting for Linux

Linux port of the Windows tool in this repository. It controls the illumination zones of NVIDIA Founders Edition cards and applies your saved settings automatically at startup or after login.

It consists of three components:

| File | What it is |
| --- | --- |
| `felight` (built from `src/*.c`) | Command-line tool that does all hardware access. It can also set up a systemd service that applies saved settings at boot. |
| `fe_lighting_gui.py` + `fe_lighting/` | GTK4 / libadwaita GUI that mirrors the Windows version and drives `felight`. `fe_lighting_gui.py` is the launcher. |
| `fe-lighting-startup.sh` | Launcher the GUI's login autostart entry runs; it execs `felight startup`. |

## How it works

NVIDIA's Linux driver has shipped `libnvidia-api.so.1` since release 525. It exports the same `nvapi_QueryInterface` entry point as `nvapi64.dll` on Windows, so the illumination functions
(`NvAPI_GPU_ClientIllumZonesGetInfo/GetControl/SetControl`) can be resolved by interface IDs and called directly.
No kernel module, I2C access, root privileges, CUDA, or X server is needed.

NVIDIA does not document NvAPI on Linux, so this is an undocumented interface that could change between driver releases.

NvAPI calls that the Linux library does not implement were dropped from the port:
`NvAPI_GetInterfaceVersionString`, `NvAPI_SYS_GetDriverAndBranchVersion` and `NvAPI_GPU_GetSystemType`.

## Requirements

- x86_64 Linux with the NVIDIA driver 525 or newer (open or proprietary kernel module).
- To build `felight`: `gcc` and `make`. The NvAPI headers come from the `NvApiWrapper/nvapi` git submodule.
- For the GUI: Python 3 with PyGObject, GTK 4.12+ and libadwaita 1.5+ (Ubuntu 24.04+, Fedora 39+; on Ubuntu the packages are `python3-gi gir1.2-gtk-4.0 gir1.2-adw-1`,
  already present on a GNOME desktop). Ubuntu 22.04 can run the `felight` command line but not the GUI.
- For `felight service`: systemd, and `sudo` or a root shell to install the service.

## From a release archive

```bash
tar xzf FELighting-linux-x86_64-<tag>.tar.gz
cd nvidia-fe-lighting
./felight list                # command line
./fe_lighting_gui.py          # GUI
```

## Build from source

```bash
git clone --recurse-submodules https://github.com/intel00000/nvidia_FE_lighting.git
cd nvidia_FE_lighting
# if you cloned without --recurse-submodules:  git submodule update --init NvApiWrapper/nvapi
make -C linux
linux/felight list
```

Optional install into `~/.local` (adds `felight`, `fe-lighting-startup` and `fe-lighting-gui` to
`~/.local/bin`, and the GUI's `fe_lighting_gui.py` and `fe_lighting/` package to
`~/.local/share/nvidia-fe-lighting`; `fe-lighting-gui` is a symlink to that `fe_lighting_gui.py`):

```bash
make -C linux install
```

Release archives built by GitHub Actions (`FELighting-linux-x86_64-<tag>.tar.gz`) contain the
same files prebuilt on Ubuntu 22.04, so they need glibc 2.35 or newer.

`make -C linux check` runs the tests in [tests/](tests/README.md) on test copies of `felight` that
can only load a mock NvAPI library. They need Python 3, but no GPU or NVIDIA driver.

## Command line

```text
felight list [--json]                                   all GPUs and their zones
felight get --gpu N [--default] [--json]                zones of one GPU
felight set --gpu N --zone Z [--rgb R,G,B | --color #RRGGBB] [--white W] [--brightness B]
                     [--no-verify] [--default]
felight save --gpu N [--delay S] FILE                   write the current state as a profile
felight apply [--gpu N] [--no-verify-gpu] FILE          apply a profile now
felight startup [--no-delay] FILE                       delayed apply of FILE, for the boot service
felight service enable --gpu N [--delay S] [--print]    apply GPU N's settings at boot and resume
felight service disable [--print]                       remove that boot service (both need root)
felight version
```

Examples:

```bash
felight set --gpu 0 --zone 1 --brightness 40        # dim the top zone
felight set --gpu 0 --zone 0 --color '#ff2000'      # RGB/RGBW zones only
felight save --gpu 0 ~/night.conf                   # snapshot the current state
felight apply ~/night.conf                          # restore it
felight service enable --gpu 0                      # apply the current state at boot and resume
```

Every write reads the zone table, patches one zone, writes the table back, and reads it again to
verify (`--no-verify` skips the read-back).
Color components must be 0-255 and brightness 0-100, invalid values are rejected.
`apply` and `startup` look for the GPU with the saved UUID; with `--gpu N` that GPU must be the
saved card.

Profile files are in plain text:

```text
# NVIDIA lighting profile written by felight 0.1.0 on 2026-10-06 14:22:59
delay_seconds=10
gpu_index=0
gpu_name=NVIDIA GeForce RTX 5090
gpu_bus_id=1
gpu_device_id=0x2b8510de
gpu_subsystem_id=0x205710de
gpu_uuid=GPU-01234567-89ab-cdef-0123-456789abcdef
zone 0 single_color brightness=50
zone 1 rgb r=255 g=32 b=0 brightness=100
zone 2 rgbw r=255 g=255 b=255 w=128 brightness=80
```

Exit codes:

- `0`: ok
- `1`: usage
- `2`: library/driver missing
- `3`: NvAPI error
- `4`: GPU or zone mismatch (nothing applied)
- `5`: write verification failed
- `6`: profile partially applied
- `7`: file error
- `8`: boot service setup failed

## GUI

```bash
python3 linux/fe_lighting_gui.py
```

The GUI uses `$FELIGHT_BIN` if it is set, otherwise `felight` next to itself, otherwise the first `felight` in `PATH`.
It stores profiles in `~/.config/nvidia-fe-lighting/profiles/profile_N.conf`.

Color changes are written immediately; sliders are written when you release them (keyboard and scroll-wheel changes once the value settles).
"Apply All" re-sends every zone and reports the ones that fail.

## Apply settings at boot

`felight service enable --gpu N` saves GPU N's current state and sets up a systemd service that applies it at every boot and after every resume from suspend or hibernation, with or without a desktop session.

```bash
felight service enable --gpu 0      # asks to re-run itself with sudo
journalctl -u nvidia-fe-lighting    # what the service did
felight service disable
```

`enable` writes:

- `/etc/nvidia-fe-lighting/startup.conf`: the saved settings
- `/usr/local/lib/nvidia-fe-lighting/felight`: a copy of `felight`, so the build or archive can move
- `/etc/systemd/system/nvidia-fe-lighting.service`: a oneshot unit ordered after `nvidia-persistenced` and `nvidia-resume`

If you decline `sudo` or there is no terminal, it prints the commands to run yourself; `--print` prints them without changing anything.
`--delay S` waits S seconds before applying. `disable` removes all three files.

## Apply settings at login

In the GUI, tick **Apply current settings on startup**. This needs no root:

- the current state goes to `~/.config/nvidia-fe-lighting/startup.conf`, applied 10 s after login
- `felight` and `fe-lighting-startup.sh` are copied to `~/.local/share/nvidia-fe-lighting/bin/`
- an autostart entry is written to `~/.config/autostart/nvidia-fe-lighting.desktop`

At login, `felight startup` waits the delay, finds the card by its saved UUID and applies every saved zone; its output goes to `journalctl --user`.
Unticking the box removes the autostart entry; the settings file and the copies stay.
If the entry's program goes missing, the box shows unticked with a status message; tick it again to repair it.

## Developer notes

- `fe_lighting_gui.py --selftest` drives the window like a user (detect zones, move a slider by one percent and restore it, save and load profile 1, toggle startup) and exits non-zero on the first failure. It writes to the card and to your configuration; set `FE_LIGHTING_CONFIG_DIR`, `FE_LIGHTING_AUTOSTART_DIR` and `FE_LIGHTING_DATA_DIR` to keep it away from your real files. `--screenshot PATH` renders the window to a PNG.
- `src/include/nvapi_linux_compat.h` predefines the SAL annotation macros the SDK headers use; without it `nvapi.h` does not compile with GCC or Clang.
- `felight get --gpu N --default` reads the card's stored default values, which are separate from the active ones; `set --default` writes them and has not been tested here.

### Source layout

`felight` is built from the C files in `src/`, with the headers in `src/include/`. Headers marked *data* hold types, constants and prototypes; the logic is in the `.c` files.

| File | Kind | Contents |
| --- | --- | --- |
| `felight.h` | data | Version, directory and file names, exit codes; included by every file. |
| `nvapi_linux_compat.h` | header | SAL annotation macros the NvAPI headers need. |
| `diag.h/.c` | logic | `log_err()`, `log_info()` (colored on a terminal; `DEBUG` builds add the caller). |
| `nvapi_loader.h` | data | `nvapi_t` table of resolved NvAPI functions, `g_nv`. |
| `nvapi_loader.c` | logic | Loads `libnvidia-api.so.1`, resolves functions by interface ID, reads the driver version. |
| `model.h` | data | `gpu_t`, `zone_t`. |
| `model.c` | logic | Zone type, location and mode names; color/white checks; `zone_read_control`. |
| `device.h` | data | `set_opts_t`. |
| `device.c` | logic | Reads GPUs (with their UUID) and zones, writes one zone (`set_zone`). |
| `parse.h/.c` | logic | Number, option and RGB parsing. |
| `output.h/.c` | logic | Text and JSON output. |
| `profile.h` | data | `profile_t`, `profile_zone_t`. |
| `profile.c` | logic | `profile_save`, `profile_load`. |
| `apply.h/.c` | logic | `apply_profile`, including the GPU identity checks. |
| `service.h` | data | Unit, settings and binary paths of the boot service. |
| `service.c` | logic | Installs and removes the boot service; prints the manual steps. |
| `commands.h` | decl | Prototypes of the `cmd_*` functions. |
| `cmd_query.c` | logic | `list`, `get`. |
| `cmd_set.c` | logic | `set`. |
| `cmd_profile.c` | logic | `save`, `apply`. |
| `cmd_startup.c` | logic | `startup`. |
| `cmd_service.c` | logic | `service enable`, `service disable`; offers to re-run itself with `sudo`. |
| `cmd_version.c` | logic | `version`. |
| `main.c` | logic | Command table, `usage`, `main`. |

`fe_lighting_gui.py` only starts the GUI; the code is in the `fe_lighting/` package. `model.py` holds the data classes, the other modules the logic and UI.

| File | Kind | Contents |
| --- | --- | --- |
| `model.py` | data | Dataclasses for `felight`'s JSON: `GpuList`, `Gpu`, `Zone`, `Piecewise`, `Endpoint`. |
| `config.py` | data | App ID and name, profile slots, timings, file and directory paths. |
| `style.css` | data | The GUI's CSS. |
| `__init__.py` | package | Selects GTK 4, GDK 4 and libadwaita 1 for every module. |
| `felight_cli.py` | logic | Finds `felight` and runs it (`Felight`, `FelightError`). |
| `autostart.py` | logic | Stable copies of `felight` and the launcher; writes, removes and checks the autostart entry (`EntryState`). |
| `style.py` | logic | Loads `style.css`. |
| `widgets.py` | logic | Color conversion, label, card and slider helpers. |
| `sliders.py` | logic | `SliderWriter`: writes a slider when it is released or its value settles. |
| `zone_card.py` | UI | `ZoneCard`, the widgets of one zone. |
| `window.py` | UI | `MainWindow`. |
| `app.py` | logic | `App` and `main()` (command-line options). |
| `selftest.py` | dev aid | `--selftest` and `--screenshot`. |
