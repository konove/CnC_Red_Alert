"""The engine's folder layout: where each file goes, and who may include whom.

docs/ENGINE_FOLDERS_PLAN.md moves the shared code in src/base, src/port,
src/sdllib, src/tech and src/winvq under src/engine/, one folder per domain.
This module is the one copy of that plan's file map and dependency order.
tools/check_layers.py imports it, and so does phase B's move script
(tools/move_engine_files.py), so the folders the checker enforces and the
folders the files land in cannot drift.

Paths here are relative to src/ and always use '/'. A "folder" is a directory
under src/engine/ ("gfx", "base/strings"); a "library" is the CMake target that
builds it. Subfolders belong to their parent's library, except video/vqa, which
is a library of its own.

The move script reads FILE_MAP (or files_for()) for the destination it moves,
and new_path() for where each file lands. Once every file has moved, the map
goes and a file's folder is just its path under engine/.
"""

from __future__ import annotations

from pathlib import Path

# The folders that hold engine code before the move, relative to src/.
OLD_ROOTS = ("base", "port", "sdllib", "tech", "winvq")

# Where engine code lives after the move, relative to src/.
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

# Matches every file of a current folder (non-recursively).
ALL = "*"

# The file map: destination folder -> current folder -> the stems it sends
# there. A stem covers <stem>.h, <stem>.cc and its test <stem>_test.cc (see
# entry_stems()), so a test moves with the file it tests; a test with no file
# of its own (the stream round-trip tests) is listed by its own stem. File
# names do not change, only folders.
FILE_MAP: dict[str, dict[str, tuple[str, ...]]] = {
    "base": {
        "base": (
            "algorithm", "array", "attributes", "buffer", "clip",
            "enum_array", "flags", "hsv", "installed", "numeric", "trig",
            "types",
        ),
        "port": ("aligned_buffer", "bytes_of", "unaligned"),
        "tech": ("buff", "fixed", "listnode", "random"),
    },
    "base/strings": {
        "port": ("format", "safe_string", "tokenizer"),
        "tech": ("number_parse",),
    },
    "platform": {
        "port": ("env", "platform", "random_seed", "sleep"),
        "sdllib": ("file_system", "mem", "memflag", "timer"),
        "tech": ("ftimer",),
    },
    "platform/win32": {
        "port/win32": (
            "win32_com", "win32_registry", "win32_system", "win32_types",
        ),
    },
    "stream": {
        "base": ("seek_origin",),
        "tech": (
            "archive", "byte_codec", "byte_sink", "byte_source",
            "byte_stream", "memory_stream", "range_stream", "readline",
            "span_sink", "span_source", "stream_sink", "stream_source",
            "tee_sink", "transform_sink", "transform_source", "vector_sink",
        ),
    },
    "codec": {
        "sdllib": (
            "aud_decoder", "compressed_block", "lcw_uncompress", "xor_delta",
        ),
        "tech": (
            "base64", "base64_codec", "base64_sink", "base64_source",
            "block_backends", "block_codec", "lcw", "lcw_sink", "lcw_source",
            "lzo_sink", "lzo_source", "lzw", "lzw_sink", "lzw_source",
            # Tests of the codecs through the streams.
            "codec_corrupt", "codec_state", "lcw_comp", "stream_error",
            "stream_golden",
        ),
    },
    "crypto": {
        "tech": (
            "blowfish", "blowfish_codec", "blowfish_sink", "blowfish_source",
            "crc", "digit_cursor", "int", "key_phrase_hash", "mp", "pk",
            "pk_sink", "pk_source", "random_source", "sha", "sha1_codec",
            "sha1_compress", "sha1_sink", "sha1_source",
            # Tests of the ciphers through the streams.
            "blowfish_stream", "pk_stream",
        ),
    },
    "file": {
        "port": ("profile_buffer",),
        "sdllib": ("string_table",),
        "tech": (
            "disk_file", "disk_stream", "file_access", "game_file",
            "mix_archive", "search_paths",
        ),
    },
    "net": {
        "port": ("inet_text", "socket_bytes"),
        "sdllib": ("net_select",),
        "tech": ("field", "packet"),
    },
    "net/serial": {
        "sdllib": ("modemreg", "wincomm"),
    },
    "gfx": {
        "sdllib": (
            "bitmap", "font", "pixel_buffer", "shape", "stamp", "tile",
            "wwstd",
            # Split out of the window in phase A.
            "fading_table", "pixel_surface", "text_window",
            # Decodes keyframes into a buffer; its Display tests went to
            # display_test.
            "keyframe",
        ),
        "tech": (
            "2keyfbuf", "glow_pulse", "hsv", "pcx_file", "rect", "rgb",
            "wsa_animation",
        ),
    },
    "audio": {
        "tech": ("audio_mixer",),
    },
    "window": {
        "sdllib": (
            "display", "display_palette", "keyboard", "misc", "ww_mouse",
            "ww_win",
        ),
    },
    "video": {
        "tech": ("game_file_vqa_io", "mixer_vqa_audio"),
    },
    "video/vqa": {
        "winvq/vqa32": (ALL,),
    },
}

