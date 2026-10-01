#!/usr/bin/env python3
"""Checks that the tree's includes follow its dependency order.

Every target shares the src/ include root, so linking only proves that a
library calls nothing it does not link; a header-only include from a folder
the library may not see still builds. This script reads the include lines
instead. It places each library file in its folder under src/engine/,
src/base/ or src/game/ by its path (tools/engine_layout.py) and fails on:

- a source file under src/engine/, src/base/ or src/game/ outside the known
  folders;
- an include of a folder the file's library may not see (the plan's
  "Dependency order"), or of src/ra, src/td, src/tools or src/testing;
- an SDL header outside window/, audio/, video/ and video/vqa/;
- a quoted include written relative to the including file ("hsv.h") rather
  than to src/, which hides the folder it names, or a project include that
  names no file;
- in a game (src/ra, src/td), a source file outside the known game folders
  (engine_layout.GAME_FOLDERS); an include of the other game, of src/tools
  or of src/testing; an include of a game folder its own folder excludes
  (engine_layout.GAME_FOLDER_EXCLUDES, tests exempt); or either of the
  previous rule's two mistakes;
- a header anywhere under src/ whose include guard is not
  CNC_RED_ALERT_<PATH>_H_ (engine_layout.header_guard()).

Project includes are checked whether written with quotes or angle brackets
(src/ is on the include path, so <engine/window/display.h> compiles too).

Tests (*_test.cc, *_test_util.h) of a library link more than their library
does, so they may include any folder, the games and SDL; the folder rule and
the relative/missing rule still apply to them. A game's tests may not include
the other game either. Files that are not C++ (CMakeLists.txt, notes) are not
placed and not checked.

Usage:
  tools/check_layers.py
  tools/check_layers.py --src path/to/src

Runs as the engine_layers_test ctest; the name predates the game rules.
"""

import argparse
import os
import re
import sys
from pathlib import Path

import engine_layout as layout

REPO_ROOT = Path(__file__).resolve().parent.parent

# The top-level folders under src/ that hold project code. A quoted include
# starting with one of them is a project include and must resolve.
PROJECT_ROOTS = (*layout.LIBRARY_ROOTS, *layout.GAMES, "tools", "testing")

# The folders library code may never include: the games, the tools and the
# test main sit above every library.
FORBIDDEN_ROOTS = (*layout.GAMES, "tools", "testing")

_GUARD = re.compile(
    r"^[ \t]*#[ \t]*ifndef[ \t]+(\w+)[ \t]*\n[ \t]*#[ \t]*define[ \t]+(\w+)",
    re.MULTILINE,
)

_INCLUDE = re.compile(r'^[ \t]*#[ \t]*include[ \t]*([<"])([^>"\n]+)[>"]', re.MULTILINE)


def strip_comments(text: str) -> str:
    """Returns text with its comments blanked out, keeping line numbers.

    String and character literals are skipped over, so a "//" or "/*" inside
    one does not start a comment.
    """
    out: list[str] = []
    i = 0
    n = len(text)
    while i < n:
        c = text[i]
        if c in "\"'":
            end = i + 1
            while end < n and text[end] != c and text[end] != "\n":
                end += 2 if text[end] == "\\" else 1
            out.append(text[i : end + 1])
            i = end + 1
        elif text.startswith("//", i):
            end = text.find("\n", i)
            end = n if end == -1 else end
            i = end
        elif text.startswith("/*", i):
            end = text.find("*/", i + 2)
            end = n if end == -1 else end + 2
            out.append("".join("\n" if ch == "\n" else " " for ch in text[i:end]))
            i = end
        else:
            out.append(c)
            i += 1
    return "".join(out)


def includes(text: str) -> list[tuple[int, str, str]]:
    """Returns (line, delimiter, name) for each #include outside comments."""
    code = strip_comments(text)
    return [
        (code.count("\n", 0, match.start()) + 1, match.group(1), match.group(2))
        for match in _INCLUDE.finditer(code)
    ]


def is_sdl_header(name: str) -> bool:
    """Returns whether an include names an SDL header (<SDL.h>, <SDL2/...>).

    Only asked of includes that do not resolve under src/, so a project
    header named SDL_*.h is layer-checked instead.
    """
    first = name.split("/", 1)[0]
    return first == "SDL2" or name.rsplit("/", 1)[-1].startswith("SDL")


def engine_files(src_root: Path) -> list[str]:
    """Returns every library source file, relative to src_root."""
    return _files_under(src_root, layout.LIBRARY_ROOTS)


def game_files(src_root: Path) -> list[str]:
    """Returns every source file of the games, relative to src_root."""
    return _files_under(src_root, layout.GAMES)


def _files_under(src_root: Path, root_names) -> list[str]:
    roots = [src_root / root for root in root_names]
    return sorted(
        path.relative_to(src_root).as_posix()
        for root in roots
        if root.is_dir()
        for path in root.rglob("*")
        if path.is_file() and layout.is_source(path.name)
    )


def folder_errors(files: list[str]) -> list[str]:
    """Returns the files the layout cannot place: under a library root but
    not in a known folder."""
    return [
        f"src/{path}: not in a known library folder"
        for path in files
        if layout.library_folder_of(path) is None
    ]


