#!/usr/bin/env python3
"""Runs the strict checks on just the translation units you edited.

A full `cmake --build build-strict` re-analyzes every file that includes a
changed header: 372 translation units for sdllib/pixel_buffer.h, about four
minutes at -j14. While a file is still being modernized that is the wrong
trade, so this script builds only the objects belonging to the sources you
name -- the same clang-tidy pass and clang compile ninja would run, with the
same flags, ccache and clang-tidy cache -- and leaves the tree-wide pass for
the end of the run.

It proves the files you touched are clean. It says nothing about the other
includers of a header you changed, so run the full strict build before the
last commit.

Usage:
  tools/strict_tu.py src/ra/techno.cc src/ra/techno.h
  tools/strict_tu.py --build-dir cmake-build-strict-ra-clang src/td/init.cc

A header is checked through its sibling .cc, so passing the pair is the same
as passing the source alone. Name the other sources to check by hand when a
header change reaches files you care about.
"""

import argparse
import os
import re
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent


def ninja_program(build_dir: Path) -> str:
    """Returns the ninja that configured build_dir.

    Ninja versions hash commands differently, so building a directory with
    another ninja marks every object dirty and rebuilds the tree.
    """
    cache = build_dir / "CMakeCache.txt"
    match = re.search(
        r"^CMAKE_MAKE_PROGRAM:\w+=(.+)$", cache.read_text(), re.MULTILINE
    )
    if not match:
        sys.exit(f"{cache} has no CMAKE_MAKE_PROGRAM")
    return match.group(1)


def object_targets(ninja: str, build_dir: Path, source: Path) -> list[str]:
    """Returns the ninja object targets built from source, which may be none."""
    query = subprocess.run(
        [ninja, "-C", str(build_dir), "-t", "query", str(source)],
        capture_output=True,
        text=True,
    )
    return [
        line.strip()
        for line in query.stdout.splitlines()
        if line.strip().endswith(".o")
    ]


def sources_to_check(paths: list[str]) -> list[Path]:
    """Maps the given files to the sources that carry them through the compiler.

    A header has no object of its own; its sibling .cc stands in for it.
    """
    sources: list[Path] = []
    for path in paths:
        resolved = Path(path).resolve()
        if resolved.suffix in (".h", ".hpp"):
            resolved = resolved.with_suffix(".cc")
            if not resolved.exists():
                print(f"note: {path} has no sibling .cc, skipping", file=sys.stderr)
                continue
        if not resolved.exists():
            sys.exit(f"{path} does not exist")
        if resolved not in sources:
            sources.append(resolved)
    return sources


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--build-dir",
        default=os.environ.get("STRICT_DIR", "build-strict"),
        help="strict build directory (default: build-strict, or $STRICT_DIR)",
    )
    parser.add_argument(
        "--parallel",
        type=int,
        default=8,
        help="ninja jobs; each clang-tidy job peaks near 1 GB (default: 8)",
    )
    parser.add_argument("files", nargs="+", help="the sources and headers you edited")
    args = parser.parse_args()

    build_dir = Path(args.build_dir)
    if not build_dir.is_absolute():
        build_dir = REPO_ROOT / build_dir
    if not (build_dir / "CMakeCache.txt").exists():
        sys.exit(f"{build_dir} is not a configured build directory")

    ninja = ninja_program(build_dir)
    targets: list[str] = []
    for source in sources_to_check(args.files):
        found = object_targets(ninja, build_dir, source)
        if not found:
            print(
                f"note: {source.relative_to(REPO_ROOT)} builds no object in "
                f"{build_dir.name}, skipping",
                file=sys.stderr,
            )
        for target in found:
            if target not in targets:
                targets.append(target)

    if not targets:
        sys.exit("nothing to check")

    for target in targets:
        print(f"checking {target}")
    return subprocess.run(
        [ninja, "-C", str(build_dir), f"-j{args.parallel}", *targets]
    ).returncode


if __name__ == "__main__":
    sys.exit(main())
