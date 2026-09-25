"""The engine's folder layout: where each file lives, and who may include whom.

docs/ENGINE_FOLDERS_PLAN.md moved the shared code that used to live in
src/base, src/port, src/sdllib, src/tech and src/winvq under src/engine/, one
folder per domain. This module is the one copy of that plan's library list
and dependency order; tools/check_layers.py imports it to enforce them.

Paths here are relative to src/ and always use '/'. A "folder" is a directory
under src/engine/ ("gfx", "base/strings"); a "library" is the CMake target
that builds it. Subfolders belong to their parent's library, except
video/vqa, which is a library of its own. A file's folder is just its path
under src/engine/ -- ra/, td/, tools/ and testing/ hold no engine code.
"""

from __future__ import annotations

# Where engine code lives, relative to src/.
ENGINE_ROOT = "engine"

# Each library's folder and its CMake target.
LIBRARIES = {
    "base": "engine_base",
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
    """Returns every folder under engine/, libraries and subfolders."""
    return tuple(LIBRARIES) + tuple(SUBFOLDERS)


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
    """Returns the engine folder path belongs in, or None if it has none.

    path is relative to src/. Only files under src/engine/ are engine code;
    a file in a known folder there ("engine/gfx/rgb.h" -> "gfx") is placed by
    its own path, a file in an unknown one and everything else (the games,
    tools, testing) returns None.
    """
    folder, _, _ = path.rpartition("/")
    if folder == ENGINE_ROOT or folder.startswith(ENGINE_ROOT + "/"):
        candidate = folder.removeprefix(ENGINE_ROOT + "/")
        return candidate if candidate in folders() else None
    return None
