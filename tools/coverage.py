#!/usr/bin/env python3
"""Reports which lines and branches of the files you name the unit tests miss.

Builds the tree with clang's source-based coverage in its own build directory
(configured on first use), runs the gtest executables, and prints, for each
file, the line, function and branch totals followed by the uncovered lines
with their source text. Exits 0 only when every named file has every line
covered and every test passed.

Usage:
  tools/coverage.py src/ra/ai/carryover.cc src/ra/ai/carryover.h
  tools/coverage.py --tests 'ra_carryover|ra_reinforcements' src/ra/ai/carryover.cc
  tools/coverage.py --branches src/engine/gfx/palette.cc

A file that no test executable links is reported as "in no test binary": it
needs a test target before it can have coverage. A header with no executable
code (declarations only) is reported as such and counts as covered.

--tests narrows the run to the test executables whose name matches the regex,
for the edit loop. Coverage then counts only those tests, so finish with a run
without it: code another suite reaches is covered too.
"""

import argparse
import concurrent.futures
import json
import os
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
# Continuous mode writes the counters as they change, so the code a death test
# reaches counts although its child process aborts without flushing a profile.
COVERAGE_FLAGS = "-fprofile-instr-generate -fcoverage-mapping -fprofile-continuous"


def llvm_tool(name: str, clang: str) -> str:
    """Returns the llvm tool matching clang's version; profile formats differ."""
    version = subprocess.run(
        [clang, "-dumpversion"], capture_output=True, text=True, check=True
    ).stdout.split(".")[0]
    for candidate in (f"{name}-{version}", f"/usr/lib/llvm-{version}/bin/{name}", name):
        found = shutil.which(candidate)
        if found:
            return found
    sys.exit(f"{name} for clang {version} not found")


def cache_value(build_dir: Path, key: str) -> str:
    match = re.search(
        rf"^{key}:\w+=(.+)$", (build_dir / "CMakeCache.txt").read_text(), re.MULTILINE
    )
    if not match:
        sys.exit(f"{build_dir}/CMakeCache.txt has no {key}")
    return match.group(1)


def configure(build_dir: Path) -> None:
    """Configures an instrumented build; the checks are off, they add nothing here."""
    subprocess.run(
        [
            "cmake",
            f"-B{build_dir}",
            "-G",
            "Ninja",
            "-DSTRICT_CHECKS=OFF",
            "-DCMAKE_C_COMPILER=clang",
            "-DCMAKE_CXX_COMPILER=clang++",
            f"-DCMAKE_C_FLAGS={COVERAGE_FLAGS}",
            f"-DCMAKE_CXX_FLAGS={COVERAGE_FLAGS}",
        ],
        cwd=REPO_ROOT,
        check=True,
    )


@dataclass(frozen=True)
class TestBinary:
    path: str
    working_dir: str


def test_binaries(build_dir: Path) -> list[TestBinary]:
    """Returns the test executables ctest knows, each once."""
    listing = subprocess.run(
        ["ctest", "--test-dir", str(build_dir), "--show-only=json-v1"],
        capture_output=True,
        text=True,
        check=True,
    )
    binaries: dict[str, TestBinary] = {}
    for test in json.loads(listing.stdout)["tests"]:
        command = test.get("command")
        if not command or not command[0].startswith(str(build_dir)):
            continue  # Not built, or a script such as cpplint_test.
        if not os.access(command[0], os.X_OK):
            continue
        working_dir = next(
            (
                prop["value"]
                for prop in test.get("properties", [])
                if prop["name"] == "WORKING_DIRECTORY"
            ),
            str(build_dir),
        )
        binaries.setdefault(command[0], TestBinary(command[0], working_dir))
    return list(binaries.values())


def run_test(binary: TestBinary, profile_dir: Path) -> tuple[TestBinary, int, str]:
    # %c is continuous mode (see COVERAGE_FLAGS); %m pools the processes of one
    # binary into one file, so a death test's children do not leave one each.
    env = os.environ | {"LLVM_PROFILE_FILE": str(profile_dir / "%c%m.profraw")}
    result = subprocess.run(
        [binary.path, "--gtest_brief=1"],
        cwd=binary.working_dir,
        env=env,
        capture_output=True,
        text=True,
        errors="replace",
    )
    return binary, result.returncode, result.stdout + result.stderr


