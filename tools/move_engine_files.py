#!/usr/bin/env python3
"""Moves one engine library's files under src/engine/, one phase-B step.

docs/ENGINE_FOLDERS_PLAN.md moves the shared code out of src/base, src/port,
src/sdllib, src/tech and src/winvq one library per commit, bottom up. This
script does the mechanical part of a step, reading the file map in
tools/engine_layout.py so the files land where tools/check_layers.py expects
them. For each library named on the command line, in order, it:

1. `git mv`s every file FILE_MAP sends to the library's folder, and to its
   subfolders: `base` also moves `base/strings`, `platform` moves
   `platform/win32`, `net` moves `net/serial`. `video/vqa` is a library of its
   own, so `video` does not move it; step 11 runs `video/vqa video`.
2. Rewrites every `#include "old/x.h"` and `#include <old/x.h>` naming a moved
   file to `engine/<folder>/x.h`, in the C++ sources under src/ and tools/.
   Includes inside comments, string literals and raw string literals are left
   alone; mentions of the old paths in comments are listed at the end instead.
3. Regenerates each moved header's include guard from its new path
   (CNC_RED_ALERT_ENGINE_BASE_TYPES_H_).
4. Takes the moved files out of the CMakeLists.txt that built them: their
   names leave add_gtest() SOURCES lists and optimize_in_debug() calls. An
   optimize_in_debug() or add_gtest() left without files is deleted with the
   comment above it (a split test target such as sdllib_keyboard_test has to
   be recreated by hand in the new folder; the summary names each one).
5. Writes src/engine/<library>/CMakeLists.txt: a STATIC library (INTERFACE
   when it has no .cc) globbing its folder and subfolders, linking
   - PUBLIC: the libraries DEPENDS_ON points at, directly only (the rest come
     through them), and the external targets the headers include;
   - PRIVATE: the external targets only the .cc files include;
   and one add_gtest() named <target>_test over every *_test.cc (and
   *_test_util.cc helper), linking the
   library plus whatever the tests include that it does not already bring:
   other engine libraries, old targets (tech, sdllib, ...) for files that have
   not moved yet, and external targets. optimize_in_debug() files carried over
   from the old CMakeLists.txt are listed again here.
   It then rewrites src/engine/CMakeLists.txt with an add_subdirectory() per
   generated library in dependency order, and adds add_subdirectory(src/engine)
   to the top-level CMakeLists.txt the first time.
6. Deletes an old root (src/base, ...) that no longer holds any source file,
   with its add_subdirectory() line.
7. Runs clang-format over just the lines it changed, which re-sorts the include
   blocks those lines sit in.

External targets come from EXTERNAL_TARGETS below, matched on the include
name; a quoted include it does not know (a new Abseil header) stops the script
before anything moves, so add it there. Angle-bracket headers it does not know
are taken to be system headers. SDL headers link SDL2::SDL2.

What it does not do, and a step must do by hand and name in its commit
message: link lists in the old targets (an old target that uses what moved
links the new target instead, or as well), Windows-only or optional libraries
(ws2_32, wsock32, libserialport), split test targets, and the documentation
(phase B's "Configuration and documentation" list).

Why clang-format and not `git clang-format`: git clang-format diffs the
working tree against HEAD per path, so a moved file has no old side and every
line of it counts as changed. Many legacy files are not clang-format clean, and
the move would reformat them whole. Passing clang-format the changed lines
(--lines, the same thing git clang-format does with the lines of its diff)
formats only what this script touched.

Usage:
  tools/move_engine_files.py base
  tools/move_engine_files.py video/vqa video
  tools/move_engine_files.py --dry-run stream

--dry-run prints the moves and the generated CMakeLists.txt without touching
the tree.
"""

from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

import engine_layout as layout
from check_layers import PROJECT_ROOTS, is_sdl_header

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC_ROOT = REPO_ROOT / "src"

