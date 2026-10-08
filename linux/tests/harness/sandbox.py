"""A bubblewrap sandbox for the service tests: felight runs as root (or as a user) in a
user namespace where /etc and /usr/local/lib are directories of the case's work
directory, everything else is read-only, and systemctl is a script that logs its
arguments."""

import functools
import shutil
import subprocess
import tempfile
from pathlib import Path

ROOT = "root"
FAKE_BIN = "fakebin"
SYSTEMCTL_LOG = "systemctl.log"
SYSTEMCTL = """#!/bin/sh
echo "systemctl $*" >> "$FAKE_SYSTEMCTL_LOG"
exit 0
"""


def prepare(work: Path) -> None:
    for d in ("etc/systemd/system", "usr/local/lib"):
        (work / ROOT / d).mkdir(parents=True)
    script = work / FAKE_BIN / "systemctl"
    script.parent.mkdir()
    script.write_text(SYSTEMCTL)
    script.chmod(0o755)


def command(argv: list[str], work: Path, build: Path, user: bool) -> list[str]:
    root = work / ROOT
    uid = "1000" if user else "0"
    return [
        "bwrap",
        "--unshare-all",
        "--die-with-parent",
        "--new-session",
        "--ro-bind", "/", "/",
        "--dev", "/dev",
        "--proc", "/proc",
        "--tmpfs", "/tmp",
        "--tmpfs", "/run",
        "--bind", str(root / "etc"), "/etc",
        "--ro-bind-try", "/etc/ld.so.cache", "/etc/ld.so.cache",
        "--bind", str(root / "usr/local/lib"), "/usr/local/lib",
        "--ro-bind", str(build), str(build),
        "--bind", str(work), str(work),
        "--uid", uid,
        "--gid", uid,
        "--chdir", str(work),
        *argv,
    ]  # fmt: skip


def environment(work: Path) -> dict[str, str]:
    return {
        "PATH": f"{work / FAKE_BIN}:/usr/bin:/bin",
        "FAKE_SYSTEMCTL_LOG": str(work / SYSTEMCTL_LOG),
    }


@functools.cache
def unavailable(build: Path) -> str | None:
    """Why the sandbox cannot run here, or None when it can."""
    if not shutil.which("bwrap"):
        return "bwrap (bubblewrap) is not installed"
    build.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=build) as tmp:
        work = Path(tmp)
        prepare(work)
        argv = command(["systemctl", "probe"], work, build, user=False)
        env = environment(work)
        proc = subprocess.run(
            argv, env=env, capture_output=True, text=True, timeout=30, check=False
        )
        if proc.returncode != 0 or not (work / SYSTEMCTL_LOG).exists():
            return f"bwrap cannot create the sandbox: {proc.stderr.strip()}"
    return None
