"""The Adw.Application and main(): version check, felight lookup."""

import sys

from gi.repository import Adw, Gio, Gtk

from . import style
from .config import APP_ID, APP_NAME
from .felight_cli import Felight, find_binary
from .window import MainWindow


class App(Adw.Application):
    def __init__(self, script_dir):
        super().__init__(application_id=APP_ID, flags=Gio.ApplicationFlags.NON_UNIQUE)
        self.script_dir = script_dir

    def do_startup(self):
        Adw.Application.do_startup(self)
        Adw.StyleManager.get_default().set_color_scheme(Adw.ColorScheme.FORCE_DARK)
        style.install_css()

    def do_activate(self):
        window = self.get_active_window()
        if window is None:
            binary = find_binary(self.script_dir)
            if binary is None:
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
                return
            window = MainWindow(self, Felight(binary), self.script_dir)
        window.present()


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
    app = App(script_dir)
    rc = app.run([sys.argv[0], *args])
    return rc
