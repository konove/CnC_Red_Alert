#!/usr/bin/env python3
"""Plays UI scripts into the game and compares the screenshots they take.

A script (src/<game>/testdata/ui/<name>.script, format in
src/engine/window/player_script.h) presses keys and clicks through the game's
screens and takes named screenshots. The game runs it with -SCRIPT under SDL's
offscreen video driver, on a clock that moves a tick a frame, so a run does the
same thing every time and takes a second or two.

Beside each script sits <name>.hashes, one "<shot> <sha256>" line per
screenshot. A script that needs the game started with a switch names it in a
comment: "# args: -EDITOR". The pictures themselves hold the game's art and are not in the
repository: a run that disagrees with the hashes leaves them, as .pcx and (with
Pillow installed) .png, in build/ui_check/<game>/<name>/ to look at. When the
change is intended, --accept rewrites the hashes.

The game data is needed (the Steam install by default; RA_CD and TD_CD
override), so this is a local check, not a ctest.

Usage:
  tools/ui_check.py                       every script of both games
  tools/ui_check.py src/ra/testdata/ui/main_menu.script
  tools/ui_check.py --accept <script>     record what the game shows now
"""

from __future__ import annotations

import argparse
import hashlib
import os
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent

STEAM = Path.home() / ".local/share/Steam/steamapps/common"


@dataclass(frozen=True)
class Game:
    name: str  # "ra" or "td", the folder under src/
    binary: str  # the executable's name
    data_env: str  # the variable that overrides the data directory
    data_default: Path


GAMES = {
    "ra": Game("ra", "rasdl", "RA_CD", STEAM / "Command & Conquer Red Alert"),
    "td": Game("td", "tdsdl", "TD_CD", STEAM / "Command & Conquer"),
}

# A run that has not ended by now is stuck: scripts take seconds.
TIMEOUT_SECONDS = 120


def game_of(script: Path) -> Game:
    """Returns the game a script belongs to, from the src/<game>/ in its path."""
    parts = script.resolve().parts
    for index, part in enumerate(parts[:-1]):
        if part == "src" and parts[index + 1] in GAMES:
            return GAMES[parts[index + 1]]
    raise SystemExit(f"{script}: not under src/ra/ or src/td/")


def all_scripts() -> list[Path]:
    scripts: list[Path] = []
    for game in GAMES.values():
        scripts += sorted((REPO / "src" / game.name / "testdata/ui").glob("*.script"))
    return scripts


def read_hashes(path: Path) -> dict[str, str]:
    """Reads a .hashes file: '<shot> <sha256>' lines, '#' comments."""
    hashes: dict[str, str] = {}
    if not path.exists():
        return hashes
    for line in path.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        shot, digest = line.split()
        hashes[shot] = digest
    return hashes


def write_hashes(path: Path, hashes: dict[str, str]) -> None:
    lines = [f"{shot} {digest}\n" for shot, digest in sorted(hashes.items())]
    path.write_text("".join(lines))


def shots_of(script_text: str) -> list[str]:
    """Returns the names of the screenshots a script takes, in order."""
    shots = []
    for line in script_text.splitlines():
        words = line.split()
        if len(words) == 4 and words[0] == "at" and words[2] == "shot":
            shots.append(words[3])
    return shots


def extra_args(script_text: str) -> list[str]:
    """Returns the switches a script asks the game to be started with.

    A script names them in a comment, '# args: -EDITOR', which the game's own
    reader skips.
    """
    args: list[str] = []
    for line in script_text.splitlines():
        if line.startswith("# args:"):
            args += line.removeprefix("# args:").split()
    return args


def compare(expected: dict[str, str], actual: dict[str, str]) -> list[str]:
    """Returns one line per screenshot that is not as expected."""
    problems = []
    for shot in sorted(expected.keys() | actual.keys()):
        if shot not in actual:
            problems.append(f"{shot}: not taken")
        elif shot not in expected:
            problems.append(f"{shot}: no hash recorded")
        elif expected[shot] != actual[shot]:
            problems.append(f"{shot}: differs")
    return problems


def run_game(game: Game, binary: Path, data: Path, run_dir: Path, script: Path) -> int:
    """Runs one script in run_dir and returns the game's exit code."""
    link = run_dir / game.binary
    if link.is_symlink() or link.exists():
        link.unlink()
    link.symlink_to(binary)
    env = dict(os.environ, SDL_VIDEODRIVER="offscreen", SDL_AUDIODRIVER="dummy")
    command = [
        str(link),
        "-NOMOVIES",
        "-SEED1",
        *extra_args(script.read_text()),
        f"-SCRIPT{script}",
        f"-CD{data}",
    ]
    try:
        result = subprocess.run(
            command,
            cwd=run_dir,
            env=env,
            timeout=TIMEOUT_SECONDS,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            errors="replace",
            check=False,
        )
    except subprocess.TimeoutExpired:
        print(f"  timed out after {TIMEOUT_SECONDS} s")
        return -1
    if result.returncode != 0:
        print("  " + "\n  ".join(result.stdout.splitlines()[-5:]))
    return result.returncode