# The suffixes of the files the checker and the move script handle. Anything
# else under the old roots (CMakeLists.txt, notes) is not code and is not
# mapped; each new folder gets its own CMakeLists.txt.
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


def is_old_path(path: str) -> bool:
    """Returns whether path lies in one of the folders the plan empties."""
    return path.split("/", 1)[0] in OLD_ROOTS


def _stem(path: str) -> str:
    name = path.rsplit("/", 1)[-1]
    return name.rsplit(".", 1)[0]


def entry_stems(name: str) -> list[str]:
    """Returns the FILE_MAP stems that could name a file: its own stem, and
    for a test the stem of the file it tests ("lcw_test.cc" -> lcw_test, lcw).
    """
    stem = _stem(name)
    return [stem] + [
        stem.removesuffix(suffix)
        for suffix in _TEST_SUFFIXES
        if stem.endswith(suffix)
    ]


def _map_index() -> dict[tuple[str, str], str]:
    """Returns (current folder, stem) -> destination folder."""
    index: dict[tuple[str, str], str] = {}
    for destination, sources in FILE_MAP.items():
        for old_folder, stems in sources.items():
            for stem in stems:
                key = (old_folder, stem)
                if key in index:
                    raise ValueError(
                        f"{old_folder}/{stem} is mapped to both "
                        f"{index[key]} and {destination}"
                    )
                index[key] = destination
    return index


_INDEX = _map_index()


def destination_of(path: str) -> str | None:
    """Returns the engine folder path belongs in, or None if it has none.

    path is relative to src/. A file already under engine/ belongs where it
    is (None if that is not a known folder); a file in an old root is looked
    up in FILE_MAP by its own stem, then as a test by the stem it tests.
    Anything else (the games, tools) is not engine code and returns None.
    """
    old_folder, _, name = path.rpartition("/")
    if old_folder == ENGINE_ROOT or old_folder.startswith(ENGINE_ROOT + "/"):
        folder = old_folder.removeprefix(ENGINE_ROOT + "/")
        return folder if folder in folders() else None
    if not is_old_path(path):
        return None
    for candidate in (*entry_stems(name), ALL):
        destination = _INDEX.get((old_folder, candidate))
        if destination is not None:
            return destination
    return None


def new_path(path: str) -> str:
    """Returns where path lands under engine/ ("tech/rgb.h" ->
    "engine/gfx/rgb.h"). Raises KeyError if the map has no place for it."""
    destination = destination_of(path)
    if destination is None:
        raise KeyError(f"{path} is not in the engine file map")
    return f"{ENGINE_ROOT}/{destination}/{path.rsplit('/', 1)[-1]}"


def files_for(destination: str, src_root: Path) -> list[str]:
    """Returns the files still in the old roots that FILE_MAP sends to
    destination, relative to src_root and sorted."""
    return sorted(
        path
        for old_folder in FILE_MAP[destination]
        for path in _source_files(src_root / old_folder, src_root)
        if destination_of(path) == destination
    )


def old_files(src_root: Path) -> list[str]:
    """Returns every source file in the old roots, relative to src_root."""
    return sorted(
        path
        for root in OLD_ROOTS
        for path in _source_files(src_root / root, src_root, recursive=True)
    )


def _source_files(
    directory: Path, src_root: Path, recursive: bool = False
) -> list[str]:
    if not directory.is_dir():
        return []
    pattern = "**/*" if recursive else "*"
    return [
        path.relative_to(src_root).as_posix()
        for path in directory.glob(pattern)
        if path.is_file() and is_source(path.name)
    ]
