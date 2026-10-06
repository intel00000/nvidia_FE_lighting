"""Slider write scheduling: write on pointer release during a drag, debounce keyboard/scroll changes."""

from gi.repository import Gdk, GLib, Gtk

from .config import SLIDER_DEBOUNCE_MS


class SliderWriter:
    """Decides when a slider value is written."""

    def __init__(self, write):
        self.write = write
        self.pending_timers = {}  # (zone, field) -> GLib source id
        self.dragging = (
            set()
        )  # (zone, field) of sliders currently held with the pointer
        self.dirty = {}  # (zone, field) -> (scale, gpu, zone, field) changed during a drag

    def track(self, scale, key):
        """Write a slider only when the pointer releases."""
        controller = Gtk.EventControllerLegacy()
        controller.set_propagation_phase(Gtk.PropagationPhase.CAPTURE)
        controller.connect("event", self.on_slider_event, key)
        scale.add_controller(controller)

    def on_slider_event(self, controller, _event, key):
        event = controller.get_current_event()
        if event is None:
            return Gdk.EVENT_PROPAGATE
        kind = event.get_event_type()
        if kind in (Gdk.EventType.BUTTON_PRESS, Gdk.EventType.TOUCH_BEGIN):
            self.dragging.add(key)
        elif kind in (
            Gdk.EventType.BUTTON_RELEASE,
            Gdk.EventType.TOUCH_END,
            Gdk.EventType.TOUCH_CANCEL,
            Gdk.EventType.GRAB_BROKEN,
        ):
            self.on_slider_released(key)
        return Gdk.EVENT_PROPAGATE

    def on_slider_released(self, key):
        self.dragging.discard(key)
        pending = self.dirty.pop(key, None)
        if pending is not None:
            if key in self.pending_timers:
                GLib.source_remove(self.pending_timers.pop(key))
            self._fire(*pending)

    def changed(self, scale, gpu_index, zone_index, field):
        """During a pointer drag the write waits for the release; keyboard and scroll-wheel
        changes are written once the value settles."""
        key = (zone_index, field)
        if key in self.pending_timers:
            GLib.source_remove(self.pending_timers.pop(key))
        if key in self.dragging:
            self.dirty[key] = (scale, gpu_index, zone_index, field)
            # Safety net in case the release event never reaches.
            self.pending_timers[key] = GLib.timeout_add(
                1500, self._fire, scale, gpu_index, zone_index, field
            )
            return
        self.pending_timers[key] = GLib.timeout_add(
            SLIDER_DEBOUNCE_MS, self._fire, scale, gpu_index, zone_index, field
        )

    def clear(self):
        for source in self.pending_timers.values():
            GLib.source_remove(source)
        self.pending_timers = {}
        self.dragging = set()
        self.dirty = {}

    def _fire(self, scale, gpu_index, zone_index, field):
        self.pending_timers.pop((zone_index, field), None)
        self.dirty.pop((zone_index, field), None)
        return self.write(scale, gpu_index, zone_index, field)
