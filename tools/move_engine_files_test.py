#!/usr/bin/env python3
"""Tests for the pure parts of tools/move_engine_files.py: reading includes
past comments and literals, rewriting them, include guards, and the CMake it
edits and generates.

Usage:
  tools/move_engine_files_test.py

Runs as the engine_move_script_test ctest.
"""

import shutil
import subprocess
import tempfile
import unittest
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path
from unittest import mock

import move_engine_files as move

MOVES = {
    "base/types.h": "engine/base/types.h",
    "port/format.h": "engine/base/strings/format.h",
}


class BlankTest(unittest.TestCase):
    def test_comments_and_literals_become_spaces(self):
        text = 'a // c\nb /* x\ny */ "s//" \'"\' c\n'
        blanked = move.blank_comments_and_literals(text)
        self.assertEqual(len(blanked), len(text))
        self.assertEqual(blanked.count("\n"), text.count("\n"))
        self.assertEqual(blanked.split(), ["a", "b", "c"])

    def test_raw_strings_are_literals(self):
        text = 'auto s = R"x(\n#include "base/types.h"\n)x";\nint y;\n'
        blanked = move.blank_comments_and_literals(text)
        self.assertNotIn("#include", blanked)
        self.assertIn("int y;", blanked)

    def test_digit_separators_open_no_literal(self):
        text = "int n = 1'000;\n#include \"base/types.h\"\n"
        self.assertIn("#include", move.blank_comments_and_literals(text))

    def test_continued_line_comment(self):
        text = '// a \\\n#include "base/types.h"\nint x;\n'
        blanked = move.blank_comments_and_literals(text)
        self.assertNotIn("#include", blanked)
        self.assertIn("int x;", blanked)


class RewriteIncludesTest(unittest.TestCase):
    def test_quoted_and_angle_includes_are_rewritten(self):
        text = (
            '#include "port/format.h"\n'
            "#include <base/types.h>  // For ssize.\n"
            '#include "base/types_extra.h"\n'
            "#include <vector>\n"
        )
        new, lines = move.rewrite_includes(text, MOVES)
        self.assertEqual(
            new,
            '#include "engine/base/strings/format.h"\n'
            "#include <engine/base/types.h>  // For ssize.\n"
            '#include "base/types_extra.h"\n'
            "#include <vector>\n",
        )
        self.assertEqual(lines, [1, 2])

    def test_comments_and_strings_are_left_alone(self):
        text = (
            '// #include "base/types.h"\n'
            '/* #include "base/types.h"\n'
            '#include "base/types.h" */\n'
            'const char* kInclude = "#include \\"base/types.h\\"";\n'
            '  #  include "base/types.h"\n'
            'const char* kPath = "base/types.h";  // See base/types.h.\n'
        )
        new, lines = move.rewrite_includes(text, MOVES)
        self.assertEqual(lines, [5])
        self.assertEqual(
            new, text.replace('  #  include "base/types.h"', '  #  include "engine/base/types.h"')
        )

    def test_included_quoting_is_reported(self):
        found = move.includes('#include <SDL.h>\n#include "absl/log/check.h"\n')
        self.assertEqual([(i.name, i.quoted) for i in found],
                         [("SDL.h", False), ("absl/log/check.h", True)])


class GuardTest(unittest.TestCase):
    def test_guard_follows_the_path(self):
        self.assertEqual(
            move.include_guard("engine/base/strings/format.h"),
            "CNC_RED_ALERT_ENGINE_BASE_STRINGS_FORMAT_H_",
        )
        self.assertEqual(
            move.include_guard("engine/gfx/2keyfbuf.h"),
            "CNC_RED_ALERT_ENGINE_GFX_2KEYFBUF_H_",
        )

    def test_guard_is_renamed_everywhere(self):
        text = (
            "// Formats. CNC_RED_ALERT_PORT_FORMAT_H_X stays.\n"
            "#ifndef CNC_RED_ALERT_PORT_FORMAT_H_\n"
            "#define CNC_RED_ALERT_PORT_FORMAT_H_\n"
            "int f();\n"
            "#endif  // CNC_RED_ALERT_PORT_FORMAT_H_\n"
        )
        new, lines = move.rewrite_guard(text, "NEW_H_")
        self.assertEqual(lines, [2, 3, 5])
        self.assertEqual(
            new,
            "// Formats. CNC_RED_ALERT_PORT_FORMAT_H_X stays.\n"
            "#ifndef NEW_H_\n#define NEW_H_\nint f();\n#endif  // NEW_H_\n",
        )

    def test_header_without_guard(self):
        self.assertIsNone(move.rewrite_guard("#pragma once\nint f();\n", "G_"))
        self.assertIsNone(move.rewrite_guard("#ifndef A\n#define B\n", "G_"))
        self.assertIsNone(move.rewrite_guard("#include <x>\n#ifndef A\n#define A\n", "G_"))

    def test_guard_already_right(self):
        text = "#ifndef G_\n#define G_\n#endif  // G_\n"
        self.assertEqual(move.rewrite_guard(text, "G_"), (text, []))


