"""The Adw.Application and main(): version check, command-line options, felight lookup."""

import sys

from gi.repository import Adw, Gio, GLib, Gtk

from . import style
from .config import APP_ID, APP_NAME
from .felight_cli import Felight, find_binary
from .selftest import run_selftest, try_screenshot
from .window import MainWindow


class App(Adw.Application):
    def __init__(self, script_dir, selftest=False, screenshot=None):
        super().__init__(application_id=APP_ID, flags=Gio.ApplicationFlags.NON_UNIQUE)
        self.script_dir = script_dir
        self.selftest = selftest
        self.screenshot = screenshot
        self.selftest_ok = True

    def do_startup(self):
        Adw.Application.do_startup(self)
        Adw.StyleManager.get_default().set_color_scheme(Adw.ColorScheme.FORCE_DARK)
        style.install_css()

    def do_activate(self):
        window = self.get_active_window()
        if window is None:
            binary = find_binary(self.script_dir)
            if binary is None:
                self.selftest_ok = False
                dialog = Adw.AlertDialog(
                    heading="felight not found",
                    body="Build it with `make` in the linux/ folder or install it into your PATH, "
                    "or point FELIGHT_BIN at the binary.",
                )
                dialog.add_response("ok", "Quit")
                dialog.connect("response", lambda *_: self.quit())
                holder = Adw.ApplicationWindow(application=self, title=APP_NAME)
                holder.set_default_size(400, 200)
                holder.present()
                dialog.present(holder)
                if self.selftest:
                    GLib.timeout_add(500, self.quit)
                return
            window = MainWindow(self, Felight(binary), self.script_dir)
        window.present()
        if self.selftest:

            def done(ok):
                self.selftest_ok = ok
                self.quit()

            run_selftest(window, self.screenshot, done)
        elif self.screenshot:
            attempts = [0]

            def shoot():
                attempts[0] += 1
                if try_screenshot(window, self.screenshot) or attempts[0] >= 10:
                    self.quit()
                    return GLib.SOURCE_REMOVE
                return GLib.SOURCE_CONTINUE

            GLib.timeout_add(500, shoot)


def main(script_dir):
    gtk_version = (Gtk.get_major_version(), Gtk.get_minor_version())
    adw_version = (Adw.get_major_version(), Adw.get_minor_version())
    if gtk_version < (4, 12) or adw_version < (1, 5):
        print(
            f"{APP_NAME} needs GTK 4.12+ and libadwaita 1.5+ (found GTK {gtk_version[0]}.{gtk_version[1]}, "
            f"libadwaita {adw_version[0]}.{adw_version[1]}). The felight command line still works.",
            file=sys.stderr,
        )
        return 1
    args = sys.argv[1:]
    selftest = "--selftest" in args
    screenshot = None
    if "--screenshot" in args:
        pos = args.index("--screenshot")
        if pos + 1 >= len(args):
            print(
                "usage: fe_lighting_gui.py [--selftest] [--screenshot PATH]",
                file=sys.stderr,
            )
            return 1
        screenshot = args[pos + 1]
        del args[pos : pos + 2]
    args = [a for a in args if a != "--selftest"]
    app = App(script_dir, selftest=selftest, screenshot=screenshot)
    rc = app.run([sys.argv[0], *args])
    if selftest and not app.selftest_ok:
        return 1
    return rc
