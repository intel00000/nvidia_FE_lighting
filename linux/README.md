# NVIDIA FE Lighting for Linux

Linux port of the Windows tool in this repository. It controls the illumination zones of NVIDIA Founders Edition cards and apply your saved settings automatically at startup or after login.

It consists of three components:

| File | What it is |
| --- | --- |
| `felight` (built from `src/*.c`) | Command-line tool that does the access. It can also set up a systemd service that applies saved settings at boot. |
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