# The folders, relative to the repository, whose C++ includes are rewritten.
REWRITE_ROOTS = ("src", "tools")
CPP_SUFFIXES = (".h", ".hpp", ".cc", ".cpp", ".c", ".inl")

# The target each old root builds; a test that includes a file which has not
# moved yet links this.
OLD_TARGETS = {
    "base": "base",
    "port": "port",
    "sdllib": "sdllib",
    "tech": "tech",
    "winvq": "vqa32",
}

# Include name prefix -> the CMake target that provides it, first match wins.
# None means add_gtest() already links it.
EXTERNAL_TARGETS: tuple[tuple[str, str | None], ...] = (
    ("absl/base/", "absl::core_headers"),
    ("absl/log/check.h", "absl::check"),
    ("absl/log/log.h", "absl::log"),
    ("absl/strings/str_format.h", "absl::str_format"),
    ("absl/strings/", "absl::strings"),
    ("absl/types/span.h", "absl::span"),
    ("absl/container/flat_hash_map.h", "absl::flat_hash_map"),
    ("absl/container/flat_hash_set.h", "absl::flat_hash_set"),
    ("magic_enum/", "magic_enum::magic_enum"),
    ("lzo/", "lzo"),
    ("gtest/", None),
    ("gmock/", None),
)
SDL_TARGET = "SDL2::SDL2"

GENERATED_BY = "tools/move_engine_files.py"

_DIRECTIVE = re.compile(r"^[ \t]*#[ \t]*(\w+)", re.MULTILINE)
_INCLUDE_NAME = re.compile(r'[ \t]*(?:<([^>\n]+)>|"([^"\n]+)")')
_IDENTIFIER_CHAR = re.compile(r"[A-Za-z0-9_]")
_RAW_PREFIXES = ("R", "u8R", "uR", "UR", "LR")


# --- Reading C++ -------------------------------------------------------------


def blank_comments_and_literals(text: str) -> str:
    """Returns text with comments and string and character literals replaced
    by spaces, newlines kept, so offsets and line numbers still match.

    Handles // comments continued by a trailing backslash, raw string literals
    (R"x(...)x"), and digit separators (1'000), which open no literal. What is
    left visible is code: a directive found in the result is a real one.
    """
    out = list(text)
    n = len(text)

    def blank(start: int, end: int) -> None:
        for k in range(start, min(end, n)):
            if out[k] != "\n":
                out[k] = " "

    i = 0
    while i < n:
        c = text[i]
        if text.startswith("//", i):
            end = i
            while True:
                end = text.find("\n", end)
                if end == -1:
                    end = n
                    break
                if text[end - 1] != "\\":
                    break
                end += 1
            blank(i, end)
            i = end
        elif text.startswith("/*", i):
            end = text.find("*/", i + 2)
            end = n if end == -1 else end + 2
            blank(i, end)
            i = end
        elif c == '"':
            end = _raw_string_end(text, i)
            if end is None:
                end = _quoted_end(text, i, '"')
            blank(i, end)
            i = end
        elif c == "'" and not _is_digit_separator(text, i):
            end = _quoted_end(text, i, "'")
            blank(i, end)
            i = end
        else:
            i += 1
    return "".join(out)


def _identifier_before(text: str, i: int) -> str:
    start = i
    while start > 0 and _IDENTIFIER_CHAR.match(text[start - 1]):
        start -= 1
    return text[start:i]


def _raw_string_end(text: str, quote: int) -> int | None:
    """Returns the offset past the raw string literal whose opening quote is
    at quote, or None if that quote does not open one."""
    if _identifier_before(text, quote) not in _RAW_PREFIXES:
        return None
    open_paren = text.find("(", quote + 1)
    if open_paren == -1:
        return None
    delimiter = text[quote + 1 : open_paren]
    if len(delimiter) > 16 or any(ch in delimiter for ch in ' \\)\t\n"'):
        return None
    close = text.find(")" + delimiter + '"', open_paren + 1)
    return len(text) if close == -1 else close + len(delimiter) + 2


