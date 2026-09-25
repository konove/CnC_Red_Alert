#!/usr/bin/env python3
"""Tests for tools/check_layers.py and tools/engine_layout.py.

Each test writes a few files into a scratch src/ tree named like the real
ones, so the real layout places them, and checks what the checker reports.

Usage:
  tools/check_layers_test.py

Runs as the engine_layers_checker_test ctest.
"""

import tempfile
import unittest
from pathlib import Path

import check_layers
import engine_layout as layout


class LayoutTest(unittest.TestCase):
    def test_allowed_libraries_is_the_transitive_closure(self):
        window = layout.allowed_libraries("window")
        self.assertTrue({"gfx", "file", "codec", "crypto", "stream", "base"} <= window)
        self.assertNotIn("net", window)
        self.assertNotIn("audio", window)
        self.assertIn("codec", layout.allowed_libraries("video"))
        self.assertNotIn("crypto", layout.allowed_libraries("codec"))
        self.assertNotIn("file", layout.allowed_libraries("video/vqa"))

    def test_every_library_has_a_dependency_entry(self):
        self.assertEqual(set(layout.LIBRARIES), set(layout.DEPENDS_ON))
        for library in layout.LIBRARIES:
            layout.allowed_libraries(library)  # Every dependency is known.

    def test_files_are_placed_by_path(self):
        self.assertEqual(layout.library_folder_of("base/types.h"), "base")
        self.assertEqual(
            layout.library_folder_of("base/strings/format_test.cc"),
            "base/strings",
        )
        self.assertEqual(
            layout.library_folder_of("engine/platform/win32/win32_com.h"),
            "platform/win32",
        )
        self.assertEqual(
            layout.library_folder_of("engine/video/vqa/movie.cc"), "video/vqa"
        )
        self.assertEqual(
            layout.library_folder_of("engine/net/serial/wincomm.cc"), "net/serial"
        )
        self.assertIsNone(layout.library_folder_of("engine/nowhere/x.h"))
        self.assertIsNone(layout.library_folder_of("engine/base/types.h"))
        self.assertIsNone(layout.library_folder_of("base/nowhere/x.h"))
        self.assertIsNone(layout.library_folder_of("engine/rgb.h"))  # No bare folder.
        self.assertIsNone(layout.library_folder_of("ra/techno.cc"))
        self.assertEqual(layout.library_of("net/serial"), "net")
        self.assertEqual(layout.library_of("video/vqa"), "video/vqa")


