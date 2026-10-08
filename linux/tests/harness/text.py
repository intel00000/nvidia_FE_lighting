"""Normalizing what felight prints and writes, so that runs compare across builds and
machines, and rendering outcomes for diffs."""

import difflib
import re

from .model import Outcome

PATTERNS = [
    (re.compile(r"\[\d{4}-\d\d-\d\d \d\d:\d\d:\d\d\]"), "[TIME]"),
    (
        re.compile(
            r"^# NVIDIA lighting profile written by felight \S+ on .*$", re.MULTILINE
        ),
        "# NVIDIA lighting profile written by felight VERSION on TIME",
    ),
    (
        re.compile(r"\bfelight \d+\.\d+\.\d+(?= startup mode| - |$)", re.MULTILINE),
        "felight VERSION",
    ),
    (re.compile(r"^driver \S+ via ", re.MULTILINE), "driver DRIVER via "),
    (re.compile(r'"driver":"[^"]*"'), '"driver":"DRIVER"'),
    (re.compile(r"^driver: .*$", re.MULTILINE), "driver: DRIVER"),
]
CONTROL = re.compile(r"[\x00-\x08\x0b-\x1f\x7f]")


def decode(data: bytes) -> str:
    return data.decode("utf-8", "backslashreplace")


def normalize(text: str, names: dict[str, str]) -> str:
    """Replaces each path in names by its placeholder, then times, versions and the
    driver version."""
    for path in sorted(names, key=len, reverse=True):
        text = text.replace(path, names[path])
    for pattern, placeholder in PATTERNS:
        text = pattern.sub(placeholder, text)
    return text


def lines(prefix: str, text: str) -> list[str]:
    """Shows each line with control characters escaped, and a missing last newline."""
    if not text:
        return []
    body = text.removesuffix("\n")
    out = []
    for line in body.split("\n"):
        if CONTROL.search(line):
            line = line.encode("unicode_escape").decode("ascii")
        out.append(f"{prefix}{line}")
    if not text.endswith("\n"):
        out.append(f"{prefix}(no newline at the end)")
    return out


def render(outcome: Outcome) -> str:
    """outcome as text, for diffs."""
    out = []
    for run in outcome.runs:
        out.append(f"$ felight {' '.join(run.args)}")
        out.append(f"exit {run.exit_code}")
        out += lines("stdout| ", run.stdout) + lines("stderr| ", run.stderr)
    for path, text in sorted(outcome.files.items()):
        out.append(f"== {path}")
        out += lines("| ", text)
    return "\n".join(out) + "\n"


def diff(a: str, b: str, a_name: str, b_name: str) -> str:
    lines = difflib.unified_diff(
        a.splitlines(keepends=True), b.splitlines(keepends=True), a_name, b_name
    )
    return "".join(lines)
