"""Installs the window stylesheet for the default display."""

import os

from gi.repository import Gdk, Gtk


def install_css():
    css_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "style.css")
    provider = Gtk.CssProvider()
    provider.load_from_path(css_path)
    Gtk.StyleContext.add_provider_for_display(
        Gdk.Display.get_default(), provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
    )
