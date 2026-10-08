"""Data types of the test runner."""

from collections.abc import Callable
from dataclasses import dataclass, field


@dataclass(frozen=True)
class Felight:
    """Runs felight with arguments."""

    args: tuple[str, ...]
    user: bool = False


@dataclass(frozen=True)
class Order:
    """Changes the order in which the mock enumerates cards."""

    cards: tuple[int, ...]


@dataclass
class Case:
    """Steps run one after another in a new work directory on one rig."""

    rig: str
    steps: list[Felight | Order]
    files: dict[str, bytes] = field(default_factory=dict)
    sandbox: bool = False


@dataclass
class Run:
    args: tuple[str, ...]
    exit_code: int
    stdout: str
    stderr: str


@dataclass
class Outcome:
    """Each felight run of a case, and every file the case left behind."""

    runs: list[Run]
    files: dict[str, str]


@dataclass
class Test:
    name: str
    run: Callable[[], None]


class Failure(Exception):
    """The test failed; the message says how."""


class Skip(Exception):
    """The test cannot run here; the message says why."""


def felight(*args: str, user: bool = False) -> Felight:
    return Felight(tuple(args), user)