class CMakeEditTest(unittest.TestCase):
    OLD = (
        "add_library(tech STATIC)\n"
        "\n"
        "# Fast kernels.\n"
        "optimize_in_debug(mp.cc lcw.cc)\n"
        "\n"
        "include(Testing)\n"
        "add_gtest(\n"
        "    NAME tech_test\n"
        "    SOURCES fixed_test.cc lcw_test.cc\n"
        "        mp_test.cc\n"
        "    LIBS tech\n"
        ")\n"
        "\n"
        "# Compiles lcw.cc directly.\n"
        "add_gtest(\n"
        "    NAME lcw_only_test\n"
        "    SOURCES lcw_only_test.cc lcw.cc sdllib/lcw.cc\n"
        "    LIBS base\n"
        ")\n"
        "\n"
        "# Something after.\n"
    )

    def test_entries_leave_lists_and_calls(self):
        text, optimized, deleted = move.remove_cmake_entries(
            self.OLD, ["lcw.cc", "mp_test.cc", "fixed_test.cc"]
        )
        self.assertEqual(optimized, ["lcw.cc"])
        self.assertEqual(deleted, [])
        self.assertIn("optimize_in_debug(mp.cc)\n", text)
        self.assertIn("    SOURCES lcw_test.cc\n    LIBS tech\n", text)
        # Whole words only, and comments stay.
        self.assertIn("SOURCES lcw_only_test.cc sdllib/lcw.cc\n", text)
        self.assertIn("# Compiles lcw.cc directly.\n", text)

    def test_emptied_calls_are_deleted_with_their_comment(self):
        text, optimized, deleted = move.remove_cmake_entries(
            self.OLD,
            ["mp.cc", "lcw.cc", "lcw_only_test.cc", "fixed_test.cc", "lcw_test.cc",
             "mp_test.cc"],
        )
        self.assertEqual(sorted(optimized), ["lcw.cc", "mp.cc"])
        self.assertEqual(deleted, ["lcw_only_test", "tech_test"])
        self.assertEqual(
            text,
            "add_library(tech STATIC)\n\ninclude(Testing)\n\n# Something after.\n",
        )

    def test_engine_subdirectory_is_added_once(self):
        top = "project(x)\n\nadd_subdirectory(src/base)\nadd_subdirectory(src/ra)\n"
        once = move.ensure_engine_subdirectory(top)
        self.assertEqual(
            once,
            "project(x)\n\nadd_subdirectory(src/engine)\nadd_subdirectory(src/base)\n"
            "add_subdirectory(src/ra)\n",
        )
        self.assertEqual(move.ensure_engine_subdirectory(once), once)

    def test_engine_cmake_is_in_dependency_order(self):
        self.assertTrue(
            move.engine_cmake(["video", "base", "video/vqa", "stream"]).endswith(
                "add_subdirectory(base)\nadd_subdirectory(stream)\n"
                "add_subdirectory(video/vqa)\nadd_subdirectory(video)\n"
            )
        )