# What a run may leave in the run directory for the next one: the archives the
# game copies there once, which are large and never change, and the snapshot
# reset_run_dir() restores.
BASE_SUFFIX = ".base"


def is_kept(path: Path) -> bool:
    return path.suffix.upper() == ".MIX" or path.name.endswith(BASE_SUFFIX)


def reset_run_dir(run_dir: Path) -> None:
    """Puts the run directory back as it was after the warm-up run.

    A script may change the options or save a game; the next script, or the
    next run of the same one, must not see that.
    """
    for path in run_dir.iterdir():
        if not is_kept(path):
            path.unlink()
    for base in run_dir.glob(f"*{BASE_SUFFIX}"):
        shutil.copy(base, base.with_name(base.name.removesuffix(BASE_SUFFIX)))


def warm_up(game: Game, binary: Path, data: Path, run_dir: Path) -> None:
    """Runs the game once in a new directory and snapshots what it wrote.

    The first run in a directory writes its configuration file and plays as a
    fresh install, so it shows other screens than the runs after it.
    """
    if any(run_dir.glob(f"*{BASE_SUFFIX}")):
        return
    script = run_dir / "warm_up.script"
    script.write_text("at 30 quit\n")
    run_game(game, binary, data, run_dir, script)
    script.unlink()
    (run_dir / game.binary).unlink()
    for path in list(run_dir.iterdir()):
        if not is_kept(path):
            path.rename(path.with_name(path.name + BASE_SUFFIX))


def to_png(pcx: Path) -> None:
    try:
        from PIL import Image  # pylint: disable=import-outside-toplevel
    except ImportError:
        return
    Image.open(pcx).convert("RGB").save(pcx.with_suffix(".png"))


def check(script: Path, args: argparse.Namespace) -> bool:
    """Runs one script; returns whether its screenshots are as recorded."""
    game = game_of(script)
    binary = Path(args.build_dir) / "src" / game.name / game.binary
    if not binary.exists():
        raise SystemExit(f"{binary} is not built")
    data = Path(os.environ.get(game.data_env, game.data_default))
    if not data.is_dir():
        raise SystemExit(f"{data}: no game data; set {game.data_env}")

    work = REPO / "build/ui_check" / game.name
    run_dir = work / "run"
    run_dir.mkdir(parents=True, exist_ok=True)
    warm_up(game, binary.resolve(), data, run_dir)
    reset_run_dir(run_dir)

    # The game writes its screenshots beside the script it was given.
    shot_dir = work / script.stem
    shutil.rmtree(shot_dir, ignore_errors=True)
    shot_dir.mkdir(parents=True)
    copy = shot_dir / script.name
    shutil.copy(script, copy)

    print(f"{script.relative_to(REPO) if script.is_relative_to(REPO) else script}")
    code = run_game(game, binary.resolve(), data, run_dir, copy)
    if code != 0:
        print(f"  FAIL: the game exited with {code}")
        return False

    actual = {
        pcx.stem: hashlib.sha256(pcx.read_bytes()).hexdigest()
        for pcx in sorted(shot_dir.glob("*.pcx"))
    }
    missing = [shot for shot in shots_of(script.read_text()) if shot not in actual]
    if missing:
        print(f"  FAIL: no screenshot for {', '.join(missing)}")
        return False

    hashes_path = script.with_suffix(".hashes")
    if args.accept:
        write_hashes(hashes_path, actual)
        for pcx in shot_dir.glob("*.pcx"):
            to_png(pcx)
        print(f"  recorded {len(actual)} screenshot(s); look at them in "
              f"{shot_dir.relative_to(REPO)}/")
        return True

    problems = compare(read_hashes(hashes_path), actual)
    if not problems:
        print(f"  ok, {len(actual)} screenshot(s)")
        shutil.rmtree(shot_dir)
        return True
    for pcx in shot_dir.glob("*.pcx"):
        to_png(pcx)
    for problem in problems:
        print(f"  FAIL: {problem}")
    print(f"  the screenshots are in {shot_dir.relative_to(REPO)}/")
    return False


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("scripts", nargs="*", type=Path, help="default: all of them")
    parser.add_argument(
        "--accept", action="store_true", help="record the screenshots as correct"
    )
    parser.add_argument(
        "--build-dir", default=str(REPO / "build"), help="default: build/"
    )
    args = parser.parse_args()

    scripts = [script.resolve() for script in args.scripts] or all_scripts()
    if not scripts:
        print("no scripts")
        return 0
    results = [check(script, args) for script in scripts]
    failed = results.count(False)
    if failed:
        print(f"{failed} of {len(results)} script(s) failed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
