"""Runs felight's tests on the test builds that make -C linux check puts in BUILD."""

import argparse
import os
import resource
import shutil
import sys
from pathlib import Path

from harness.context import Context
from harness.runner import run_tests
from suites import smoke

SUITES = {"smoke": smoke}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path, help="directory with the test builds")
    parser.add_argument("suites", nargs="*", help=f"suites to run: {', '.join(SUITES)}")
    parser.add_argument(
        "-k", "--keyword", default="", help="only tests with this in their name"
    )
    parser.add_argument("-j", "--jobs", type=int, default=os.cpu_count() or 1)
    parser.add_argument("-v", "--verbose", action="store_true", help="list every test")
    parser.add_argument("--strict", action="store_true", help="fail on skipped tests")
    args = parser.parse_args()
    unknown = [s for s in args.suites if s not in SUITES]
    if unknown:
        parser.error(f"unknown suite {', '.join(unknown)}")
    # some tests make a test build abort on purpose; they leave no core dumps
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    ctx = Context(args.build.resolve())
    if ctx.work.exists():
        shutil.rmtree(ctx.work)
    tests = [t for name in args.suites or SUITES for t in SUITES[name].tests(ctx)]
    tests = [t for t in tests if args.keyword in t.name]
    return run_tests(tests, args.jobs, args.strict, args.verbose)


if __name__ == "__main__":
    sys.exit(main())
