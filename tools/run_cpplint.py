#!/usr/bin/env python3
"""Runs cpplint over every C++ file under src/ (docs/CPPLINT_PLAN.md).

The settings live in CPPLINT.cfg at the repository root and in the
per-directory CPPLINT.cfg files, which cpplint reads by itself; this script
only finds the files and runs cpplint on them in parallel, so the
cpplint_test ctest takes seconds instead of the ~45 s of one process.

Usage: tools/run_cpplint.py [--cpplint PATH] [--src DIR] [FILE ...]

With FILEs, lints only those. Exits non-zero if cpplint reports anything.
"""
import argparse
import concurrent.futures
import os
import pathlib
import re
import subprocess
import sys

CHUNK = 40

# A finding: "path:line:  message  [category] [confidence]". cpplint also
# prints progress and totals, which are not findings.
FINDING = re.compile(r'^.+:(\d+|None):  .* \[[\w/+]+\] \[\d\]$')


def lint(cpplint, files):
    """Returns cpplint's findings for files, one line each.

    A failure that produced no findings (a bad CPPLINT.cfg, a crash) comes
    back as cpplint's whole output, so it fails the run instead of passing.
    """
    result = subprocess.run([cpplint, '--quiet', *files],
                            capture_output=True, text=True, check=False)
    output = (result.stdout + result.stderr).splitlines()
    findings = [line for line in output if FINDING.match(line)]
    if result.returncode != 0 and not findings:
        return output or [f'cpplint exited with {result.returncode}']
    return findings


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--cpplint', default='cpplint')
    parser.add_argument('--src', type=pathlib.Path,
                        default=pathlib.Path(__file__).parent.parent / 'src')
    parser.add_argument('files', nargs='*')
    args = parser.parse_args()

    files = args.files or sorted(
        str(p) for p in args.src.rglob('*') if p.suffix in ('.h', '.cc'))
    chunks = [files[i:i + CHUNK] for i in range(0, len(files), CHUNK)]
    with concurrent.futures.ThreadPoolExecutor(os.cpu_count()) as pool:
        findings = sorted(line for lines in pool.map(
            lambda chunk: lint(args.cpplint, chunk), chunks) for line in lines)

    for line in findings:
        print(line)
    if findings:
        print(f'cpplint: {len(findings)} findings in {len(files)} files; '
              'see CPPLINT.cfg and docs/CPPLINT_PLAN.md', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
