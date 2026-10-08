"""Checks of the test machinery: test builds load nothing but the mock NvAPI library,
the mock serves its rig, records writes and injects faults, and the sandbox keeps the
service's files in the work directory."""

from collections.abc import Callable

from harness import execute, rig, sandbox
from harness.context import Context
from harness.model import Case, Failure, Felight, Order, Outcome, Test, felight
from harness.text import render


def expect(condition: bool, what: str, outcome: Outcome | None = None) -> None:
    if not condition:
        raise Failure(what + ("\n" + render(outcome) if outcome else ""))


def run(ctx: Context, name: str, case: Case) -> Outcome:
    return execute.run_case(ctx, ctx.felight, case, ctx.work / "smoke" / name)


def raw_run(ctx: Context, name: str, env: dict[str, str]):
    """Runs felight list with a changed environment, outside execute.run_case."""
    execute.require_wrapped(ctx.felight)
    work = ctx.work / "smoke" / name
    work.mkdir(parents=True)
    full = execute.environment(ctx, work) | env
    proc = execute.execute([str(ctx.felight), "list"], full, work)
    return proc.returncode, proc.stderr.decode()


def refuses_other_binaries(ctx: Context) -> None:
    execute.require_wrapped(ctx.felight)
    try:
        execute.require_wrapped(ctx.mock_lib)
    except Failure:
        return
    raise Failure("require_wrapped accepted a binary without __wrap_dlopen")


def aborts_without_the_mock(ctx: Context) -> None:
    code, err = raw_run(ctx, "no_mock", {"MOCK_NVAPI_LIB": ""})
    expect(code == -6 and "MOCK_NVAPI_LIB is not set" in err, f"exit {code}: {err}")


def refuses_other_libraries(ctx: Context) -> None:
    other = ctx.work / "smoke" / "other.so"
    other.parent.mkdir(parents=True, exist_ok=True)
    other.write_bytes(b"\x7fELF, but not the mock")
    code, err = raw_run(ctx, "other", {"MOCK_NVAPI_LIB": str(other)})
    expect(code == -6 and "is not the mock NvAPI library" in err, f"exit {code}: {err}")
    code, err = raw_run(ctx, "relative", {"MOCK_NVAPI_LIB": "libnvidia-api.so.1"})
    expect(code == -6 and "must be an absolute path" in err, f"exit {code}: {err}")


def missing_library(ctx: Context) -> None:
    code, err = raw_run(ctx, "missing", {"MOCK_NVAPI_LIB": "/nonexistent/lib.so"})
    expect(code == 2 and "cannot load libnvidia-api.so.1" in err, f"exit {code}: {err}")


def serves_the_rig(ctx: Context) -> None:
    out = run(ctx, "list", Case("fe5090", [felight("list")]))
    text = out.runs[0].stdout
    gpu = "GPU 0: NVIDIA GeForce RTX 5090  (bus 1, device 0x2b8510de, subsystem 0x205710de)"
    expect(
        gpu + "  GPU-3f6c1a2e-8d4b-4c9a-b1e7-5a0d9c2f4e81\n" in text, "GPU line", out
    )
    for zone, where, brightness in (
        (0, "Front", 50),
        (1, "Top  ", 60),
        (2, "Back ", 60),
    ):
        line = f"zone {zone}: Single Color @ GPU {where} mode=manual"
        expect(f"{line}           brightness={brightness}%\n" in text, line, out)


def records_writes(ctx: Context) -> None:
    out = run(
        ctx,
        "set",
        Case(
            "fe5090",
            [felight("set", "--gpu", "0", "--zone", "1", "--brightness", "40")],
        ),
    )
    state = rig.zones(ctx.work / "smoke/set" / execute.RIG)
    expect(
        out.runs[0].stdout == "ok\n" and state[0][1]["br"] == "40", "zone 1 at 40", out
    )
    expect(
        out.files[execute.LOG] == "Initialize\nSetControl card=0 zones=1\n", "log", out
    )


