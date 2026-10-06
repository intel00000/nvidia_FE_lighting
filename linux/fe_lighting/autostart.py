"""XDG autostart entry: the stable copy of felight it runs, Exec quoting, and the entry file itself."""

import os
import shutil
from enum import Enum, auto

from gi.repository import GLib

from .config import APP_NAME, AUTOSTART_DIR, AUTOSTART_FILE, STABLE_DIR, STARTUP_CONF


def find_startup_launcher_source(script_dir):
    for candidate in (
        os.path.join(script_dir, "fe-lighting-startup.sh"),
        shutil.which("fe-lighting-startup"),
    ):
        if candidate and os.access(candidate, os.X_OK):
            return candidate
    return None


def same_file_contents(src, dst):
    try:
        s, d = os.stat(src), os.stat(dst)
    except OSError:
        return False
    return s.st_size == d.st_size and int(s.st_mtime) == int(d.st_mtime)


def install_stable_copy(binary, script_dir):
    """Copy felight (and the launcher script, if present) into STABLE_DIR and return the command
    the autostart entry should run. Existing copies with the same size and mtime are kept."""
    os.makedirs(STABLE_DIR, exist_ok=True)
    binary_copy = os.path.join(STABLE_DIR, "felight")
    if not same_file_contents(binary, binary_copy):
        shutil.copy2(binary, binary_copy)
        os.chmod(binary_copy, 0o755)
    launcher_src = find_startup_launcher_source(script_dir)
    if launcher_src:
        launcher_copy = os.path.join(STABLE_DIR, "fe-lighting-startup.sh")
        if not same_file_contents(launcher_src, launcher_copy):
            shutil.copy2(launcher_src, launcher_copy)
            os.chmod(launcher_copy, 0o755)
        return [launcher_copy, STARTUP_CONF]
    return [binary_copy, "startup", STARTUP_CONF]


def desktop_exec_quote(arg):
    """Quote one Exec argument the way the Desktop Entry specification requires: the Exec quoting
    layer (double quotes, backslash before \\ " ` $) and then the key-file string escaping layer
    (every backslash doubled)."""
    reserved = set(" \t\n\"'\\><~|&;$*?#()`%")
    if arg and not any(ch in reserved for ch in arg):
        return arg
    exec_quoted = (
        '"'
        + arg.replace("\\", "\\\\")
        .replace('"', '\\"')
        .replace("`", "\\`")
        .replace("$", "\\$")
        + '"'
    )
    # A literal percent sign is a field-code escape in Exec and must be doubled.
    return exec_quoted.replace("\\", "\\\\").replace("%", "%%")


def desktop_exec_target(path):
    """Return the program an autostart entry runs, decoded the way GLib decodes it, or None."""
    try:
        keyfile = GLib.KeyFile()
        keyfile.load_from_file(path, GLib.KeyFileFlags.NONE)
        value = keyfile.get_string("Desktop Entry", "Exec")
        ok, argv = GLib.shell_parse_argv(value)
    except GLib.Error:
        return None
    return argv[0].replace("%%", "%") if ok and argv else None


def desktop_entry(command):
    """The text of the autostart entry that runs `command` (a list of arguments) at login."""
    exec_line = " ".join(desktop_exec_quote(part) for part in command)
    entry = (
        "[Desktop Entry]\n"
        "Type=Application\n"
        f"Name={APP_NAME} (apply at login)\n"
        "Comment=Applies the saved GPU illumination settings after login\n"
        f"Exec={exec_line}\n"
        "Terminal=false\n"
        "NoDisplay=true\n"
        "X-GNOME-Autostart-enabled=true\n"
    )
    return entry


def write_entry(command):
    """Write the autostart entry for `command`. Raises OSError."""
    entry = desktop_entry(command)
    os.makedirs(AUTOSTART_DIR, exist_ok=True)
    with open(AUTOSTART_FILE, "w", encoding="utf-8") as handle:
        handle.write(entry)


def remove_entry():
    """Remove the autostart entry if there is one. Raises OSError."""
    if os.path.exists(AUTOSTART_FILE):
        os.remove(AUTOSTART_FILE)


class EntryState(Enum):
    ABSENT = auto()
    VALID = auto()
    BROKEN = auto()


def entry_state():
    """ABSENT, VALID (exists, its program is executable and the startup settings exist) or BROKEN."""
    if not os.path.exists(AUTOSTART_FILE):
        return EntryState.ABSENT
    target = desktop_exec_target(AUTOSTART_FILE)
    if target and os.access(target, os.X_OK) and os.path.exists(STARTUP_CONF):
        return EntryState.VALID
    return EntryState.BROKEN
