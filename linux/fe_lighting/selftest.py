"""Developer aids: --screenshot PATH renders the window to a PNG; --selftest drives the
window like a user (detect, slider write + restore, profile save/load, startup toggle)."""

import os
import shutil
import subprocess
import sys

from gi.repository import GLib, Gtk

from .autostart import desktop_exec_target
from .config import (
    AUTOSTART_FILE,
    SLIDER_DEBOUNCE_MS,
    STABLE_DIR,
    STARTUP_CONF,
    profile_path,
)


def try_screenshot(window, path):
    """Render the window content to a PNG. Returns False if GTK has not laid it out yet."""
    for widget in (window.get_content(), window):
        paintable = Gtk.WidgetPaintable.new(widget)
        snapshot = Gtk.Snapshot()
        paintable.snapshot(snapshot, widget.get_width(), widget.get_height())
        node = snapshot.to_node()
        if node is not None:
            texture = window.get_renderer().render_texture(node, None)
            texture.save_to_png(path)
            return True
    return False


def run_selftest(window, screenshot_path, on_done):
    steps = selftest_steps(window, screenshot_path)

    def advance():
        try:
            delay = next(steps)
        except StopIteration:
            on_done(True)
            return GLib.SOURCE_REMOVE
        except Exception as exc:  # noqa: BLE001 - any failure ends the self-test
            print(f"SELFTEST FAIL: {exc}", file=sys.stderr)
            on_done(False)
            return GLib.SOURCE_REMOVE
        GLib.timeout_add(delay, advance)
        return GLib.SOURCE_REMOVE

    GLib.timeout_add(500, advance)


def selftest_steps(window, screenshot_path):
    def check(condition, message):
        if not condition:
            raise AssertionError(message)
        print("SELFTEST ok:", message)

    check(window.gpus, "GPUs listed")
    window.on_detect_clicked(None)
    yield 100
    check(window.zones, "zones detected")
    zone = window.zones[0]
    idx = zone.index
    original = zone.brightness
    target = original + 1 if original < 100 else original - 1
    scale = window.zone_cards[idx].brightness
    active = window.zone_cards[idx].active
    scale.set_value(target)
    yield SLIDER_DEBOUNCE_MS + 500
    live = window.felight.get(window.current_gpu).zones[0].brightness
    check(live == target, f"brightness slider wrote {target}% (read back {live}%)")
    if active is not None and not zone.has_color:
        check(
            active.get_text() == f"Active brightness: {target}%",
            "active line follows the write without re-detecting: " + active.get_text(),
        )
    scale.set_value(original)
    yield SLIDER_DEBOUNCE_MS + 500
    live = window.felight.get(window.current_gpu).zones[0].brightness
    check(live == original, f"brightness restored to {original}% (read back {live}%)")
    if active is not None and not zone.has_color:
        check(
            active.get_text() == f"Active brightness: {original}%",
            "active line follows the restore: " + active.get_text(),
        )
    window.on_save_profile(None, 1)
    yield 100
    check(os.path.exists(profile_path(1)), "profile 1 saved to " + profile_path(1))
    window.on_load_profile(None, 1)
    yield 100
    check(
        window.status_label.get_text() == "Loaded profile 1",
        "profile 1 loaded, status: " + window.status_label.get_text(),
    )
    window.startup_check.set_active(True)
    yield 100
    check(os.path.exists(STARTUP_CONF), "startup settings written to " + STARTUP_CONF)
    check(
        os.path.exists(AUTOSTART_FILE), "autostart entry written to " + AUTOSTART_FILE
    )
    check(
        os.access(os.path.join(STABLE_DIR, "felight"), os.X_OK),
        "felight copied to " + STABLE_DIR,
    )
    target = desktop_exec_target(AUTOSTART_FILE)
    check(
        target and os.access(target, os.X_OK),
        f"autostart Exec target is executable: {target}",
    )
    with open(AUTOSTART_FILE, encoding="utf-8") as handle:
        print(handle.read())
    validator = shutil.which("desktop-file-validate")
    if validator:
        proc = subprocess.run(
            [validator, AUTOSTART_FILE], capture_output=True, text=True, check=False
        )
        check(
            proc.returncode == 0,
            "desktop-file-validate accepts the entry: "
            + (proc.stdout + proc.stderr).strip(),
        )
    check(window.autostart_entry_is_valid(), "autostart entry validates on reload")
    window.startup_check.set_active(False)
    yield 100
    check(not os.path.exists(AUTOSTART_FILE), "autostart entry removed")
    if screenshot_path:
        window.queue_draw()
        for _attempt in range(10):
            yield 250
            if try_screenshot(window, screenshot_path):
                print("SELFTEST ok: screenshot written to " + screenshot_path)
                break
        else:
            print(
                "SELFTEST warn: screenshot skipped, the window was not drawn (occluded?)"
            )
        yield 100