@dataclass
class FileCoverage:
    lines: dict[int, int] = field(default_factory=dict)
    # Keyed by the line of the definition, so the instantiations of a template
    # count as one function, called when any of them is.
    functions: dict[int, int] = field(default_factory=dict)
    function_names: dict[int, str] = field(default_factory=dict)
    # (line, block, branch) -> taken count
    branches: dict[tuple[int, int, int], int] = field(default_factory=dict)


def parse_lcov(text: str) -> dict[Path, FileCoverage]:
    files: dict[Path, FileCoverage] = {}
    current = FileCoverage()
    lines_by_name: dict[str, int] = {}
    for line in text.splitlines():
        tag, _, value = line.partition(":")
        if tag == "SF":
            current = files.setdefault(Path(value), FileCoverage())
            lines_by_name = {}
        elif tag == "DA":
            number, count = value.split(",")[:2]
            current.lines[int(number)] = current.lines.get(int(number), 0) + int(count)
        elif tag == "FN":
            number, name = value.split(",", 1)
            lines_by_name[name] = int(number)
            current.function_names.setdefault(int(number), name)
            current.functions.setdefault(int(number), 0)
        elif tag == "FNDA":
            count, name = value.split(",", 1)
            if name in lines_by_name:
                current.functions[lines_by_name[name]] += int(count)
        elif tag == "BRDA":
            number, block, branch, count = value.split(",")
            key = (int(number), int(block), int(branch))
            taken = 0 if count == "-" else int(count)
            current.branches[key] = current.branches.get(key, 0) + taken
    return files


def ranges(numbers: list[int]) -> list[tuple[int, int]]:
    """Collapses sorted line numbers into inclusive runs."""
    runs: list[tuple[int, int]] = []
    for number in numbers:
        if runs and number == runs[-1][1] + 1:
            runs[-1] = (runs[-1][0], number)
        else:
            runs.append((number, number))
    return runs


def percent(covered: int, total: int) -> str:
    return "100%" if covered == total else f"{100 * covered / total:.1f}%"


def compiled_files(build_dir: Path) -> set[Path]:
    """Returns every source and header that some object in build_dir was built from."""
    deps = subprocess.run(
        [cache_value(build_dir, "CMAKE_MAKE_PROGRAM"), "-C", str(build_dir), "-t", "deps"],
        capture_output=True,
        text=True,
        check=True,
    )
    return {
        (build_dir / line.strip()).resolve()
        for line in deps.stdout.splitlines()
        if line.startswith("    ")
    }