class CheckerTest(unittest.TestCase):
    def setUp(self):
        self._dir = tempfile.TemporaryDirectory()
        self.src = Path(self._dir.name).resolve()

    def tearDown(self):
        self._dir.cleanup()

    def write(self, path: str, text: str = "") -> None:
        file = self.src / path
        file.parent.mkdir(parents=True, exist_ok=True)
        file.write_text(text)

    def errors(self, path: str) -> list[str]:
        return check_layers.include_errors(self.src, path)

    def test_includes_down_the_order_pass(self):
        self.write("engine/gfx/pixel_buffer.h")
        self.write("base/types.h")
        self.write(
            "engine/window/display.cc",
            '#include "engine/gfx/pixel_buffer.h"\n#include <SDL.h>\n',
        )
        self.write(
            "base/strings/format.cc", '#include "base/types.h"\n'
        )
        self.write("base/clip.h", '#include "base/strings/format.h"\n')
        self.write("base/strings/format.h")
        self.assertEqual(self.errors("engine/window/display.cc"), [])
        self.assertEqual(self.errors("base/strings/format.cc"), [])
        self.assertEqual(self.errors("base/clip.h"), [])  # base/strings is base.

    def test_include_up_the_order_fails(self):
        self.write("engine/window/display.h")
        self.write(
            "engine/gfx/pixel_buffer.cc",
            '// Draws.\n#include "engine/window/display.h"\n',
        )
        errors = self.errors("engine/gfx/pixel_buffer.cc")
        self.assertEqual(len(errors), 1)
        self.assertIn("pixel_buffer.cc:2:", errors[0])
        self.assertIn("gfx (engine_gfx) may not include window", errors[0])

    def test_siblings_may_not_include_each_other(self):
        self.write("engine/crypto/sha.h")
        self.write("engine/codec/lcw.cc", '#include "engine/crypto/sha.h"\n')
        self.assertEqual(len(self.errors("engine/codec/lcw.cc")), 1)

    def test_vqa_may_not_include_file(self):
        self.write("engine/file/game_file.h")
        self.write(
            "engine/video/vqa/movie.cc", '#include "engine/file/game_file.h"\n'
        )
        self.assertEqual(len(self.errors("engine/video/vqa/movie.cc")), 1)

    def test_engine_may_not_include_the_games(self):
        self.write("ra/keyframe.h")
        self.write("engine/gfx/rgb.cc", '#include "ra/keyframe.h"\n')
        self.assertIn("may not include ra/", self.errors("engine/gfx/rgb.cc")[0])

    def test_tests_may_include_anything(self):
        self.write("ra/keyframe.h")
        self.write("engine/window/display.h")
        self.write(
            "base/fixed_test.cc",
            '#include "ra/keyframe.h"\n#include "engine/window/display.h"\n'
            "#include <SDL.h>\n",
        )
        self.assertEqual(self.errors("base/fixed_test.cc"), [])

    def test_sdl_only_where_allowed(self):
        self.write("engine/platform/timer.cc", "#include <SDL_timer.h>\n")
        self.write("engine/platform/timer.h", "#include <SDL_timer.h>\n")
        self.write(
            "engine/gfx/shape.cc",
            '#include <SDL2/SDL_video.h>\n#include "SDL.h"\n',
        )
        self.write("engine/audio/audio_mixer.h", "#include <SDL_audio.h>\n")
        self.assertEqual(self.errors("engine/platform/timer.cc"), [])
        self.assertEqual(len(self.errors("engine/platform/timer.h")), 1)
        self.assertEqual(len(self.errors("engine/gfx/shape.cc")), 2)
        self.assertEqual(self.errors("engine/audio/audio_mixer.h"), [])

    def test_relative_include_fails_and_is_still_checked(self):
        self.write("engine/gfx/hsv.h")
        self.write("engine/window/display.h")
        self.write(
            "engine/gfx/rgb.cc", '#include "hsv.h"\n#include "absl/log/check.h"\n'
        )
        # ".." crosses from gfx/ into window/, a folder gfx may not include.
        self.write("engine/gfx/shape.cc", '#include "../window/display.h"\n')
        self.assertEqual(len(self.errors("engine/gfx/rgb.cc")), 1)
        errors = self.errors("engine/gfx/shape.cc")
        self.assertEqual(len(errors), 2)
        self.assertIn("relative to the file", errors[0])
        self.assertIn("may not include window", errors[1])

    def test_angle_bracket_project_include_is_checked(self):
        self.write("engine/window/display.h")
        self.write(
            "engine/gfx/rgb.cc",
            "#include <engine/window/display.h>\n#include <engine/gfx/none.h>\n",
        )
        errors = self.errors("engine/gfx/rgb.cc")
        self.assertEqual(len(errors), 2)
        self.assertIn("may not include window", errors[0])
        self.assertIn("no such file", errors[1])

    def test_project_header_named_like_sdl_is_layer_checked(self):
        self.write("base/SDL_util.h")
        self.write("engine/window/SDL_glue.h")
        self.write("engine/platform/mem.cc", '#include "base/SDL_util.h"\n')
        self.write("engine/gfx/rgb.cc", '#include "engine/window/SDL_glue.h"\n')
        self.assertEqual(self.errors("engine/platform/mem.cc"), [])
        errors = self.errors("engine/gfx/rgb.cc")
        self.assertEqual(len(errors), 1)
        self.assertIn("may not include window", errors[0])

    def test_missing_project_include_fails(self):
        self.write("engine/gfx/rgb.cc", '#include "engine/gfx/nothing.h"\n')
        self.assertIn("no such file", self.errors("engine/gfx/rgb.cc")[0])

    def test_comments_and_strings_hide_nothing(self):
        self.write("engine/window/display.h")
        self.write(
            "engine/gfx/rgb.cc",
            '/* #include "engine/window/display.h"\n'
            '#include "engine/window/display.h" */\n'
            '// #include "engine/window/display.h"\n'
            'const char* kGlob = "dir/*";\n'
            '#include "engine/window/display.h"\n'
            'const char* kEnd = "*/";\n',
        )
        errors = self.errors("engine/gfx/rgb.cc")
        self.assertEqual(len(errors), 1)
        self.assertIn("rgb.cc:5:", errors[0])

    def test_folder_errors(self):
        paths = [
            "base/nowhere/x.h",
            "base/types.h",
            "engine/gfx/hsv.h",
            "engine/nowhere/x.cc",
        ]
        for path in paths:
            self.write(path)
        files = check_layers.engine_files(self.src)
        self.assertEqual(files, paths)
        errors = check_layers.folder_errors(files)
        self.assertEqual(
            errors,
            [
                "src/base/nowhere/x.h: not in a known engine folder",
                "src/engine/nowhere/x.cc: not in a known engine folder",
            ],
        )

    def test_non_source_files_are_ignored(self):
        self.write("base/CMakeLists.txt")
        self.write("engine/gfx/README.md")
        self.assertEqual(check_layers.engine_files(self.src), [])


if __name__ == "__main__":
    unittest.main()
