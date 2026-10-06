# NVIDIA FE Lighting Control

Controlling RGB/RGBW illumination zones on NVIDIA graphics cards, primarily for Founders Edition models. Only tested on RTX50 series, but should work on 30/40 series as well.

![Build Status](https://github.com/intel00000/nvidia_FE_lighting/workflows/Build%20and%20Release/badge.svg)

## Features

- **Full Zone Control**: Adjust RGB/RGBW colors and brightness
- **Profile Management**: Save and load up to 5 profiles
- **Startup Integration**: Automatically apply settings on Windows startup

## Requirements

- Windows 10/11 (64-bit), or Linux (see [Linux](#linux) below)
- .NET 8.0.x Runtime ([Download here](https://dotnet.microsoft.com/download/dotnet/8.0)) (Windows only)
- NVIDIA drivers installed

## Installation

### From Releases

1. Download the latest release ZIP from the [Releases](https://github.com/intel00000/nvidia_FE_lighting/releases) page
2. Extract the single `nvidia_FE_lighting.exe` file
3. Run it.

### Building from Source

**Prerequisites:**

- Visual Studio 2022
- .NET 8.0 SDK

**Build Steps for Single-File Executable:**

```bash
git clone --recurse-submodules https://github.com/intel00000/nvidia_FE_lighting.git
cd nvidia_FE_lighting

# If you already cloned without --recurse-submodules, run:
# git submodule update --init --recursive

# Build the C++ wrapper
msbuild nvidia_FE_lighting.sln /p:Configuration=Release /p:Platform=x64 /t:NvApiWrapper

# Publish as single-file executable
dotnet publish nvidia_FE_lighting\nvidia_FE_lighting.csproj -c Release -r win-x64 --self-contained false -p:PublishSingleFile=true -o publish
```

Output will be a single `nvidia_FE_lighting.exe` in the `publish\` folder.

## Usage

1. **Select GPU**: Choose your GPU from the dropdown
2. **Detect Zones**: Click "Detect Zones" to find all available illumination zones
3. **Customize Colors**: Use the color pickers and sliders to adjust each zone
4. **Apply Settings**: Click "Apply All" to apply the changes
5. **Save Profile**: Save your favorite configurations to profile slots
6. **Enable Startup**: Check "Apply current settings on startup" to automatically apply settings on boot

## Startup Feature

The startup feature ensures your lighting settings persist across reboots:

- **10-second delay** after boot to allow driver initialization
- **GPU verification** prevents applying settings to wrong GPU after hardware changes
- **Lightweight startup** runs without UI when triggered at startup
- **Logging** in `%AppData%\NvidiaFELighting\startup_log.txt`

The application will copy itself to `%AppData%\NvidiaFELighting\FELighting.exe` for reliable startup execution.

## Linux

A native Linux port lives in [`linux/`](linux/): the `felight` command-line tool (C, talks to the driver's `libnvidia-api.so.1`), a GTK4/libadwaita GUI that mirrors this window, and a login autostart launcher. `felight service enable` sets up a systemd service that applies your settings at boot, also on machines without a desktop. Build it with `make -C linux`; see [linux/README.md](linux/README.md) for details, requirements and how to apply your settings at boot or login. Release archives for Linux are built by the same GitHub Actions workflow as the Windows exe.

## Project Structure

```
nvidia_FE_lighting/
├── nvidia_FE_lighting/         # Main WPF application (C#)
│   ├── App.xaml/App.xaml.cs    # Application entry point and startup logic
│   ├── MainWindow.xaml/.cs     # Main UI and control logic
│   └── NvApiWrapper.cs         # P/Invoke declarations
├── NvApiWrapper/               # C++ wrapper for NVIDIA API (Windows)
│   ├── NvApiDll.cpp            # NVAPI implementation
│   ├── NvApiDll.h              # Header file
│   └── nvapi/                  # NVIDIA NvAPI SDK (git submodule)
└── linux/                      # Linux port
    ├── src/                    # felight command-line tool (C) over libnvidia-api.so.1
    │   └── include/            # Headers, incl. nvapi_linux_compat.h (NvAPI headers with GCC/Clang)
    ├── fe_lighting/            # GTK4 / libadwaita GUI (Python package)
    ├── fe_lighting_gui.py      # GUI launcher
    └── fe-lighting-startup.sh  # Login autostart launcher
```

## Acknowledgments

- The NVAPI library
- Extended WPF Toolkit for the ColorPicker control