def report(
    path: Path, coverage: FileCoverage | None, compiled: bool, show_branches: bool
) -> bool:
    """Prints one file's coverage. Returns whether every line is covered."""
    name = path.relative_to(REPO_ROOT)
    if coverage is None and not compiled:
        print(f"\n{name}: in no test binary -- no test target compiles or links it")
        return False
    # A file with nothing to instrument, such as a header of declarations, has
    # no record in the export.
    if coverage is None or not coverage.lines:
        print(f"\n{name}: no executable code")
        return True

    missed_lines = sorted(n for n, count in coverage.lines.items() if count == 0)
    missed_functions = sorted(
        (line, coverage.function_names[line])
        for line, count in coverage.functions.items()
        if count == 0
    )
    missed_branches = sorted(key for key, taken in coverage.branches.items() if not taken)
    total_lines = len(coverage.lines)
    print(
        f"\n{name}: lines {percent(total_lines - len(missed_lines), total_lines)} "
        f"({total_lines - len(missed_lines)}/{total_lines})"
        f", functions {len(coverage.functions) - len(missed_functions)}"
        f"/{len(coverage.functions)}"
        f", branches {len(coverage.branches) - len(missed_branches)}"
        f"/{len(coverage.branches)}"
    )

    source = path.read_text(errors="replace").splitlines()
    if missed_functions:
        print("  functions never called:")
        cxxfilt = shutil.which("c++filt")
        for line, mangled in missed_functions:
            shown = mangled
            if cxxfilt:
                # A function with internal linkage is exported as "file.cc:_Z...".
                shown = subprocess.run(
                    [cxxfilt, mangled.rpartition(":")[2]], capture_output=True, text=True
                ).stdout.strip()
            print(f"    {line}: {shown}")
    if len(missed_lines) == total_lines:
        print("  no test reaches this file")
    elif missed_lines:
        print("  uncovered lines:")
        for first, last in ranges(missed_lines):
            for number in range(first, last + 1):
                print(f"    {number:5}: {source[number - 1]}")
            print("    " + "-" * 5)
    if missed_branches and len(missed_lines) < total_lines:
        lines_with_misses = sorted({line for line, _, _ in missed_branches})
        if show_branches:
            print("  lines with a branch never taken:")
            for number in lines_with_misses:
                sides = sum(1 for line, _, _ in missed_branches if line == number)
                print(f"    {number:5}: [{sides} missed] {source[number - 1].strip()}")
        else:
            print(
                f"  {len(missed_branches)} branch sides never taken on "
                f"{len(lines_with_misses)} lines (--branches lists them)"
            )
    return not missed_lines


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        "--build-dir",
        default=os.environ.get("COVERAGE_DIR", "build-coverage"),
        help="instrumented build directory (default: build-coverage, or $COVERAGE_DIR)",
    )
    parser.add_argument("--parallel", type=int, default=(os.cpu_count() or 2) // 2)
    parser.add_argument(
        "--tests", help="run only the test executables whose name matches this regex"
    )
    parser.add_argument(
        "--branches", action="store_true", help="list the lines with untaken branches"
    )
    parser.add_argument(
        "--no-build", action="store_true", help="reuse the binaries as they are"
    )
    parser.add_argument("files", nargs="+", help="the sources and headers to report on")
    args = parser.parse_args()

    files = [Path(name).resolve() for name in args.files]
    for path in files:
        if not path.is_file():
            sys.exit(f"{path} does not exist")

    build_dir = Path(args.build_dir)
    if not build_dir.is_absolute():
        build_dir = REPO_ROOT / build_dir
    if not (build_dir / "CMakeCache.txt").exists():
        configure(build_dir)
    if cache_value(build_dir, "CMAKE_CXX_FLAGS") != COVERAGE_FLAGS:
        sys.exit(f"{build_dir} was not configured by this script; delete it or pass another")

    if not args.no_build:
        build = subprocess.run(
            ["cmake", "--build", str(build_dir), "--parallel", str(args.parallel)],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )
        if build.returncode != 0:
            print(build.stdout[-8000:])
            sys.exit("build failed")

    binaries = test_binaries(build_dir)
    if args.tests:
        binaries = [b for b in binaries if re.search(args.tests, Path(b.path).name)]
    if not binaries:
        sys.exit("no test executables to run")

    profile_dir = build_dir / "coverage-profiles"
    shutil.rmtree(profile_dir, ignore_errors=True)
    profile_dir.mkdir()
    tests_passed = True
    with concurrent.futures.ThreadPoolExecutor(args.parallel) as pool:
        for binary, code, output in pool.map(
            lambda b: run_test(b, profile_dir), binaries
        ):
            if code != 0:
                tests_passed = False
                print(f"FAILED: {Path(binary.path).name}\n{output[-4000:]}")

    clang = cache_value(build_dir, "CMAKE_CXX_COMPILER")
    merged = profile_dir / "merged.profdata"
    subprocess.run(
        [llvm_tool("llvm-profdata", clang), "merge", "-sparse", "-o", str(merged)]
        + [str(p) for p in profile_dir.glob("*.profraw")],
        check=True,
    )
    objects = [binaries[0].path]
    for binary in binaries[1:]:
        objects += ["-object", binary.path]
    export = subprocess.run(
        [
            llvm_tool("llvm-cov", clang),
            "export",
            "-format=lcov",
            f"-instr-profile={merged}",
            *objects,
            *[str(path) for path in files],
        ],
        capture_output=True,
        text=True,
    )
    if export.returncode != 0:
        print(export.stderr[-4000:])
        sys.exit("llvm-cov failed")
    coverage = parse_lcov(export.stdout)

    print(f"ran {len(binaries)} test executables")
    complete = True
    compiled = compiled_files(build_dir)
    for path in files:
        complete &= report(path, coverage.get(path), path in compiled, args.branches)
    if not tests_passed:
        print("\nsome tests failed; the numbers above undercount")
    return 0 if complete and tests_passed else 1


if __name__ == "__main__":
    sys.exit(main())