def _quoted_end(text: str, quote: int, delimiter: str) -> int:
    """Returns the offset past an ordinary literal; it ends at its closing
    delimiter or, unterminated, at the end of the line."""
    end = quote + 1
    while end < len(text) and text[end] not in (delimiter, "\n"):
        end += 2 if text[end] == "\\" else 1
    if end < len(text) and text[end] == delimiter:
        return end + 1
    return min(end, len(text))


def _is_digit_separator(text: str, quote: int) -> bool:
    """Returns whether the ' at quote separates digits in a number (1'000)."""
    start = quote
    while start > 0 and re.match(r"[A-Za-z0-9_.']", text[start - 1]):
        start -= 1
    return start < quote and text[start].isdigit()


@dataclass(frozen=True)
class Include:
    """One #include directive: where its name sits in the text."""

    line: int
    start: int  # text[start:end] is the name, without its delimiters.
    end: int
    name: str
    quoted: bool  # "name" rather than <name>.


def includes(text: str) -> list[Include]:
    """Returns the #include directives outside comments and literals."""
    code = blank_comments_and_literals(text)
    found = []
    for directive in _DIRECTIVE.finditer(code):
        if directive.group(1) != "include":
            continue
        name = _INCLUDE_NAME.match(text, directive.end())
        if name is None:
            continue
        group = 1 if name.group(1) is not None else 2
        found.append(
            Include(
                line=code.count("\n", 0, directive.start()) + 1,
                start=name.start(group),
                end=name.end(group),
                name=name.group(group),
                quoted=group == 2,
            )
        )
    return found


def rewrite_includes(text: str, moves: dict[str, str]) -> tuple[str, list[int]]:
    """Returns text with every include of a key of moves (a path relative to
    src/) naming its value instead, and the lines changed. Quoted and
    angle-bracket includes are both rewritten; the delimiters stay."""
    lines = []
    for include in reversed(includes(text)):
        new = moves.get(include.name)
        if new is not None:
            text = text[: include.start] + new + text[include.end :]
            lines.append(include.line)
    return text, sorted(lines)


def include_guard(path: str) -> str:
    """Returns the include guard for a header at path, relative to src/
    ("engine/base/types.h" -> "CNC_RED_ALERT_ENGINE_BASE_TYPES_H_")."""
    return "CNC_RED_ALERT_" + re.sub(r"[^A-Za-z0-9]", "_", path).upper() + "_"


def rewrite_guard(text: str, guard: str) -> tuple[str, list[int]] | None:
    """Returns text with its include guard renamed to guard, and the lines
    changed, or None if the header does not start with an #ifndef/#define pair.

    The guard is the macro of the first directive, an #ifndef followed by a
    #define of the same name; every whole-word use of it is renamed, the
    comment after #endif included.
    """
    code = blank_comments_and_literals(text)
    directives = []
    for match in list(_DIRECTIVE.finditer(code))[:2]:
        rest = code[match.end() :].split("\n", 1)[0].split()
        directives.append((match.group(1), rest[0] if rest else None))
    if (
        len(directives) < 2
        or directives[0][0] != "ifndef"
        or directives[1][0] != "define"
        or directives[0][1] is None
        or directives[0][1] != directives[1][1]
    ):
        return None
    old = directives[0][1]
    if old == guard:
        return text, []
    pattern = re.compile(rf"\b{re.escape(old)}\b")
    lines = sorted({text.count("\n", 0, m.start()) + 1 for m in pattern.finditer(text)})
    return pattern.sub(guard, text), lines


def external_target(name: str) -> tuple[bool, str | None]:
    """Returns (known, target) for an include outside the project: whether
    EXTERNAL_TARGETS or the SDL rule knows it, and the target that provides
    it (None when add_gtest() already links it)."""
    for prefix, target in EXTERNAL_TARGETS:
        if name.startswith(prefix):
            return True, target
    if is_sdl_header(name):
        return True, SDL_TARGET
    return False, None


