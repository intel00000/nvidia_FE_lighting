"""Small widget helpers shared by the window and the zone cards."""

from gi.repository import Gdk, Gtk


def rgba_from_bytes(r, g, b):
    return Gdk.RGBA(red=r / 255.0, green=g / 255.0, blue=b / 255.0, alpha=1.0)


def bytes_from_rgba(rgba):
    return (round(rgba.red * 255), round(rgba.green * 255), round(rgba.blue * 255))


def make_label(text, css=None, **kwargs):
    label = Gtk.Label(label=text, xalign=0.0, **kwargs)
    if css:
        for cls in css.split():
            label.add_css_class(cls)
    return label


def make_card():
    box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
    box.add_css_class("fe-card")
    return box


def make_slider(lo, hi, value):
    scale = Gtk.Scale.new_with_range(Gtk.Orientation.HORIZONTAL, lo, hi, 1)
    scale.set_value(value)
    scale.set_draw_value(True)
    scale.set_value_pos(Gtk.PositionType.RIGHT)
    scale.set_digits(0)
    scale.set_size_request(220, -1)
    scale.set_valign(Gtk.Align.CENTER)
    return scale
