"""Local context for the test harness."""

from dataclasses import dataclass
from pathlib import Path

TESTS = Path(__file__).resolve().parent.parent


@dataclass(frozen=True)
class Context:
    build: Path

    @property
    def tests(self) -> Path:
        return TESTS

    @property
    def work(self) -> Path:
        return self.build / "work"

    @property
    def felight(self) -> Path:
        return self.build / "felight"

    @property
    def mock_lib(self) -> Path:
        return self.build / "libnvidia-api.so.1"

    def rig(self, name: str) -> Path:
        return TESTS / "rigs" / f"{name}.rig"
