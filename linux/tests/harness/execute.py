"""Runs test builds of felight on the mock NvAPI library. A test build links
tests/shim/wrap.c with -Wl,--wrap=dlopen, so it can only load the mock; any other
binary is refused before it runs."""

import functools
import shutil
import subprocess
from pathlib import Path

from . import rig, sandbox
from .context import Context
from .model import Case, Failure, Felight, Order, Outcome, Run, Skip
from .text import decode, normalize

RIG = "rig"
LOG = "nvapi.log"
LEFT_OUT = {f"{sandbox.ROOT}/etc/ld.so.cache", f"{sandbox.FAKE_BIN}/systemctl"}


@functools.cache
def symbols(binary: Path) -> frozenset[str]:
    proc = subprocess.run(["nm", binary], capture_output=True, text=True, check=False)
    lines = proc.stdout.splitlines()
    return frozenset(line.split()[-1].split("@")[0] for line in lines if line.strip())


def require_wrapped(binary: Path) -> None:
    """Refuses any binary that could load the driver's libnvidia-api.so.1."""
    if not binary.is_file():
        raise Failure(f"{binary} is missing; run make -C linux check")
    if "__wrap_dlopen" not in symbols(binary):
        raise Failure(f"{binary} has no __wrap_dlopen: not a test build, not run")


def environment(ctx: Context, work: Path) -> dict[str, str]:
    return {
        "PATH": "/usr/bin:/bin",
        "LC_ALL": "C",
        "TZ": "UTC",
        "HOME": str(work),
        "MOCK_NVAPI_LIB": str(ctx.mock_lib),
        "MOCK_NVAPI_STATE": str(work / RIG),
        "MOCK_NVAPI_LOG": str(work / LOG),
    }


def placeholders(ctx: Context, binary: Path, work: Path) -> dict[str, str]:
    return {str(work): "WORK", str(ctx.mock_lib): "MOCK_LIB", str(binary): "FELIGHT"}


def execute(
    argv: list[str], env: dict[str, str], work: Path
) -> subprocess.CompletedProcess:
    return subprocess.run(
        argv,
        cwd=work,
        env=env,
        stdin=subprocess.DEVNULL,
        capture_output=True,
        timeout=60,
        check=False,
    )


def run_felight(
    ctx: Context, binary: Path, step: Felight, work: Path, in_sandbox: bool
) -> Run:
    require_wrapped(binary)
    argv = [str(binary), *step.args]
    env = environment(ctx, work)
    if in_sandbox:
        argv = sandbox.command(argv, work, ctx.build, step.user)
        env.update(sandbox.environment(work))
    proc = execute(argv, env, work)
    names = placeholders(ctx, binary, work)
    return Run(
        step.args,
        proc.returncode,
        normalize(decode(proc.stdout), names),
        normalize(decode(proc.stderr), names),
    )


def prepare(ctx: Context, case: Case, work: Path) -> None:
    if case.sandbox:
        reason = sandbox.unavailable(ctx.build)
        if reason:
            raise Skip(reason)
    if work.exists():
        shutil.rmtree(work)
    work.mkdir(parents=True)
    shutil.copyfile(ctx.rig(case.rig), work / RIG)
    for name, data in case.files.items():
        path = work / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
    if case.sandbox:
        sandbox.prepare(work)


@functools.cache
def contents(binary: Path) -> bytes:
    return binary.read_bytes()


def collect(ctx: Context, binary: Path, work: Path) -> dict[str, str]:
    """Every file and directory the case left behind, normalized."""
    names = placeholders(ctx, binary, work)
    program = contents(binary)
    files = {}
    for path in sorted(work.rglob("*")):
        name = path.relative_to(work).as_posix()
        if name in LEFT_OUT:
            continue
        if path.is_dir():
            files[name + "/"] = ""
            continue
        data = path.read_bytes()
        files[name] = "(felight)" if data == program else normalize(decode(data), names)
    return files


def run_case(ctx: Context, binary: Path, case: Case, work: Path) -> Outcome:
    require_wrapped(binary)
    prepare(ctx, case, work)
    runs = []
    for step in case.steps:
        if isinstance(step, Order):
            rig.set_order(work / RIG, step.cards)
        else:
            runs.append(run_felight(ctx, binary, step, work, case.sandbox))
    return Outcome(runs, collect(ctx, binary, work))
