"""The main window: status bar."""

from gi.repository import Adw, Gtk

from .config import APP_NAME
from .widgets import make_label


class MainWindow(Adw.ApplicationWindow):
    def __init__(self, app, felight, script_dir):
        super().__init__(application=app, title=APP_NAME)
        self.felight = felight
        self.script_dir = script_dir
        self.add_css_class("fe-window")
        self.set_default_size(1920, 1080)
        self.set_size_request(960, 540)

        root = self._build_header()
        self._build_status_bar(root)

    def _build_header(self):
        toolbar = Adw.ToolbarView()
        header = Adw.HeaderBar()
        header.set_title_widget(Gtk.Label(label=APP_NAME))
        toolbar.add_top_bar(header)
        self.set_content(toolbar)

        scroller = Gtk.ScrolledWindow(
            hscrollbar_policy=Gtk.PolicyType.NEVER, vexpand=True
        )
        self.toasts = Adw.ToastOverlay()
        self.toasts.set_child(scroller)
        toolbar.set_content(self.toasts)
        root = Gtk.Box(
            orientation=Gtk.Orientation.VERTICAL,
            spacing=12,
            margin_top=12,
            margin_bottom=12,
            margin_start=12,
            margin_end=12,
        )
        scroller.set_child(root)

        root.append(make_label(APP_NAME, "fe-title"))
        subtitle = make_label(
            "Control illumination zones on your Founders Edition GPU",
            "fe-subtitle",
            margin_start=36,
            margin_bottom=6,
        )
        root.append(subtitle)
        return root

    def _build_status_bar(self, root):
        status = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
        status.add_css_class("fe-status")
        self.status_label = make_label("Ready", hexpand=True)
        status.append(self.status_label)
        dot = Gtk.Box(valign=Gtk.Align.CENTER)
        dot.add_css_class("fe-dot")
        status.append(dot)
        root.append(status)

    def set_status(self, message):
        self.status_label.set_text(message)

    def toast(self, message):
        self.toasts.add_toast(Adw.Toast(title=message, timeout=3))

    def alert(self, heading, body):
        dialog = Adw.AlertDialog(heading=heading, body=body)
        dialog.add_response("ok", "OK")
        dialog.set_default_response("ok")
        dialog.present(self)