class GenerateTest(unittest.TestCase):
    def test_links_follow_the_includes(self):
        sources = {
            "engine/stream/archive.h": '#include "absl/log/check.h"\n#include <cstdint>\n',
            "engine/stream/archive.cc": (
                '#include "engine/stream/archive.h"\n'
                '#include "absl/strings/str_cat.h"\n#include <SDL.h>\n'
            ),
            "engine/stream/archive_test.cc": (
                '#include "engine/gfx/rgb.h"\n#include "engine/base/types.h"\n'
                '#include "tech/lcw.h"\n#include "ra/keyframe.h"\n'
                '#include "gtest/gtest.h"\n#include "absl/log/check.h"\n'
                '#include "absl/types/span.h"\n'
            ),
        }
        links = move.library_links("stream", sources)
        self.assertEqual(links.public, ["engine_platform", "absl::check"])
        self.assertEqual(links.private, ["SDL2::SDL2", "absl::strings"])
        # engine_base comes through engine_platform; absl::check through the
        # library's headers.
        self.assertEqual(links.tests, ["engine_stream", "engine_gfx", "tech", "absl::span"])
        self.assertEqual(links.unlinked, ["src/engine/stream/archive_test.cc: ra/keyframe.h"])
        self.assertEqual(links.unknown, [])

    def test_unknown_quoted_external_is_reported(self):
        links = move.library_links(
            "base",
            {"engine/base/x.h": '#include "absl/hash/hash.h"\n#include <windows.h>\n'},
        )
        self.assertEqual(links.unknown, ["src/engine/base/x.h: absl/hash/hash.h"])

    def test_static_library_with_subfolder(self):
        files = [
            "engine/base/types.h",
            "engine/base/fixed.cc",
            "engine/base/fixed_test.cc",
            "engine/base/strings/format_test.cc",
        ]
        links = move.Links(public=["absl::check"], private=["absl::log"], tests=["engine_base"])
        text = move.library_cmake("base", files, links, ["fixed.cc"])
        self.assertIn("add_library(engine_base STATIC)\n", text)
        self.assertIn('"${CMAKE_CURRENT_SOURCE_DIR}/strings/*.cc"\n', text)
        self.assertIn("target_sources(engine_base PUBLIC\n", text)
        self.assertIn("        PUBLIC absl::check\n        PRIVATE absl::log\n", text)
        self.assertIn("optimize_in_debug(fixed.cc)\n", text)
        self.assertIn("        NAME engine_base_test\n", text)
        self.assertIn("        SOURCES fixed_test.cc strings/format_test.cc\n", text)
        self.assertNotIn("find_package(SDL2", text)

    def test_test_helpers_are_test_sources(self):
        files = ["engine/video/vqa/movie.cc", "engine/video/vqa/movie_test.cc",
                 "engine/video/vqa/vqa_test_util.cc", "engine/video/vqa/vqa_test_util.h"]
        links = move.Links(tests=["engine_vqa"])
        text = move.library_cmake("video/vqa", files, links, [])
        self.assertIn('EXCLUDE REGEX "_test(_util)?\\\\.cc$"', text)
        self.assertIn("SOURCES movie_test.cc vqa_test_util.cc\n", text)

    def test_emptied_roots(self):
        old = ["port/a.h", "port/b.cc", "tech/c.h", "winvq/vqa32/d.cc"]
        self.assertEqual(move.emptied_roots(old, ["port/a.h"]), [])
        self.assertEqual(
            move.emptied_roots(old, ["port/a.h", "port/b.cc", "winvq/vqa32/d.cc"]),
            ["port", "winvq"],
        )

    def test_header_only_library_is_interface(self):
        links = move.Links(public=["engine_stream", "SDL2::SDL2"], tests=["engine_video"])
        text = move.library_cmake("video", ["engine/video/glue.h"], links, [])
        self.assertIn("add_library(engine_video INTERFACE)\n", text)
        self.assertNotIn("_SOURCES", text)
        self.assertIn("target_sources(engine_video INTERFACE\n", text)
        self.assertIn("find_package(SDL2 REQUIRED)\n", text)
        self.assertIn("        INTERFACE engine_stream SDL2::SDL2\n", text)
        # video/vqa is a library of its own, not a subfolder of video.
        self.assertNotIn("vqa", text)
        self.assertNotIn("add_gtest", text)


