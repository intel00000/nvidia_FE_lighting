"""The main window: GPU selection and information, zone cards, status bar."""

from gi.repository import Adw, GLib, Gtk

from .config import APP_NAME
from .felight_cli import FelightError
from .sliders import SliderWriter
from .widgets import bytes_from_rgba, make_card, make_label
from .zone_card import ZoneCard


class MainWindow(Adw.ApplicationWindow):
    def __init__(self, app, felight, script_dir):
        super().__init__(application=app, title=APP_NAME)
        self.felight = felight
        self.script_dir = script_dir
        self.add_css_class("fe-window")
        self.set_default_size(1920, 1080)
        self.set_size_request(960, 540)

        self.gpus = []  # from `felight list --json`
        self.current_gpu = 0
        self.zones = []  # cache of the last `felight get --json`
        self.zone_cards = {}  # zone index -> ZoneCard
        self.sliders = SliderWriter(self.apply_slider)
        self.initializing = True

        root = self._build_header()
        self._build_gpu_section(root)
        self._build_zones_section(root)
        self._build_status_bar(root)

        self.populate_gpus()
        self.initializing = False

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

    def _build_gpu_section(self, root):
        top = Gtk.Box(
            orientation=Gtk.Orientation.HORIZONTAL, spacing=12, homogeneous=True
        )
        root.append(top)

        select_card = make_card()
        select_card.append(make_label("Select GPU", "fe-heading", margin_bottom=4))
        select_row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        self.gpu_dropdown = Gtk.DropDown.new_from_strings([])
        self.gpu_dropdown.set_size_request(250, -1)
        self.gpu_dropdown.set_valign(Gtk.Align.CENTER)
        self.gpu_dropdown.connect("notify::selected", self.on_gpu_selected)
        select_row.append(self.gpu_dropdown)
        detect = Gtk.Button(label="Detect Zones", valign=Gtk.Align.CENTER)
        detect.connect("clicked", self.on_detect_clicked)
        select_row.append(detect)
        select_card.append(select_row)
        top.append(select_card)

        info_card = make_card()
        info_card.append(make_label("GPU Information", "fe-heading", margin_bottom=4))
        self.info_name = make_label("Name: ")
        self.info_name.add_css_class("fe-heading")
        self.info_details = make_label("Details: ", "fe-secondary")
        self.info_pci = make_label("PCI: ", "fe-secondary")
        self.info_uuid = make_label("UUID: ", "fe-secondary")
        self.info_driver = make_label("Driver Version: ", "fe-secondary")
        self.info_library = make_label("NvAPI Library: ", "fe-secondary")
        for widget in (
            self.info_name,
            self.info_details,
            self.info_pci,
            self.info_uuid,
            self.info_driver,
            self.info_library,
        ):
            widget.set_wrap(True)
            widget.set_selectable(True)
            info_card.append(widget)
        top.append(info_card)

    def _build_zones_section(self, root):
        zones_card = make_card()
        zones_card.append(make_label("Illumination Zones", "fe-heading"))
        self.zones_box = Gtk.Box(
            orientation=Gtk.Orientation.VERTICAL, spacing=12, margin_top=6
        )
        zones_card.append(self.zones_box)
        self.apply_all_button = Gtk.Button(
            label="Apply All", halign=Gtk.Align.END, margin_top=6, sensitive=False
        )
        self.apply_all_button.connect("clicked", self.on_apply_all)
        zones_card.append(self.apply_all_button)
        root.append(zones_card)

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

    def zone_by_index(self, index):
        for zone in self.zones:
            if zone.index == index:
                return zone
        return None

    def populate_gpus(self):
        try:
            data = self.felight.list()
        except FelightError as exc:
            self.set_status("NvAPI initialization failed.")
            self.alert("Failed to initialize NvAPI", exc.message)
            self.gpus = []
            self.driver = ""
            self.library = ""
            return
        self.gpus = data.gpus
        self.driver = data.driver or "unknown"
        self.library = data.library or "libnvidia-api.so.1"
        names = [f"{g.index}: {g.name}" for g in self.gpus]
        self.gpu_dropdown.set_model(Gtk.StringList.new(names))
        if self.gpus:
            self.gpu_dropdown.set_selected(0)
            self.refresh_gpu_info(0)
            self.set_status(f"Detected {len(self.gpus)} GPU(s).")
        else:
            self.set_status("No GPUs detected.")

    def refresh_gpu_info(self, index):
        self.current_gpu = index
        gpu = self.gpus[index]
        self.info_name.set_text("Name: " + gpu.name)
        if gpu.rt_cores is not None:
            external = "Yes" if gpu.external else "No"
            self.info_details.set_text(
                f"Details: Ray Tracing Cores: {gpu.rt_cores}, Tensor Cores: {gpu.tensor_cores}, isExternal GPU: {external}"
            )
        else:
            self.info_details.set_text("Details: not available")
        if gpu.bus_id is not None:
            self.info_pci.set_text(
                f"PCI: bus {gpu.bus_id}, device {gpu.device_id}, subsystem {gpu.subsystem_id}"
            )
        else:
            self.info_pci.set_text("PCI: not available")
        self.info_uuid.set_text("UUID: " + (gpu.uuid or "not available"))
        self.info_driver.set_text("Driver Version: " + self.driver)
        self.info_library.set_text("NvAPI Library: " + self.library)
        self.set_status(f"Loaded GPU {index}")

    def on_gpu_selected(self, dropdown, _pspec):
        if self.initializing or not self.gpus:
            return
        index = dropdown.get_selected()
        if index == Gtk.INVALID_LIST_POSITION:
            self.set_status("No GPU selected.")
            return
        self.refresh_gpu_info(index)
        self.clear_zones()

    def clear_zones(self):
        child = self.zones_box.get_first_child()
        while child is not None:
            nxt = child.get_next_sibling()
            self.zones_box.remove(child)
            child = nxt
        self.zones = []
        self.zone_cards = {}
        self.sliders.clear()
        self.apply_all_button.set_sensitive(False)

    def on_detect_clicked(self, _button):
        if not self.gpus:
            self.set_status("Select a GPU to detect zones.")
            return
        self.set_status(f"Detecting zones on GPU {self.current_gpu}...")
        self.populate_zones(self.current_gpu)

    def populate_zones(self, gpu_index):
        self.clear_zones()
        try:
            data = self.felight.get(gpu_index)
        except FelightError as exc:
            self.set_status("Failed to read illumination zones.")
            self.alert("Failed to read illumination zones", exc.message)
            return
        self.zones = data.zones
        if not self.zones:
            self.zones_box.append(
                make_label("No illumination zones found.", margin_start=35)
            )
            self.set_status("No illumination zones found.")
            return
        for zone in self.zones:
            card = ZoneCard(zone, gpu_index, self)
            self.zone_cards[zone.index] = card
            self.zones_box.append(card)
        self.apply_all_button.set_sensitive(True)
        self.set_status(f"Found {len(self.zones)} illumination zone(s).")

    def refresh_zone_labels(self, zone_index):
        """Rewrite a card's header and "Active" line from the cache after a successful write."""
        zone = self.zone_by_index(zone_index)
        card = self.zone_cards.get(zone_index)
        if zone is None:
            return
        if card is not None:
            card.refresh_labels(zone)

    def after_zone_write(self, gpu_index, zone_index, was_piecewise):
        """Update the card once a write succeeded. A zone that was animated (piecewise) is now in
        manual mode, so its whole card changes shape: re-detect instead of patching it."""
        if not was_piecewise:
            self.refresh_zone_labels(zone_index)
            return
        status = self.status_label.get_text()

        def redetect():
            self.populate_zones(gpu_index)
            self.set_status(status)
            return GLib.SOURCE_REMOVE

        GLib.idle_add(redetect)

    def on_color_changed(self, picker, _pspec, gpu_index, zone_index):
        if self.initializing:
            return
        zone = self.zone_by_index(zone_index)
        if zone is None:
            return
        r, g, b = bytes_from_rgba(picker.get_rgba())
        was_piecewise = zone.piecewise is not None
        try:
            self.felight.set(gpu_index, zone_index, rgb=(r, g, b))
        except FelightError as exc:
            self.alert(f"Failed to set color for zone {zone_index}", exc.message)
            return
        zone.r, zone.g, zone.b = r, g, b
        zone.ctrl_mode = "manual"
        self.set_status(
            f"Set {'RGBW' if zone.has_white else 'RGB'} color ({r}, {g}, {b}) on zone {zone_index}"
        )
        self.after_zone_write(gpu_index, zone_index, was_piecewise)

    def on_slider_changed(self, scale, gpu_index, zone_index, field):
        if self.initializing:
            return
        self.sliders.changed(scale, gpu_index, zone_index, field)

    def apply_slider(self, scale, gpu_index, zone_index, field):
        zone = self.zone_by_index(zone_index)
        if zone is None:
            return GLib.SOURCE_REMOVE
        value = round(scale.get_value())
        was_piecewise = zone.piecewise is not None
        try:
            if field == "white":
                self.felight.set(gpu_index, zone_index, white=value)
                zone.w = value
                self.set_status(f"Set white level {value} on zone {zone_index}")
            else:
                self.felight.set(gpu_index, zone_index, brightness=value)
                zone.brightness = value
                self.set_status(f"Set brightness {value}% on zone {zone_index}")
            zone.ctrl_mode = "manual"
        except FelightError as exc:
            self.alert(f"Failed to set {field} for zone {zone_index}", exc.message)
            return GLib.SOURCE_REMOVE
        self.after_zone_write(gpu_index, zone_index, was_piecewise)
        return GLib.SOURCE_REMOVE

    def on_apply_all(self, _button):
        """Re-send every cached zone value; report zones that fail instead of claiming success."""
        if not self.zones:
            return
        failures = []
        written = []
        any_piecewise = False
        for zone in self.zones:
            if not zone.controllable:
                continue
            kwargs = {"brightness": zone.brightness}
            if zone.has_color:
                kwargs["rgb"] = (zone.r, zone.g, zone.b)
            if zone.has_white:
                kwargs["white"] = zone.w
            try:
                self.felight.set(self.current_gpu, zone.index, **kwargs)
            except FelightError as exc:
                failures.append(f"zone {zone.index}: {exc.message}")
                continue
            any_piecewise = any_piecewise or zone.piecewise is not None
            zone.ctrl_mode = "manual"
            written.append(zone.index)
        if failures:
            self.set_status(f"Apply All: {len(failures)} zone(s) failed.")
            self.alert("Apply All finished with errors", "\n".join(failures))
        else:
            self.set_status("Applied settings to all zones.")
        if any_piecewise:
            self.after_zone_write(self.current_gpu, None, True)
        else:
            for index in written:
                self.refresh_zone_labels(index)
