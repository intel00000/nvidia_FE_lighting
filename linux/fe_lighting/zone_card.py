"""The card shown for one illumination zone: header, state, color picker and sliders."""

from gi.repository import Gtk

from .widgets import make_label, make_slider, rgba_from_bytes


def zone_header_text(zone):
    mode = zone.ctrl_mode
    mode_label = {"manual": "Manual", "piecewise_linear": "Piecewise Linear"}.get(
        mode, mode.replace("_", " ").title()
    )
    return (
        f"Zone {zone.index}: {mode_label} for {zone.type_label} @ {zone.location_label}"
    )


def zone_active_text(zone):
    if zone.has_white:
        return f"Active RGBW: R={zone.r}, G={zone.g}, B={zone.b}, W={zone.w}"
    if zone.has_color:
        return f"Active RGB: R={zone.r}, G={zone.g}, B={zone.b}"
    return f"Active brightness: {zone.brightness}%"


class ZoneCard(Gtk.Box):
    """One card per zone. `header`, `active`, `color`, `white` and `brightness` are the card's
    widgets, None when the card has no such widget."""

    def __init__(self, zone, gpu_index, window):
        super().__init__(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        self.add_css_class("fe-zone-card")
        idx = zone.index
        self.active = None
        self.color = None
        self.white = None
        self.brightness = None
        header = make_label(zone_header_text(zone), margin_bottom=4)
        header.add_css_class("fe-heading")
        self.append(header)
        self.header = header

        if not zone.controllable:
            self.append(make_label("This zone cannot be controlled.", "fe-secondary"))
            return

        if zone.piecewise is not None:
            pw = zone.piecewise
            self.append(
                make_label(
                    f"Piecewise Mode: {pw.cycle}, Group Count: {pw.group_count}",
                    "fe-secondary",
                )
            )
            for j, ep in enumerate(pw.endpoints):
                if zone.has_white:
                    text = f"  [{j}] R={ep.r}, G={ep.g}, B={ep.b}, W={ep.w}, Bright={ep.brightness}"
                elif zone.has_color:
                    text = (
                        f"  [{j}] R={ep.r}, G={ep.g}, B={ep.b}, Bright={ep.brightness}"
                    )
                else:
                    text = f"  [{j}] Brightness={ep.brightness}"
                self.append(make_label(text, "fe-secondary"))
            self.append(
                make_label(
                    "Changing a value below switches this zone to manual mode.",
                    "fe-secondary",
                )
            )
        else:
            active = make_label(zone_active_text(zone), "fe-secondary")
            self.append(active)
            self.active = active

        if zone.has_color:
            row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
            row.append(make_label("Color: ", valign=Gtk.Align.CENTER))
            dialog = Gtk.ColorDialog(with_alpha=False, title=f"Zone {idx} color")
            picker = Gtk.ColorDialogButton(dialog=dialog, valign=Gtk.Align.CENTER)
            picker.set_rgba(rgba_from_bytes(zone.r, zone.g, zone.b))
            picker.connect("notify::rgba", window.on_color_changed, gpu_index, idx)
            row.append(picker)
            self.color = picker
            if zone.has_white:
                row.append(
                    make_label("  White: ", valign=Gtk.Align.CENTER, margin_start=10)
                )
                white = make_slider(0, 255, zone.w)
                row.append(white)
                self.white = white
            self.append(row)

        row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        row.append(make_label("Brightness: ", valign=Gtk.Align.CENTER))
        brightness = make_slider(0, 100, zone.brightness)
        row.append(brightness)
        self.brightness = brightness
        self.append(row)

    def refresh_labels(self, zone):
        if self.header is not None:
            self.header.set_text(zone_header_text(zone))
        if self.active is not None:
            self.active.set_text(zone_active_text(zone))