def project_root(name: str) -> str | None:
    """Returns the folder under src/ an include names ("tech/lcw.h" -> tech),
    or None if it is not a project include."""
    root, slash, _ = name.partition("/")
    return root if slash and root in PROJECT_ROOTS else None


# --- CMake -------------------------------------------------------------------


@dataclass
class Links:
    """The link lists generated for one library."""

    public: list[str] = field(default_factory=list)
    private: list[str] = field(default_factory=list)
    tests: list[str] = field(default_factory=list)
    # Quoted includes of a library EXTERNAL_TARGETS does not know.
    unknown: list[str] = field(default_factory=list)
    # Project includes of a test that no engine or old target builds (a game
    # header): link or compile them by hand.
    unlinked: list[str] = field(default_factory=list)


def library_links(library: str, sources: dict[str, str]) -> Links:
    """Returns the link lists for library from its moved files.

    sources maps each file's new path (relative to src/) to its text, with its
    includes already rewritten.
    """
    target = layout.LIBRARIES[library]
    provided = layout.allowed_libraries(library)
    header_ext: set[str] = set()
    source_ext: set[str] = set()
    test_engine: set[str] = set()
    test_old: set[str] = set()
    test_ext: set[str] = set()
    links = Links(public=[layout.LIBRARIES[d] for d in layout.DEPENDS_ON[library]])
    for path, text in sorted(sources.items()):
        test = layout.is_test(path)
        for include in includes(text):
            root = project_root(include.name)
            if root is None:
                known, ext = external_target(include.name)
                if not known and include.quoted:
                    links.unknown.append(f"src/{path}: {include.name}")
                if ext is None:
                    continue  # A system header, or one add_gtest() links.
                if test:
                    test_ext.add(ext)
                elif path.endswith(".h"):
                    header_ext.add(ext)
                else:
                    source_ext.add(ext)
            elif test:
                # The layer checker polices the library's own includes; only
                # tests reach outside what the library links.
                folder = layout.destination_of(include.name)
                if root == layout.ENGINE_ROOT and folder is not None:
                    dependency = layout.library_of(folder)
                    if dependency not in provided:
                        test_engine.add(dependency)
                elif root in OLD_TARGETS:
                    test_old.add(OLD_TARGETS[root])
                else:
                    links.unlinked.append(f"src/{path}: {include.name}")
    links.public += sorted(header_ext)
    links.private = sorted(source_ext - header_ext)
    order = list(layout.LIBRARIES)
    links.tests = (
        [target]
        + [layout.LIBRARIES[d] for d in sorted(test_engine, key=order.index)]
        + sorted(test_old)
        + sorted(test_ext - header_ext)
    )
    return links


def _wrap(items: list[str], indent: str, width: int = 100) -> str:
    """Returns items joined by spaces and wrapped at width: the first line
    indented by indent, the lines it wraps onto by four more."""
    lines: list[str] = []
    line = indent
    for item in items:
        if line.strip() and len(line) + 1 + len(item) > width:
            lines.append(line)
            line = indent + "    " + item
        else:
            line = f"{line} {item}" if line.strip() else line + item
    lines.append(line)
    return "\n".join(lines)


