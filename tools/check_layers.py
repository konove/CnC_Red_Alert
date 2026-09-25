#!/usr/bin/env python3
"""Checks that the engine's includes follow its dependency order.

Every engine target shares the src/ include root, so linking only proves that
a library calls nothing it does not link; a header-only include from a folder
the library may not see still builds. This script reads the include lines
instead. It places each engine file in its folder under src/engine/ -- by the
file map in tools/engine_layout.py while the file still sits in src/base,
src/port, src/sdllib, src/tech or src/winvq, by its path once it has moved --
and fails on:

- a source file in those folders that the file map does not place, or a file
  under src/engine/ outside the known folders;
- a file map entry that names no file, old or moved;
- an include of a folder the file's library may not see (the plan's
  "Dependency order"), or of src/ra, src/td, src/tools or src/testing;
- an SDL header outside window/, audio/, video/, video/vqa/ and
  platform/timer.cc;
- a quoted include written relative to the including file ("hsv.h") rather
  than to src/, which hides the folder it names, or a project include that
  names no file.

Project includes are checked whether written with quotes or angle brackets
(src/ is on the include path, so <sdllib/display.h> compiles too).

Tests (*_test.cc, *_test_util.h) link more than their library does, so they
may include any folder, the games and SDL; the map rules and the last rule
still apply to them. Files that are not C++ (CMakeLists.txt, notes) are not
mapped and not checked.

Usage:
  tools/check_layers.py
  tools/check_layers.py --src path/to/src

Runs as the engine_layers_test ctest.
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
PROJECT_ROOTS = layout.OLD_ROOTS + (layout.ENGINE_ROOT, "ra", "td", "tools", "testing")

# The folders engine code may never include: the games, the tools and the
# test main sit above every engine library.
FORBIDDEN_ROOTS = ("ra", "td", "tools", "testing")

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
    """Returns every engine source file, old or moved, relative to src_root."""
    moved = src_root / layout.ENGINE_ROOT
    new = (
        [
            path.relative_to(src_root).as_posix()
            for path in moved.rglob("*")
            if path.is_file() and layout.is_source(path.name)
        ]
        if moved.is_dir()
        else []
    )
    return sorted(layout.old_files(src_root) + new)


def _covers(stem: str, old_folder: str, destination: str, path: str) -> bool:
    """Returns whether the file map entry old_folder/stem names path, before
    or after its move to destination."""
    folder, _, name = path.rpartition("/")
    if folder not in (old_folder, f"{layout.ENGINE_ROOT}/{destination}"):
        return False
    return stem == layout.ALL or stem in layout.entry_stems(name)


def map_errors(files: list[str]) -> list[str]:
    """Returns the files the layout cannot place and the stale map entries."""
    errors = [
        (
            f"src/{path}: not in a known engine folder"
            if path.startswith(layout.ENGINE_ROOT + "/")
            else f"src/{path}: not in the engine file map (tools/engine_layout.py)"
        )
        for path in files
        if layout.destination_of(path) is None
    ]
    for destination, sources in layout.FILE_MAP.items():
        for old_folder, stems in sources.items():
            for stem in stems:
                if not any(
                    _covers(stem, old_folder, destination, path) for path in files
                ):
                    errors.append(
                        f"tools/engine_layout.py: {old_folder}/{stem} -> "
                        f"{destination} names no file"
                    )
    return errors


def include_errors(src_root: Path, path: str) -> list[str]:
    """Returns the include-rule violations in one engine file."""
    folder = layout.destination_of(path)
    if folder is None:
        return []  # Reported by map_errors().
    library = layout.library_of(folder)
    allowed = layout.allowed_libraries(library)
    test = layout.is_test(path)
    name = path.rsplit("/", 1)[-1]
    sdl_allowed = (
        test
        or library in layout.SDL_LIBRARIES
        or f"{folder}/{name}" in layout.SDL_FILES
    )
    errors: list[str] = []
    text = (src_root / path).read_text(encoding="utf-8", errors="replace")
    for line, delimiter, included in includes(text):
        closer = ">" if delimiter == "<" else '"'
        where = f"src/{path}:{line}: {delimiter}{included}{closer}"
        root = included.split("/", 1)[0]
        sibling = Path(os.path.normpath((src_root / path).parent / included))
        if root in PROJECT_ROOTS:
            # Angle brackets reach src/ too (it is on the include path), so
            # <sdllib/display.h> is checked like "sdllib/display.h".
            if not (src_root / included).is_file():
                errors.append(f"{where}: no such file under src/")
                continue
        elif (
            delimiter == '"'
            and sibling.is_file()
            and sibling.is_relative_to(src_root)
        ):
            errors.append(
                f"{where}: include relative to the file; write the path from src/"
            )
            included = sibling.relative_to(src_root).as_posix()
            root = included.split("/", 1)[0]
        else:
            # Outside src/: a system or third-party header (absl, gtest, lzo,
            # SDL). Only SDL is restricted.
            if is_sdl_header(included) and not sdl_allowed:
                errors.append(
                    f"{where}: SDL header in {folder}, which compiles without SDL"
                )
            continue
        if test:
            continue
        if root in FORBIDDEN_ROOTS:
            errors.append(f"{where}: engine code may not include {root}/")
            continue
        target = layout.destination_of(included)
        if target is None:
            continue  # An unmapped file, reported by map_errors().
        target_library = layout.library_of(target)
        if target_library not in allowed:
            errors.append(
                f"{where}: {folder} ({layout.LIBRARIES[library]}) may not "
                f"include {target} ({layout.LIBRARIES[target_library]})"
            )
    return errors


def check(src_root: Path) -> list[str]:
    """Returns every violation in the tree at src_root."""
    files = engine_files(src_root)
    errors = map_errors(files)
    for path in files:
        errors.extend(include_errors(src_root, path))
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
    print(f"{len(engine_files(src_root))} engine files follow the dependency order")
    return 0


if __name__ == "__main__":
    sys.exit(main())