@unittest.skipUnless(shutil.which("git") and shutil.which("clang-format"), "needs git, clang-format")
class MoveTest(unittest.TestCase):
    """Runs a whole step in a scratch repository."""

    FILES = {
        "CMakeLists.txt": "project(x)\nadd_subdirectory(src/port)\nadd_subdirectory(src/tech)\n"
        "add_subdirectory(src/ra)\n",
        "src/port/CMakeLists.txt": "add_library(port STATIC)\n\n# Port tests.\n"
        "add_gtest(\n    NAME port_test\n    SOURCES inet_text_test.cc\n    LIBS port\n)\n",
        "src/port/inet_text.h": "#ifndef CNC_RED_ALERT_PORT_INET_TEXT_H_\n"
        "#define CNC_RED_ALERT_PORT_INET_TEXT_H_\n#endif  // CNC_RED_ALERT_PORT_INET_TEXT_H_\n",
        "src/port/inet_text.cc": '#include "port/inet_text.h"\n',
        "src/port/inet_text_test.cc": '#include "port/inet_text.h"\n#include "gtest/gtest.h"\n',
        "src/tech/CMakeLists.txt": "add_library(tech STATIC)\nadd_gtest(\n    NAME tech_test\n"
        "    SOURCES packet_test.cc rgb_test.cc\n    LIBS tech\n)\n",
        "src/tech/packet.h": "#ifndef CNC_RED_ALERT_TECH_PACKET_H_\n"
        "#define CNC_RED_ALERT_TECH_PACKET_H_\n#endif  // CNC_RED_ALERT_TECH_PACKET_H_\n",
        "src/tech/packet_test.cc": '#include "tech/packet.h"\n',
        "src/tech/rgb.h": '#include "tech/packet.h"\n',
        "src/ra/net.cc": '#include "tech/rgb.h"\n#include <port/inet_text.h>\n'
        '// See port/inet_text.h.\n',
    }

    def setUp(self):
        self._dir = tempfile.TemporaryDirectory()
        self.root = Path(self._dir.name).resolve()
        for path, text in self.FILES.items():
            (self.root / path).parent.mkdir(parents=True, exist_ok=True)
            (self.root / path).write_text(text)
        (self.root / ".clang-format").write_text("BasedOnStyle: Google\n")
        # What an earlier step's git mv left behind (src/port/win32).
        (self.root / "src/port/win32").mkdir()
        for args in (["init", "-q"], ["add", "-A"],
                     ["-c", "user.name=t", "-c", "user.email=t@t", "commit", "-q", "-m", "x"]):
            subprocess.run(["git", *args], cwd=self.root, check=True)

    def tearDown(self):
        self._dir.cleanup()

    def read(self, path: str) -> str:
        return (self.root / path).read_text()

    def test_net_step_empties_port(self):
        with mock.patch.object(move, "REPO_ROOT", self.root), \
                mock.patch.object(move, "SRC_ROOT", self.root / "src"), \
                redirect_stdout(StringIO()) as out:
            move.move_library("net", dry_run=False)
        self.assertIn("deleted src/port", out.getvalue())
        self.assertFalse((self.root / "src/port").exists())
        self.assertEqual(
            self.read("CMakeLists.txt"),
            "project(x)\nadd_subdirectory(src/engine)\nadd_subdirectory(src/tech)\n"
            "add_subdirectory(src/ra)\n",
        )
        # The root that keeps files keeps its CMakeLists.txt, minus the moved test.
        self.assertIn("SOURCES rgb_test.cc\n", self.read("src/tech/CMakeLists.txt"))
        self.assertEqual(
            self.read("src/ra/net.cc"),
            '#include <engine/net/inet_text.h>\n\n#include "tech/rgb.h"\n'
            '// See port/inet_text.h.\n',
        )
        self.assertIn("#define CNC_RED_ALERT_ENGINE_NET_INET_TEXT_H_\n",
                      self.read("src/engine/net/inet_text.h"))
        self.assertIn('#include "engine/net/packet.h"\n', self.read("src/tech/rgb.h"))
        generated = self.read("src/engine/net/CMakeLists.txt")
        self.assertIn("add_library(engine_net STATIC)\n", generated)
        self.assertIn("SOURCES inet_text_test.cc packet_test.cc\n", generated)
        self.assertEqual(self.read("src/engine/CMakeLists.txt").splitlines()[-1],
                         "add_subdirectory(net)")

    def test_local_changes_in_an_emptied_root_stop_before_the_move(self):
        (self.root / "src/port/CMakeLists.txt").write_text("# changed\n")
        with mock.patch.object(move, "REPO_ROOT", self.root), \
                mock.patch.object(move, "SRC_ROOT", self.root / "src"), \
                redirect_stdout(StringIO()), self.assertRaises(SystemExit):
            move.move_library("net", dry_run=False)
        self.assertTrue((self.root / "src/port/inet_text.h").exists())


if __name__ == "__main__":
    unittest.main()