def library_cmake(
    library: str, files: list[str], links: Links, optimized: list[str]
) -> str:
    """Returns src/engine/<library>/CMakeLists.txt.

    files are the library's new paths relative to src/; optimized are sources,
    relative to the library's folder, that keep optimize_in_debug().
    """
    target = layout.LIBRARIES[library]
    var = target.upper()
    folder = f"{layout.ENGINE_ROOT}/{library}/"
    rel = sorted(path.removeprefix(folder) for path in files)
    subfolders = sorted(
        sub.removeprefix(library + "/")
        for sub, parent in layout.SUBFOLDERS.items()
        if parent == library
    )
    tests = [path for path in rel if path.endswith(".cc") and layout.is_test(path)]
    static = any(p.endswith(".cc") and not layout.is_test(p) for p in rel)
    scope = "PUBLIC" if static else "INTERFACE"

    def glob(suffix: str) -> list[str]:
        return [
            f'        "${{CMAKE_CURRENT_SOURCE_DIR}}/{sub}*{suffix}"'
            for sub in [""] + [f"{sub}/" for sub in subfolders]
        ]

    where = f"src/{folder}" + "".join(f" and {sub}/" for sub in subfolders)
    out = [
        f"# {target}: {where} (tools/engine_layout.py).",
        f"# Generated by {GENERATED_BY}; edited by hand from here on.",
        "",
        f"add_library({target} {'STATIC' if static else 'INTERFACE'})",
        "",
    ]
    if static:
        out += [f"file(GLOB {var}_SOURCES CONFIGURE_DEPENDS", *glob(".cc"), ")"]
        out += [f'list(FILTER {var}_SOURCES EXCLUDE REGEX "_test(_util)?\\\\.cc$")', ""]
    out += [f"file(GLOB {var}_HEADERS CONFIGURE_DEPENDS", *glob(".h"), ")", ""]
    if static:
        out += [f"target_sources({target} PRIVATE", f"        ${{{var}_SOURCES}}", ")", ""]
    out += [
        f"target_sources({target} {scope}",
        "        FILE_SET HEADERS",
        "        BASE_DIRS ${CMAKE_SOURCE_DIR}/src",
        f"        FILES ${{{var}_HEADERS}}",
        ")",
        "",
    ]
    if SDL_TARGET in links.public + links.private + links.tests:
        out.append("find_package(SDL2 REQUIRED)")
    if links.public or links.private:
        out.append(f"target_link_libraries({target}")
        if links.public:
            out.append(_wrap([scope] + links.public, "        "))
        if links.private:
            out.append(_wrap(["PRIVATE"] + links.private, "        "))
        out += [")", ""]
    if optimized:
        out += [
            "# Byte- and pixel-crunching kernels: keep them fast when debugging",
            "# everything else (cmake/OptimizeInDebug.cmake).",
            f"optimize_in_debug({' '.join(sorted(optimized))})",
            "",
        ]
    if tests:
        out += [
            "# Tests",
            "include(Testing)",
            "add_gtest(",
            f"        NAME {target}_test",
            _wrap(["SOURCES"] + tests, "        "),
            _wrap(["LIBS"] + links.tests, "        "),
            ")",
        ]
    while out[-1] == "":
        out.pop()
    return "\n".join(out) + "\n"


def engine_cmake(generated: list[str]) -> str:
    """Returns src/engine/CMakeLists.txt, adding the folders of the generated
    libraries in dependency order."""
    order = [library for library in layout.LIBRARIES if library in generated]
    lines = [
        "# The engine libraries, in dependency order (tools/engine_layout.py).",
        f"# Maintained by {GENERATED_BY}.",
        "",
    ] + [f"add_subdirectory({library})" for library in order]
    return "\n".join(lines) + "\n"


def _entry(entry: str) -> str:
    """Returns a pattern matching entry as a whole CMake word."""
    return rf"(?<![\w./-]){re.escape(entry)}(?![\w./-])"


_CALL = r"^[ \t]*{name}\((?P<args>[^)]*)\)[ \t]*(?:\n|$)"


def _delete_block(text: str, start: int, end: int) -> str:
    """Returns text without the whole lines text[start:end] and the comment
    lines directly above them, leaving one blank line between neighbors."""
    lines = text[:start].split("\n")[:-1]
    while lines and lines[-1].lstrip().startswith("#"):
        lines.pop()
    head = "\n".join(lines).rstrip("\n")
    tail = text[end:].lstrip("\n")
    if head and tail:
        return head + "\n\n" + tail
    return (head + "\n") if head else tail