def injects_write_faults(ctx: Context) -> None:
    steps: list[Felight | Order] = [
        felight("set", "--gpu", "0", "--zone", z, "--brightness", "5")
        for z in ("1", "2")
    ]
    out = run(ctx, "faults", Case("faults", steps))
    state = rig.zones(ctx.work / "smoke/faults" / execute.RIG)
    rejected, ignored = out.runs
    expect(
        rejected.exit_code == 3 and "SetControl failed: NVAPI_ERROR" in rejected.stderr,
        "rejected",
        out,
    )
    expect(
        ignored.exit_code == 5 and "zone 2 read-back mismatch" in ignored.stderr,
        "ignored",
        out,
    )
    expect(
        state[0][1]["br"] == state[0][2]["br"] == "60", "zones 1 and 2 unchanged", out
    )
    expect("SetControl card=0 zones=1 failed\n" in out.files[execute.LOG], "log", out)


def card_without_lighting(ctx: Context) -> None:
    out = run(
        ctx, "nolight", Case("nolight", [felight("list"), felight("get", "--gpu", "0")])
    )
    listed, got = out.runs
    expect(
        listed.exit_code == 0 and "illumination zones unavailable" in listed.stderr,
        "list",
        out,
    )
    expect(got.exit_code == 3 and "NVAPI_NOT_SUPPORTED" in got.stderr, "get", out)


def save_and_apply(ctx: Context) -> None:
    steps: list[Felight | Order] = [
        felight("save", "--gpu", "0", "p.conf"),
        felight("set", "--gpu", "0", "--zone", "0", "--brightness", "10"),
        felight("apply", "p.conf"),
    ]
    out = run(ctx, "save_apply", Case("fe5090", steps))
    state = rig.zones(ctx.work / "smoke/save_apply" / execute.RIG)
    expect(out.runs[2].stdout.endswith("applied 3/3 zone(s).\n"), "applied", out)
    expect([z["br"] for z in state[0]] == ["50", "60", "60"], "zones restored", out)


def enumeration_order(ctx: Context) -> None:
    out = run(ctx, "order", Case("mock3", [Order((1, 0)), felight("list")]))
    text = out.runs[0].stdout
    expect(
        "GPU 0: NVIDIA GeForce RTX 4090" in text
        and "GPU 1: NVIDIA GeForce RTX 5090" in text,
        "order",
        out,
    )
    expect("A2000" not in text, "the A2000 is not enumerated", out)


def startup_waits_and_retries(ctx: Context) -> None:
    profile = b"delay_seconds=7\nzone 0 single_color brightness=20\n"
    case = Case("flaky", [felight("startup", "p.conf")], {"p.conf": profile})
    out = run(ctx, "startup", case)
    log = (
        "sleep 7\nInitialize failed\nsleep 2\nInitialize failed\nsleep 2\nInitialize\n"
    )
    expect(
        out.runs[0].exit_code == 0 and out.files[execute.LOG].startswith(log),
        "log",
        out,
    )


def sandbox_keeps_service_files(ctx: Context) -> None:
    out = run(
        ctx,
        "sandbox",
        Case("fe5090", [felight("service", "enable", "--gpu", "0")], sandbox=True),
    )
    files = out.files
    root = sandbox.ROOT
    expect(out.runs[0].exit_code == 0, "service enable", out)
    expect(
        files[f"{root}/usr/local/lib/nvidia-fe-lighting/felight"] == "(felight)",
        "binary",
        out,
    )
    expect(f"{root}/etc/nvidia-fe-lighting/startup.conf" in files, "settings", out)
    expect(
        f"{root}/etc/systemd/system/nvidia-fe-lighting.service" in files, "unit", out
    )
    calls = "systemctl daemon-reload\nsystemctl enable nvidia-fe-lighting.service\n"
    expect(files[sandbox.SYSTEMCTL_LOG] == calls, "systemctl calls", out)


CHECKS: list[Callable[[Context], None]] = [
    refuses_other_binaries,
    aborts_without_the_mock,
    refuses_other_libraries,
    missing_library,
    serves_the_rig,
    records_writes,
    injects_write_faults,
    card_without_lighting,
    save_and_apply,
    enumeration_order,
    startup_waits_and_retries,
    sandbox_keeps_service_files,
]


def tests(ctx: Context) -> list[Test]:
    return [Test(f"smoke/{check.__name__}", lambda c=check: c(ctx)) for check in CHECKS]
