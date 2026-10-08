"""Runs tests in parallel and reports the results."""

import subprocess
import traceback
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass

from .model import Failure, Skip, Test

WORDS = [("ok", "passed"), ("fail", "failed"), ("skip", "skipped")]


@dataclass
class Result:
    name: str
    status: str
    detail: str = ""


def run_one(test: Test) -> Result:
    try:
        test.run()
    except Skip as skip:
        return Result(test.name, "skip", str(skip))
    except Failure as failure:
        return Result(test.name, "fail", str(failure))
    except (OSError, LookupError, ValueError, subprocess.SubprocessError):
        return Result(test.name, "fail", traceback.format_exc())
    return Result(test.name, "ok")


def report(results: list[Result], strict: bool, verbose: bool) -> int:
    for r in results:
        if r.status == "fail":
            print(f"FAIL {r.name}\n{r.detail.rstrip()}\n")
        elif verbose:
            print(f"{r.status} {r.name}")
    by_suite: dict[str, Counter] = {}
    for r in results:
        by_suite.setdefault(r.name.split("/")[0], Counter())[r.status] += 1
    for suite, counts in by_suite.items():
        parts = [f"{counts[s]} {w}" for s, w in WORDS if counts[s]]
        print(f"{suite}: {', '.join(parts)}")
    for reason, count in Counter(
        r.detail for r in results if r.status == "skip"
    ).items():
        print(f"skipped {count} test(s): {reason}")
    failed = any(r.status == "fail" for r in results)
    skipped = any(r.status == "skip" for r in results)
    if strict and skipped:
        print("--strict: skipped tests count as failures")
    return 1 if failed or (strict and skipped) else 0


def run_tests(tests: list[Test], jobs: int, strict: bool, verbose: bool) -> int:
    with ThreadPoolExecutor(max_workers=jobs) as pool:
        results = list(pool.map(run_one, tests))
    return report(results, strict, verbose)