def remove_cmake_entries(
    text: str, entries: list[str]
) -> tuple[str, list[str], list[str]]:
    """Returns (text, optimized, deleted_tests): text with each entry (a file
    name relative to the CMakeLists.txt) taken out of its lists, the entries
    that were in an optimize_in_debug() call, and the names of the add_gtest()
    targets deleted because no *_test.cc was left in their SOURCES. Comment
    lines are left alone."""
    call = re.compile(_CALL.format(name="optimize_in_debug"), re.M)
    optimized = [
        entry
        for match in call.finditer(text)
        for entry in entries
        if re.search(_entry(entry), match.group("args"))
    ]
    kept = []
    for line in text.split("\n"):
        new = line
        if not line.lstrip().startswith("#"):
            for entry in entries:
                new = re.sub(_entry(entry) + r"[ \t]*", "", new)
        if new != line:
            if not new.strip():
                continue
            new = re.sub(r"[ \t]+\)", ")", new.rstrip())
        kept.append(new)
    text = "\n".join(kept)

    for match in reversed(list(call.finditer(text))):
        if not match.group("args").strip():
            text = _delete_block(text, match.start(), match.end())

    deleted = []
    gtest = re.compile(_CALL.format(name="add_gtest"), re.M)
    for match in reversed(list(gtest.finditer(text))):
        args = match.group("args")
        sources = re.search(r"\bSOURCES\b(.*?)(?:\bLIBS\b|$)", args, re.S)
        if sources and not re.search(r"_test\.cc\b", sources.group(1)):
            name = re.search(r"\bNAME\s+(\S+)", args)
            deleted.append(name.group(1) if name else "?")
            text = _delete_block(text, match.start(), match.end())
    return text, optimized, sorted(deleted)


def ensure_engine_subdirectory(text: str) -> str:
    """Returns the top-level CMakeLists.txt with add_subdirectory(src/engine)
    ahead of the first add_subdirectory(src/...)."""
    if re.search(r"^add_subdirectory\(src/engine\)", text, re.M):
        return text
    first = re.search(r"^add_subdirectory\(src/", text, re.M)
    if first is None:
        raise ValueError("no add_subdirectory(src/...) in the top-level CMakeLists.txt")
    return text[: first.start()] + "add_subdirectory(src/engine)\n" + text[first.start() :]


# --- The move ----------------------------------------------------------------


def _read(path: Path) -> str:
    # Bytes in, bytes out: keeps CRLF files and non-UTF-8 legacy files intact.
    return path.read_bytes().decode("utf-8", "surrogateescape")


def _write(path: Path, text: str) -> None:
    path.write_bytes(text.encode("utf-8", "surrogateescape"))


def _git(*args: str) -> None:
    subprocess.run(["git", *args], cwd=REPO_ROOT, check=True)


def _cpp_files() -> list[Path]:
    return sorted(
        path
        for root in REWRITE_ROOTS
        for path in (REPO_ROOT / root).rglob("*")
        if path.is_file() and path.suffix in CPP_SUFFIXES
    )


def emptied_roots(old_files: list[str], moved: list[str]) -> list[str]:
    """Returns the old roots (src/base, ...) that hold source files in
    old_files, the tree before the move, and none once moved leave them."""
    before = {path.split("/", 1)[0] for path in old_files}
    after = {path.split("/", 1)[0] for path in set(old_files) - set(moved)}
    return [root for root in layout.OLD_ROOTS if root in before - after]


def _owner_cmake(old: str) -> Path | None:
    """Returns the CMakeLists.txt that builds src/old: the nearest one in its
    folder or above, inside its old root."""
    folder = (SRC_ROOT / old).parent
    root = SRC_ROOT / old.split("/", 1)[0]
    while True:
        if (folder / "CMakeLists.txt").is_file():
            return folder / "CMakeLists.txt"
        if folder == root:
            return None
        folder = folder.parent