def _resolve(src_root: Path, path: str, delimiter: str, included: str):
    """Places one include of the file at path.

    Returns (name, error): name is the include's path from src/ when it is a
    project file, else None (a system or third-party header); error describes
    a project include that names no file or is written relative to the
    including file, else None. A relative include that resolves still gets
    its name, so the caller checks it like any other.
    """
    root = included.split("/", 1)[0]
    sibling = Path(os.path.normpath((src_root / path).parent / included))
    if root in PROJECT_ROOTS:
        # Angle brackets reach src/ too (it is on the include path), so
        # <engine/window/display.h> is checked like "engine/window/display.h".
        if not (src_root / included).is_file():
            return None, "no such file under src/"
        return included, None
    if delimiter == '"' and sibling.is_file() and sibling.is_relative_to(src_root):
        return (
            sibling.relative_to(src_root).as_posix(),
            "include relative to the file; write the path from src/",
        )
    return None, None


def include_errors(src_root: Path, path: str) -> list[str]:
    """Returns the include-rule violations in one library or game file."""
    game = layout.game_of(path)
    if game is not None:
        return _game_include_errors(src_root, path, game)
    folder = layout.library_folder_of(path)
    if folder is None:
        return []  # Reported by folder_errors().
    library = layout.library_of(folder)
    allowed = layout.allowed_libraries(library)
    test = layout.is_test(path)
    sdl_allowed = test or library in layout.SDL_LIBRARIES
    errors: list[str] = []
    text = (src_root / path).read_text(encoding="utf-8", errors="replace")
    for line, delimiter, included in includes(text):
        closer = ">" if delimiter == "<" else '"'
        where = f"src/{path}:{line}: {delimiter}{included}{closer}"
        name, error = _resolve(src_root, path, delimiter, included)
        if error is not None:
            errors.append(f"{where}: {error}")
        if name is None:
            # Outside src/: a system or third-party header (absl, gtest, lzo,
            # SDL). Only SDL is restricted.
            if error is None and is_sdl_header(included) and not sdl_allowed:
                errors.append(
                    f"{where}: SDL header in {folder}, which compiles without SDL"
                )
            continue
        included = name
        root = included.split("/", 1)[0]
        if test:
            continue
        if root in FORBIDDEN_ROOTS:
            errors.append(f"{where}: engine code may not include {root}/")
            continue
        target = layout.library_folder_of(included)
        if target is None:
            continue  # An unplaced file, reported by folder_errors().
        target_library = layout.library_of(target)
        if target_library not in allowed:
            errors.append(
                f"{where}: {folder} ({layout.LIBRARIES[library]}) may not "
                f"include {target} ({layout.LIBRARIES[target_library]})"
            )
    return errors


def game_folder_errors(files: list[str]) -> list[str]:
    """Returns the game files that sit outside the known game folders."""
    return [
        f"src/{path}: not in a known game folder"
        for path in files
        if layout.game_folder_of(path) is None
    ]


def _game_include_errors(src_root: Path, path: str, game: str) -> list[str]:
    """Returns the include-rule violations in one file of game. A game may
    include every library, its own files and SDL; not the other game, the
    tools or the test main, nor a folder its own folder excludes."""
    folder = layout.game_folder_of(path)
    excluded = () if layout.is_test(path) else layout.GAME_FOLDER_EXCLUDES.get(folder, ())
    errors: list[str] = []
    text = (src_root / path).read_text(encoding="utf-8", errors="replace")
    for line, delimiter, included in includes(text):
        closer = ">" if delimiter == "<" else '"'
        where = f"src/{path}:{line}: {delimiter}{included}{closer}"
        name, error = _resolve(src_root, path, delimiter, included)
        if error is not None:
            errors.append(f"{where}: {error}")
        if name is None:
            continue
        root = name.split("/", 1)[0]
        if root in FORBIDDEN_ROOTS and root != game:
            errors.append(f"{where}: {game} may not include {root}/")
        elif root == game and layout.game_folder_of(name) in excluded:
            errors.append(
                f"{where}: {game}/{folder} may not include "
                f"{game}/{layout.game_folder_of(name)}"
            )
    return errors


def header_guard_errors(src_root: Path, path: str) -> list[str]:
    """Returns the include-guard violations in the header at path: none, or
    one if its first #ifndef/#define pair is missing or names another guard
    than engine_layout.header_guard(path)."""
    text = (src_root / path).read_text(encoding="utf-8", errors="replace")
    code = strip_comments(text)
    match = _GUARD.search(code)
    expected = layout.header_guard(path)
    if match is None:
        return [f"src/{path}: no header guard; expected {expected}"]
    where = f"src/{path}:{code.count(chr(10), 0, match.start()) + 1}"
    guard, defined = match.group(1), match.group(2)
    if guard != expected:
        return [
            f"{where}: header guard {guard} does not match the path; "
            f"expected {expected}"
        ]
    if defined != guard:
        return [f"{where}: tests {guard} but defines {defined}"]
    return []


def check(src_root: Path) -> list[str]:
    """Returns every violation in the tree at src_root."""
    files = engine_files(src_root)
    games = game_files(src_root)
    errors = folder_errors(files) + game_folder_errors(games)
    for path in files + games:
        errors.extend(include_errors(src_root, path))
    for path in _files_under(src_root, PROJECT_ROOTS):
        if path.endswith(".h"):
            errors.extend(header_guard_errors(src_root, path))
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        "--src",
        type=Path,
        default=REPO_ROOT / "src",
        help="the source root to check (default: the repository's src/)",
    )
    args = parser.parse_args()
    src_root = args.src.resolve()
    if not src_root.is_dir():
        sys.exit(f"{args.src} is not a directory")
    errors = check(src_root)
    for error in errors:
        print(error)
    if errors:
        print(f"{len(errors)} layering violation(s)")
        return 1
    print(
        f"{len(engine_files(src_root))} library and {len(game_files(src_root))} "
        "game files follow the dependency order"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
