"""Data classes for the JSON printed by `felight list --json` and `felight get --json`."""

from __future__ import annotations

from dataclasses import dataclass


@dataclass
class Endpoint:
    """One of the two color/brightness endpoints of a piecewise-linear zone."""

    r: int
    g: int
    b: int
    w: int
    brightness: int

    @classmethod
    def from_json(cls, data):
        return cls(
            r=data["r"],
            g=data["g"],
            b=data["b"],
            w=data["w"],
            brightness=data["brightness"],
        )


@dataclass
class Piecewise:
    """The animation of a zone in piecewise-linear mode."""

    cycle: str
    group_count: int
    rise_ms: int
    fall_ms: int
    a_ms: int
    b_ms: int
    idle_ms: int
    phase_offset_ms: int
    endpoints: list[Endpoint]

    @classmethod
    def from_json(cls, data):
        return cls(
            cycle=data["cycle"],
            group_count=data["group_count"],
            rise_ms=data["rise_ms"],
            fall_ms=data["fall_ms"],
            a_ms=data["a_ms"],
            b_ms=data["b_ms"],
            idle_ms=data["idle_ms"],
            phase_offset_ms=data["phase_offset_ms"],
            endpoints=[Endpoint.from_json(ep) for ep in data.get("endpoints", [])],
        )


@dataclass
class Zone:
    """One illumination zone. Mutable: the window updates it after every successful write."""

    index: int
    type: str
    type_label: str
    location: str
    location_label: str
    present_in_info: bool
    present_in_control: bool
    device_index: int | None
    ctrl_mode: str
    ctrl_mode_mask: int | None
    has_color: bool
    has_white: bool
    r: int
    g: int
    b: int
    w: int
    brightness: int
    piecewise: Piecewise | None = None

    @property
    def controllable(self):
        return (
            self.type != "invalid"
            and self.present_in_control
            and self.ctrl_mode != "unknown"
        )

    @classmethod
    def from_json(cls, data):
        pw = data.get("piecewise")
        return cls(
            index=data["index"],
            type=data["type"],
            type_label=data["type_label"],
            location=data["location"],
            location_label=data["location_label"],
            present_in_info=data["present_in_info"],
            present_in_control=data["present_in_control"],
            device_index=data["device_index"],
            ctrl_mode=data["ctrl_mode"],
            ctrl_mode_mask=data["ctrl_mode_mask"],
            has_color=data["has_color"],
            has_white=data["has_white"],
            r=data["r"],
            g=data["g"],
            b=data["b"],
            w=data["w"],
            brightness=data["brightness"],
            piecewise=Piecewise.from_json(pw) if pw else None,
        )


@dataclass
class Gpu:
    """One GPU. The optional fields are None when felight could not read them."""

    index: int
    name: str
    zones: list[Zone]
    error: str | None = None
    bus_id: int | None = None
    device_id: str | None = None
    subsystem_id: str | None = None
    revision_id: str | None = None
    ext_device_id: str | None = None
    uuid: str | None = None
    rt_cores: int | None = None
    tensor_cores: int | None = None
    external: bool | None = None

    @classmethod
    def from_json(cls, data):
        return cls(
            index=data["index"],
            name=data["name"],
            zones=[Zone.from_json(zone) for zone in data.get("zones", [])],
            error=data.get("error"),
            bus_id=data.get("bus_id"),
            device_id=data.get("device_id"),
            subsystem_id=data.get("subsystem_id"),
            revision_id=data.get("revision_id"),
            ext_device_id=data.get("ext_device_id"),
            uuid=data.get("uuid"),
            rt_cores=data.get("rt_cores"),
            tensor_cores=data.get("tensor_cores"),
            external=data.get("external"),
        )


@dataclass
class GpuList:
    """`felight list --json`: the library and driver strings are kept as printed (may be empty)."""

    library: str | None
    driver: str | None
    gpus: list[Gpu]

    @classmethod
    def from_json(cls, data):
        return cls(
            library=data.get("library"),
            driver=data.get("driver"),
            gpus=[Gpu.from_json(gpu) for gpu in data.get("gpus", [])],
        )
