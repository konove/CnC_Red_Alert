#!/usr/bin/env python3
"""Plays a two-player network game between two instances and checks they agree.

Each instance is started with -NETGAME<1|2>, which skips the lobby and binds a
loopback address of its own, and with -QUITFRAME, which logs where every
mobile object is on each frame. A network game is lockstep: both sides must
compute the same game from the orders they exchange, so the two logs have to
be identical. They are not the same from one run to the next, since the
instances keep real time.

A check is a pair of scripts, <name>.p1.script and <name>.p2.script (format in
src/engine/window/player_script.h), in src/ra/testdata/net/. Player 1's script
names the map and the length of the game in comments:

  # map: SCM01EA
  # frames: 900

The scripts give the orders; without any, only the frame-sync packets cross
the wire. Their frames are frames presented in real time, about 70 a second,
so leave wide gaps. Screenshots land in build/net_check/ra/<name>/ for aiming
the clicks; they are not compared.

Needs the game data (RA_CD overrides the Steam directory), so this is a local
check, not a ctest.

Usage:
  tools/net_check.py                 every check
  tools/net_check.py src/ra/testdata/net/orders
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
DATA = Path(
    os.environ.get(
        "RA_CD",
        Path.home() / ".local/share/Steam/steamapps/common/Command & Conquer Red Alert",
    )
)
TIMEOUT_SECONDS = 180


def setting(script_text: str, name: str, default: str) -> str:
    """Returns the value of a '# name: value' comment, or the default."""
    match = re.search(rf"^# {name}:\s*(\S+)", script_text, re.MULTILINE)
    return match.group(1) if match else default


def positions(log: str) -> list[str]:
    """Returns the 'frame ...' lines of an instance's output."""
    return re.findall(r"frame .*", log)


def moved(lines: list[str]) -> int:
    """Returns how many objects were logged at more than one coordinate."""
    # Objects have no name in the log, so one is told from the next by its
    # place among its frame's lines.
    seen: dict[str, set[str]] = {}
    by_frame: dict[str, list[str]] = {}
    for line in lines:
        by_frame.setdefault(line.split()[1], []).append(line)
    for frame_lines in by_frame.values():
        for slot, line in enumerate(frame_lines):
            words = line.split()
            seen.setdefault(f"{slot} {words[2]} {words[3]}", set()).add(words[5])
    return sum(1 for coords in seen.values() if len(coords) > 1)


def check(base: Path, build_dir: Path) -> bool:
    scripts = [base.with_name(f"{base.name}.p{n}.script") for n in (1, 2)]
    for script in scripts:
        if not script.exists():
            raise SystemExit(f"{script}: missing")
    binary = (build_dir / "src/ra/rasdl").resolve()
    if not binary.exists():
        raise SystemExit(f"{binary} is not built")
    if not DATA.is_dir():
        raise SystemExit(f"{DATA}: no game data; set RA_CD")

    first = scripts[0].read_text()
    game_map = setting(first, "map", "SCM01EA")
    frames = setting(first, "frames", "600")

    work = REPO / "build/net_check/ra"
    shot_dir = work / base.name
    shutil.rmtree(shot_dir, ignore_errors=True)
    shot_dir.mkdir(parents=True)

    print(base.relative_to(REPO) if base.is_relative_to(REPO) else base)
    processes = []
    for number, script in enumerate(scripts, start=1):
        run_dir = work / f"p{number}"
        run_dir.mkdir(parents=True, exist_ok=True)
        link = run_dir / "rasdl"
        if link.is_symlink() or link.exists():
            link.unlink()
        link.symlink_to(binary)
        # Each game writes its screenshots beside the script it was given.
        copy = shot_dir / f"p{number}" / script.name
        copy.parent.mkdir()
        shutil.copy(script, copy)
        env = dict(os.environ, SDL_VIDEODRIVER="offscreen", SDL_AUDIODRIVER="dummy")
        command = [
            str(link),
            "-NOMOVIES",
            "-SEED1",
            f"-NEWGAME{game_map}",
            f"-NETGAME{number}",
            f"-QUITFRAME{frames}",
            f"-SCRIPT{copy}",
            f"-CD{DATA}",
        ]
        # Straight to a file: an instance that blocked on a full pipe would
        # stop answering the other, which then plays on without it.
        log_path = shot_dir / f"p{number}.log"
        with open(log_path, "w", encoding="utf-8") as log_file:
            processes.append(
                subprocess.Popen(
                    command,
                    cwd=run_dir,
                    env=env,
                    stdout=log_file,
                    stderr=subprocess.STDOUT,
                )
            )

    logs = []
    ok = True
    for number, process in enumerate(processes, start=1):
        try:
            process.wait(timeout=TIMEOUT_SECONDS)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
            print(f"  FAIL: player {number} timed out after {TIMEOUT_SECONDS} s")
            ok = False
        output = (shot_dir / f"p{number}.log").read_text(errors="replace")
        if process.returncode != 0 and ok:
            print(f"  FAIL: player {number} exited with {process.returncode}")
            print("  " + "\n  ".join(output.splitlines()[-4:]))
            ok = False
        logs.append(positions(output))
    if not ok:
        return False

    last_frame = logs[0][-1].split()[1] if logs[0] else "none"
    if not logs[0] or last_frame != frames:
        print(f"  FAIL: player 1 logged up to frame {last_frame}, not {frames}")
        return False
    if logs[0] != logs[1]:
        differing = next(
            (a for a, b in zip(logs[0], logs[1]) if a != b), "one log is shorter"
        )
        print(f"  FAIL: the two games differ, first at: {differing}")
        print(f"  the logs are in {shot_dir.relative_to(REPO)}/")
        return False
    print(
        f"  ok, {frames} frames in step, {len(logs[0])} positions, "
        f"{moved(logs[0])} object(s) moved"
    )
    for pcx in shot_dir.glob("p*/*.pcx"):
        try:
            from PIL import Image  # pylint: disable=import-outside-toplevel
        except ImportError:
            break
        Image.open(pcx).convert("RGB").save(pcx.with_suffix(".png"))
    return True


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("checks", nargs="*", type=Path, help="default: all of them")
    parser.add_argument("--build-dir", default=str(REPO / "build"))
    args = parser.parse_args()

    checks = [path.resolve() for path in args.checks]
    if not checks:
        net_dir = REPO / "src/ra/testdata/net"
        checks = sorted(
            path.with_name(path.name.removesuffix(".p1.script"))
            for path in net_dir.glob("*.p1.script")
        )
    if not checks:
        print("no checks")
        return 0
    results = [check(base, Path(args.build_dir)) for base in checks]
    failed = results.count(False)
    if failed:
        print(f"{failed} of {len(results)} check(s) failed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