def _mentions(moves: dict[str, str]) -> list[str]:
    """Returns file:line for each remaining mention of a moved file's old
    path, with or without its extension, in the C++ sources, the CMake files
    and CLAUDE.md: the comments the include rewrite leaves alone."""
    stems = sorted({old.rsplit(".", 1)[0] for old in moves}, key=len, reverse=True)
    pattern = re.compile(
        r"(?<![\w/.-])(?:" + "|".join(re.escape(s) for s in stems) + r")(?![\w/])"
    )
    files = _cpp_files() + sorted(SRC_ROOT.glob("**/CMakeLists.txt"))
    files += sorted((REPO_ROOT / "cmake").glob("*.cmake"))
    files += [REPO_ROOT / "CMakeLists.txt", REPO_ROOT / "CLAUDE.md"]
    found = []
    for path in files:
        if not path.is_file():
            continue
        for number, line in enumerate(_read(path).split("\n"), 1):
            if pattern.search(line):
                found.append(f"{path.relative_to(REPO_ROOT)}:{number}: {line.strip()}")
    return found


def move_library(library: str, dry_run: bool) -> None:
    """Runs one step for library; see the module docstring."""
    folders = [library] + [s for s, p in layout.SUBFOLDERS.items() if p == library]
    olds = sorted(path for f in folders for path in layout.files_for(f, SRC_ROOT))
    if not olds:
        sys.exit(f"{library}: nothing left to move (tools/engine_layout.py)")
    moves = {old: layout.new_path(old) for old in olds}
    engine_dir = SRC_ROOT / layout.ENGINE_ROOT
    library_file = engine_dir / library / "CMakeLists.txt"
    for path in [*(SRC_ROOT / new for new in moves.values()), library_file]:
        if path.exists():
            sys.exit(f"{path.relative_to(REPO_ROOT)} already exists")

    print(f"== {library} ({layout.LIBRARIES[library]}): {len(moves)} files")
    for old, new in moves.items():
        print(f"  src/{old} -> src/{new}")

    # Everything that can fail is worked out before the tree changes.
    texts = {
        new: rewrite_includes(_read(SRC_ROOT / old), moves)[0]
        for old, new in moves.items()
    }
    links = library_links(library, texts)
    if links.unknown:
        sys.exit(
            "add these includes to EXTERNAL_TARGETS in tools/move_engine_files.py:\n  "
            + "\n  ".join(links.unknown)
        )
    # An old root the move empties is deleted whole, with its add_subdirectory()
    # and CMakeLists.txt files, so those are not edited. git rm refuses files
    # with local changes, so check for them now rather than halfway through.
    emptied = emptied_roots(layout.old_files(SRC_ROOT), olds)
    for root in emptied:
        dirty = subprocess.run(
            ["git", "status", "--porcelain", "--", f"src/{root}"],
            cwd=REPO_ROOT, check=True, capture_output=True, text=True,
        ).stdout.split("\n")
        dirty = [line for line in dirty if line and line[3:] not in {f"src/{o}" for o in olds}]
        if dirty:
            sys.exit(f"src/{root} would be deleted but has local changes:\n" + "\n".join(dirty))
    owners: dict[Path, list[str]] = {}
    for old in olds:
        owner = _owner_cmake(old)
        if owner is not None:
            entry = (SRC_ROOT / old).relative_to(owner.parent).as_posix()
            owners.setdefault(owner, []).append(entry)
    cmake_edits: dict[Path, str] = {}
    optimized: list[str] = []
    deleted_tests: list[str] = []
    for owner, entries in owners.items():
        text, carried, deleted = remove_cmake_entries(_read(owner), entries)
        if owner.relative_to(SRC_ROOT).parts[0] not in emptied:
            cmake_edits[owner] = text
            deleted_tests += deleted
        for entry in carried:
            old = (owner.parent / entry).relative_to(SRC_ROOT).as_posix()
            optimized.append(moves[old].removeprefix(f"{layout.ENGINE_ROOT}/{library}/"))
    generated = library_cmake(library, list(moves.values()), links, optimized)
    if dry_run:
        for root in emptied:
            print(f"  src/{root} would be empty and deleted")
        print(f"-- {library_file.relative_to(REPO_ROOT)}\n{generated}")
        return

    for old, new in moves.items():
        (SRC_ROOT / new).parent.mkdir(parents=True, exist_ok=True)
        _git("mv", f"src/{old}", f"src/{new}")
    # git mv leaves the folders it empties (src/port/win32) behind.
    for old in olds:
        folder = (SRC_ROOT / old).parent
        while folder != SRC_ROOT and folder.is_dir() and not any(folder.iterdir()):
            folder.rmdir()
            folder = folder.parent

    changed: dict[Path, list[int]] = {}
    for path in _cpp_files():
        text, lines = rewrite_includes(_read(path), moves)
        if lines:
            _write(path, text)
            changed[path] = lines
    for new in moves.values():
        if not new.endswith(".h"):
            continue
        path = SRC_ROOT / new
        result = rewrite_guard(_read(path), include_guard(new))
        if result is None:
            print(f"  warning: src/{new} has no #ifndef/#define include guard")
        elif result[1]:
            _write(path, result[0])
            changed[path] = sorted(set(changed.get(path, []) + result[1]))

    for owner, text in cmake_edits.items():
        _write(owner, text)
    _write(library_file, generated)
    built = [lib for lib in layout.LIBRARIES if (engine_dir / lib / "CMakeLists.txt").is_file()]
    _write(engine_dir / "CMakeLists.txt", engine_cmake(built))
    top = REPO_ROOT / "CMakeLists.txt"
    top_text = ensure_engine_subdirectory(_read(top))
    for root in emptied:
        _git("rm", "-r", "-q", f"src/{root}")
        if (SRC_ROOT / root).is_dir() and not any(
            path.is_file() for path in (SRC_ROOT / root).rglob("*")
        ):
            shutil.rmtree(SRC_ROOT / root)
        top_text = re.sub(rf"^add_subdirectory\(src/{root}\)\n", "", top_text, flags=re.M)
    _write(top, top_text)

    for path, lines in sorted(changed.items()):
        subprocess.run(
            ["clang-format", "-i", *(f"--lines={n}:{n}" for n in lines), str(path)],
            cwd=REPO_ROOT,
            check=True,
        )

    print(f"  rewrote includes or guards in {len(changed)} files")
    print(f"  wrote {library_file.relative_to(REPO_ROOT)}")
    for owner in cmake_edits:
        print(f"  edited {owner.relative_to(REPO_ROOT)}")
    for root in emptied:
        print(f"  deleted src/{root}: nothing left in it; drop links to `{OLD_TARGETS[root]}`")
    for name in deleted_tests:
        print(f"  deleted add_gtest({name}): recreate it by hand if it was a split target")
    for entry in links.unlinked:
        print(f"  not linked into the test, add by hand: {entry}")
    mentions = _mentions(moves)
    if mentions:
        print(f"  {len(mentions)} mentions of the old paths remain (comments, CMake, docs):")
        for mention in mentions:
            print(f"    {mention}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        "libraries",
        nargs="+",
        choices=list(layout.LIBRARIES),
        metavar="library",
        help="the library folders to move, in order (base, stream, video/vqa, ...)",
    )
    parser.add_argument(
        "--dry-run", action="store_true", help="print the plan, change nothing"
    )
    args = parser.parse_args()
    for library in args.libraries:
        move_library(library, args.dry_run)
    return 0


if __name__ == "__main__":
    sys.exit(main())
