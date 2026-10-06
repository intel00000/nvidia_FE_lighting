"""Application constants and the locations of the profile, startup and autostart files."""

import os

from gi.repository import GLib

APP_ID = "io.github.intel00000.NvidiaFELighting"
APP_NAME = "NVIDIA FE Lighting"
APP_DIR_NAME = "nvidia-fe-lighting"
PROFILE_SLOTS = 5
STARTUP_DELAY_SECONDS = 10
SLIDER_DEBOUNCE_MS = 150

CONFIG_DIR = os.environ.get("FE_LIGHTING_CONFIG_DIR") or os.path.join(
    GLib.get_user_config_dir(), APP_DIR_NAME
)
PROFILES_DIR = os.path.join(CONFIG_DIR, "profiles")
STARTUP_CONF = os.path.join(CONFIG_DIR, "startup.conf")
AUTOSTART_DIR = os.environ.get("FE_LIGHTING_AUTOSTART_DIR") or os.path.join(
    GLib.get_user_config_dir(), "autostart"
)
AUTOSTART_FILE = os.path.join(AUTOSTART_DIR, "nvidia-fe-lighting.desktop")
STABLE_DIR = os.environ.get("FE_LIGHTING_DATA_DIR") or os.path.join(
    GLib.get_user_data_dir(), APP_DIR_NAME, "bin"
)


def profile_path(slot):
    return os.path.join(PROFILES_DIR, f"profile_{slot}.conf")
