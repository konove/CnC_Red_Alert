#!/usr/bin/env python3
"""Tests for tools/check_layers.py and tools/engine_layout.py.

Each test writes a few files into a scratch src/ tree named like the real
ones, so the real file map places them, and checks what the checker reports.

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

    def test_tests_and_subfolders_are_placed(self):
        self.assertEqual(layout.destination_of("tech/fixed_test.cc"), "base")
        self.assertEqual(layout.destination_of("tech/codec_state_test.cc"), "codec")
        self.assertEqual(layout.destination_of("sdllib/keyframe_test.cc"), "gfx")
        self.assertEqual(
            layout.destination_of("port/win32/win32_com.h"), "platform/win32"
        )
        self.assertEqual(layout.destination_of("winvq/vqa32/anything.cc"), "video/vqa")
        self.assertEqual(
            layout.destination_of("engine/net/serial/wincomm.cc"), "net/serial"
        )
        self.assertIsNone(layout.destination_of("tech/unknown.cc"))
        self.assertIsNone(layout.destination_of("engine/nowhere/x.h"))
        self.assertIsNone(layout.destination_of("ra/techno.cc"))
        self.assertEqual(layout.library_of("net/serial"), "net")
        self.assertEqual(layout.library_of("video/vqa"), "video/vqa")

    def test_new_path_keeps_the_file_name(self):
        self.assertEqual(layout.new_path("tech/rgb.h"), "engine/gfx/rgb.h")
        self.assertEqual(
            layout.new_path("port/format.cc"), "engine/base/strings/format.cc"
        )
        with self.assertRaises(KeyError):
            layout.new_path("tech/unknown.h")


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
        self.write("sdllib/pixel_buffer.h")
        self.write("base/types.h")
        self.write(
            "sdllib/display.cc", '#include "sdllib/pixel_buffer.h"\n#include <SDL.h>\n'
        )
        self.write("port/format.cc", '#include "base/types.h"\n')
        self.write("base/clip.h", '#include "port/format.h"\n')
        self.write("port/format.h")
        self.assertEqual(self.errors("sdllib/display.cc"), [])
        self.assertEqual(self.errors("port/format.cc"), [])
        self.assertEqual(self.errors("base/clip.h"), [])  # base/strings is base.

    def test_include_up_the_order_fails(self):
        self.write("sdllib/display.h")
        self.write("sdllib/pixel_buffer.cc", '// Draws.\n#include "sdllib/display.h"\n')
        errors = self.errors("sdllib/pixel_buffer.cc")
        self.assertEqual(len(errors), 1)
        self.assertIn("pixel_buffer.cc:2:", errors[0])
        self.assertIn("gfx (engine_gfx) may not include window", errors[0])

    def test_siblings_may_not_include_each_other(self):
        self.write("tech/sha.h")
        self.write("tech/lcw.cc", '#include "tech/sha.h"\n')
        self.assertEqual(len(self.errors("tech/lcw.cc")), 1)

    def test_vqa_may_not_include_file(self):
        self.write("tech/game_file.h")
        self.write("winvq/vqa32/movie.cc", '#include "tech/game_file.h"\n')
        self.assertEqual(len(self.errors("winvq/vqa32/movie.cc")), 1)

    def test_moved_files_are_placed_by_path(self):
        self.write("engine/window/display.h")
        self.write("engine/gfx/pixel_buffer.cc", '#include "engine/window/display.h"\n')
        self.write("engine/window/display.cc", '#include "engine/gfx/pixel_buffer.h"\n')
        self.write("engine/gfx/pixel_buffer.h")
        self.assertEqual(len(self.errors("engine/gfx/pixel_buffer.cc")), 1)
        self.assertEqual(self.errors("engine/window/display.cc"), [])

    def test_engine_may_not_include_the_games(self):
        self.write("ra/keyframe.h")
        self.write("tech/rgb.cc", '#include "ra/keyframe.h"\n')
        self.assertIn("may not include ra/", self.errors("tech/rgb.cc")[0])

    def test_tests_may_include_anything(self):
        self.write("ra/keyframe.h")
        self.write("sdllib/display.h")
        self.write(
            "tech/fixed_test.cc",
            '#include "ra/keyframe.h"\n#include "sdllib/display.h"\n#include <SDL.h>\n',
        )
        self.assertEqual(self.errors("tech/fixed_test.cc"), [])

    def test_sdl_only_where_allowed(self):
        self.write("sdllib/timer.cc", "#include <SDL_timer.h>\n")
        self.write("sdllib/timer.h", "#include <SDL_timer.h>\n")
        self.write("sdllib/shape.cc", '#include <SDL2/SDL_video.h>\n#include "SDL.h"\n')
        self.write("tech/audio_mixer.h", "#include <SDL_audio.h>\n")
        self.assertEqual(self.errors("sdllib/timer.cc"), [])
        self.assertEqual(len(self.errors("sdllib/timer.h")), 1)
        self.assertEqual(len(self.errors("sdllib/shape.cc")), 2)
        self.assertEqual(self.errors("tech/audio_mixer.h"), [])

    def test_relative_include_fails_and_is_still_checked(self):
        self.write("tech/hsv.h")
        self.write("sdllib/display.h")
        self.write("tech/rgb.cc", '#include "hsv.h"\n#include "absl/log/check.h"\n')
        self.write("sdllib/shape.cc", '#include "display.h"\n')
        self.assertEqual(len(self.errors("tech/rgb.cc")), 1)
        errors = self.errors("sdllib/shape.cc")
        self.assertEqual(len(errors), 2)
        self.assertIn("relative to the file", errors[0])
        self.assertIn("may not include window", errors[1])

    def test_angle_bracket_project_include_is_checked(self):
        self.write("sdllib/display.h")
        self.write(
            "tech/rgb.cc", "#include <sdllib/display.h>\n#include <tech/none.h>\n"
        )
        errors = self.errors("tech/rgb.cc")
        self.assertEqual(len(errors), 2)
        self.assertIn("may not include window", errors[0])
        self.assertIn("no such file", errors[1])

    def test_project_header_named_like_sdl_is_layer_checked(self):
        self.write("engine/base/SDL_util.h")
        self.write("engine/window/SDL_glue.h")
        self.write("sdllib/mem.cc", '#include "engine/base/SDL_util.h"\n')
        self.write("tech/rgb.cc", '#include "engine/window/SDL_glue.h"\n')
        self.assertEqual(self.errors("sdllib/mem.cc"), [])
        errors = self.errors("tech/rgb.cc")
        self.assertEqual(len(errors), 1)
        self.assertIn("may not include window", errors[0])

    def test_missing_project_include_fails(self):
        self.write("tech/rgb.cc", '#include "tech/nothing.h"\n')
        self.assertIn("no such file", self.errors("tech/rgb.cc")[0])

    def test_comments_and_strings_hide_nothing(self):
        self.write("sdllib/display.h")
        self.write(
            "tech/rgb.cc",
            '/* #include "sdllib/display.h"\n'
            '#include "sdllib/display.h" */\n'
            '// #include "sdllib/display.h"\n'
            'const char* kGlob = "dir/*";\n'
            '#include "sdllib/display.h"\n'
            'const char* kEnd = "*/";\n',
        )
        errors = self.errors("tech/rgb.cc")
        self.assertEqual(len(errors), 1)
        self.assertIn("rgb.cc:5:", errors[0])

    def test_map_errors(self):
        paths = [
            "engine/gfx/hsv.h",
            "engine/nowhere/x.cc",
            "tech/rgb.cc",
            "tech/stray.h",
        ]
        for path in paths:
            self.write(path)
        files = check_layers.engine_files(self.src)
        self.assertEqual(files, paths)
        errors = check_layers.map_errors(files)
        self.assertIn(
            "src/tech/stray.h: not in the engine file map (tools/engine_layout.py)",
            errors,
        )
        self.assertIn("src/engine/nowhere/x.cc: not in a known engine folder", errors)
        stale = [error for error in errors if "names no file" in error]
        self.assertIn(
            "tools/engine_layout.py: tech/glow_pulse -> gfx names no file", stale
        )
        # rgb is still in tech/, hsv has moved: neither is stale.
        self.assertFalse(any("tech/rgb " in error for error in stale))
        self.assertFalse(any("tech/hsv " in error for error in stale))

    def test_non_source_files_are_ignored(self):
        self.write("tech/CMakeLists.txt")
        self.write("sdllib/README.md")
        self.assertEqual(check_layers.engine_files(self.src), [])


if __name__ == "__main__":
    unittest.main()
