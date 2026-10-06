"""felight command-line wrapper"""

import json
import os
import shutil
import subprocess

from .model import Gpu, GpuList


class FelightError(Exception):
    def __init__(self, code, message):
        super().__init__(message)
        self.code = code
        self.message = message


def find_binary(script_dir):
    env = os.environ.get("FELIGHT_BIN")
    if env and os.access(env, os.X_OK):
        return env
    local = os.path.join(script_dir, "felight")
    if os.access(local, os.X_OK):
        return local
    found = shutil.which("felight")
    if found:
        return found
    return None


class Felight:
    """Thin synchronous wrapper. Every call spawns the CLI; calls take well under a second."""

    def __init__(self, binary):
        self.binary = binary

    def run(self, *args, check=True):
        try:
            proc = subprocess.run(
                [self.binary, *args],
                capture_output=True,
                text=True,
                timeout=60,
                check=False,
            )
        except (OSError, subprocess.TimeoutExpired) as exc:
            raise FelightError(-1, f"could not run felight: {exc}") from exc
        if check and proc.returncode != 0:
            message = (
                proc.stderr.strip()
                or proc.stdout.strip()
                or f"felight exited with code {proc.returncode}"
            )
            raise FelightError(proc.returncode, message)
        return proc

    def list(self):
        return GpuList.from_json(json.loads(self.run("list", "--json").stdout))

    def get(self, gpu):
        return Gpu.from_json(
            json.loads(self.run("get", "--gpu", str(gpu), "--json").stdout)
        )

    def set(self, gpu, zone, rgb=None, white=None, brightness=None):
        args = ["set", "--gpu", str(gpu), "--zone", str(zone)]
        if rgb is not None:
            args += ["--rgb", ",".join(map(str, rgb))]
        if white is not None:
            args += ["--white", str(white)]
        if brightness is not None:
            args += ["--brightness", str(brightness)]
        self.run(*args)

    def save(self, gpu, path, delay=None):
        args = ["save", "--gpu", str(gpu)]
        if delay is not None:
            args += ["--delay", str(delay)]
        self.run(*args, path)

    def apply(self, gpu, path):
        """Apply a profile to the given GPU. Returns (exit code, output); 6 is a partial apply."""
        proc = self.run(
            "apply", "--gpu", str(gpu), "--no-verify-gpu", path, check=False
        )
        if proc.returncode not in (0, 6):
            raise FelightError(
                proc.returncode, proc.stderr.strip() or proc.stdout.strip()
            )
        return proc.returncode, proc.stdout
