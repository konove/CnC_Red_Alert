"""The engine's folder layout: where each file lives, and who may include whom.

docs/ENGINE_FOLDERS_PLAN.md moved the shared code that used to live in
src/base, src/port, src/sdllib, src/tech and src/winvq under src/engine/, one
folder per domain; docs/ENGINE_NAMESPACES_PLAN.md then moved the vocabulary
library, base, back out to src/base/, beside the engine rather than in it.
This module is the one copy of the library list and dependency order;
tools/check_layers.py imports it to enforce them.

Paths here are relative to src/ and always use '/'. A "folder" names a
library's directory or a subdirectory of one ("gfx", "base/strings"); a
"library" is the CMake target that builds it. Folders live under src/engine/,
except those of TOP_LEVEL_LIBRARIES, which live at the top of src/
(folder_path()). Subfolders belong to their parent's library, except
video/vqa, which is a library of its own. ra/, td/, tools/ and testing/ hold
no library code.
"""

from __future__ import annotations

# Where engine code lives, relative to src/.
ENGINE_ROOT = "engine"

# The libraries at the top of src/ rather than under engine/: the vocabulary
# the engine, the games and the tools all share.
TOP_LEVEL_LIBRARIES = ("base",)

# The directories under src/ that hold library code.
LIBRARY_ROOTS = (ENGINE_ROOT, *TOP_LEVEL_LIBRARIES)

# Each library's folder and its CMake target.
LIBRARIES = {
    "base": "base",
    "platform": "engine_platform",
    "stream": "engine_stream",
    "codec": "engine_codec",
    "crypto": "engine_crypto",
    "file": "engine_file",
    "net": "engine_net",
    "gfx": "engine_gfx",
    "audio": "engine_audio",
    "window": "engine_window",
    "video/vqa": "engine_vqa",
    "video": "engine_video",
}

# Folders compiled into their parent's library rather than one of their own.
SUBFOLDERS = {
    "base/strings": "base",
    "platform/win32": "platform",
    "net/serial": "net",
}

# The plan's "Dependency order": each library and the libraries it points at.
# A library may include what it points at and what those point at in turn;
# allowed_libraries() takes that closure. Games sit above all of it and are
# not listed: no engine library may include them.
DEPENDS_ON = {
    "base": (),
    "platform": ("base",),
    "stream": ("platform",),
    "codec": ("stream",),
    "crypto": ("stream",),
    "file": ("codec", "crypto"),
    "net": ("platform",),
    "gfx": ("file",),
    "window": ("gfx",),
    "audio": ("file", "codec"),
    "video/vqa": ("stream", "codec"),
    "video": ("file", "audio", "video/vqa"),
}

# The libraries whose files may include SDL headers, and the single file
# outside them that may. Everything else, gfx included, compiles without SDL.
SDL_LIBRARIES = ("window", "audio", "video", "video/vqa")
SDL_FILES = ("platform/timer.cc",)

# The suffixes of the files the checker handles. Anything else under
# src/engine/ (CMakeLists.txt, notes) is not code and is not checked.
SOURCE_SUFFIXES = (".h", ".cc")

_TEST_SUFFIXES = ("_test", "_test_util")


def library_of(folder: str) -> str:
    """Returns the library that builds folder ("net/serial" -> "net")."""
    return SUBFOLDERS.get(folder, folder)


def folders() -> tuple[str, ...]:
    """Returns every folder, libraries and subfolders."""
    return tuple(LIBRARIES) + tuple(SUBFOLDERS)


def folder_path(folder: str) -> str:
    """Returns where folder lives, relative to src/ ("gfx" -> "engine/gfx",
    "base/strings" -> "base/strings")."""
    if library_of(folder).split("/", 1)[0] in TOP_LEVEL_LIBRARIES:
        return folder
    return f"{ENGINE_ROOT}/{folder}"


def allowed_libraries(library: str) -> frozenset[str]:
    """Returns the libraries library may include: itself and its closure."""
    allowed = {library}
    pending = list(DEPENDS_ON[library])
    while pending:
        dependency = pending.pop()
        if dependency not in allowed:
            allowed.add(dependency)
            pending.extend(DEPENDS_ON[dependency])
    return frozenset(allowed)


def is_source(path: str) -> bool:
    """Returns whether path is a C++ file the layout covers."""
    return path.endswith(SOURCE_SUFFIXES)


def is_test(path: str) -> bool:
    """Returns whether path is a test or a test-only helper."""
    return _stem(path).endswith(_TEST_SUFFIXES)


def _stem(path: str) -> str:
    name = path.rsplit("/", 1)[-1]
    return name.rsplit(".", 1)[0]


def library_folder_of(path: str) -> str | None:
    """Returns the folder path belongs in, or None if it has none.

    path is relative to src/. A file directly in a known folder
    ("engine/gfx/rgb.h" -> "gfx", "base/strings/format.h" -> "base/strings")
    is placed by its own path; a file in an unknown one and everything else
    (the games, tools, testing) returns None.
    """
    directory, _, _ = path.rpartition("/")
    for folder in folders():
        if folder_path(folder) == directory:
            return folder
    return None
