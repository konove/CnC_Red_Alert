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

    def test_game_libraries_are_placed_by_path(self):
        self.assertEqual(layout.library_folder_of("game/core/vector.h"), "game/core")
        self.assertEqual(layout.library_folder_of("game/ui/gadget_test.cc"), "game/ui")
        self.assertIsNone(layout.library_folder_of("game/vector.h"))  # No bare folder.
        self.assertIsNone(layout.library_folder_of("game/nowhere/x.h"))
        self.assertIsNone(layout.library_folder_of("engine/game/core/x.h"))
        self.assertEqual(layout.LIBRARIES["game/sim"], "game_sim")

    def test_game_libraries_sit_above_the_engine(self):
        sim = layout.allowed_libraries("game/sim")
        self.assertTrue({"game/core", "window", "video", "net", "base"} <= sim)
        self.assertNotIn("game/ui", sim)
        self.assertNotIn("game/sim", layout.allowed_libraries("game/ui"))
        self.assertNotIn("game/sim", layout.allowed_libraries("game/core"))
        for library in layout.LIBRARIES:
            if not library.startswith("game/"):
                allowed = layout.allowed_libraries(library)
                self.assertFalse(
                    any(name.startswith("game/") for name in allowed), library
                )

    def test_games_are_known_by_path(self):
        self.assertEqual(layout.game_of("ra/techno.cc"), "ra")
        self.assertEqual(layout.game_of("td/net/ipxmgr.h"), "td")
        self.assertIsNone(layout.game_of("engine/gfx/rgb.h"))
        self.assertIsNone(layout.game_of("game/core/vector.h"))
        self.assertIsNone(layout.game_of("tools/mixdump.cc"))

    def test_header_guard_follows_the_path(self):
        self.assertEqual(
            layout.header_guard("engine/gfx/pixel_buffer.h"),
            "CNC_RED_ALERT_ENGINE_GFX_PIXEL_BUFFER_H_",
        )
        self.assertEqual(
            layout.header_guard("ra/wolapi/wolapi.h"),
            "CNC_RED_ALERT_RA_WOLAPI_WOLAPI_H_",
        )


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

    def test_engine_may_not_include_game_libraries(self):
        self.write("game/core/vector.h")
        self.write("engine/gfx/rgb.cc", '#include "game/core/vector.h"\n')
        errors = self.errors("engine/gfx/rgb.cc")
        self.assertEqual(len(errors), 1)
        self.assertIn("may not include game/core (game_core)", errors[0])

    def test_game_libraries_follow_their_order(self):
        self.write("game/core/vector.h")
        self.write("game/ui/gadget.h")
        self.write("engine/window/display.h")
        self.write("ra/techno.h")
        self.write(
            "game/sim/door.cc",
            '#include "game/core/vector.h"\n#include "engine/window/display.h"\n',
        )
        self.write("game/sim/fly.cc", '#include "game/ui/gadget.h"\n')
        self.write("game/core/heap.cc", '#include "ra/techno.h"\n')
        self.assertEqual(self.errors("game/sim/door.cc"), [])
        errors = self.errors("game/sim/fly.cc")
        self.assertEqual(len(errors), 1)
        self.assertIn("game/sim (game_sim) may not include game/ui", errors[0])
        self.assertIn("may not include ra/", self.errors("game/core/heap.cc")[0])

    def test_games_include_down_the_order(self):
        self.write("base/types.h")
        self.write("engine/gfx/rgb.h")
        self.write("game/ui/gadget.h")
        self.write("ra/techno.h")
        self.write(
            "ra/unit.cc",
            '#include "base/types.h"\n#include "engine/gfx/rgb.h"\n'
            '#include "game/ui/gadget.h"\n#include "ra/techno.h"\n'
            "#include <SDL.h>\n",
        )
        self.assertEqual(self.errors("ra/unit.cc"), [])

    def test_games_may_not_include_each_other(self):
        self.write("ra/filepcx.h")
        self.write("td/techno.h")
        self.write("td/winstub.cc", '#include "ra/filepcx.h"\n')
        self.write("ra/unit_test.cc", '#include "td/techno.h"\n')
        errors = self.errors("td/winstub.cc")
        self.assertEqual(len(errors), 1)
        self.assertIn("td may not include ra/", errors[0])
        self.assertIn("ra may not include td/", self.errors("ra/unit_test.cc")[0])

    def test_game_includes_must_resolve_from_src(self):
        self.write("ra/techno.h")
        self.write(
            "ra/unit.cc", '#include "techno.h"\n#include "ra/nothing.h"\n'
        )
        errors = self.errors("ra/unit.cc")
        self.assertEqual(len(errors), 2)
        self.assertIn("relative to the file", errors[0])
        self.assertIn("no such file", errors[1])

    def test_header_guard_errors(self):
        self.write(
            "ra/techno.h",
            "/* License. */\n// #ifndef OLD_H\n"
            "#ifndef CNC_RED_ALERT_RA_TECHNO_H_\n"
            "#define CNC_RED_ALERT_RA_TECHNO_H_\n#endif\n",
        )
        self.write(
            "engine/gfx/rgb.h",
            "#ifndef CNC_RED_ALERT_ENGINE_RGB_H_\n"
            "#define CNC_RED_ALERT_ENGINE_RGB_H_\n#endif\n",
        )
        self.write(
            "td/unit.h",
            "#ifndef CNC_RED_ALERT_TD_UNIT_H_\n#define CNC_RED_ALERT_TD_UNITS_H_\n",
        )
        self.write("base/types.h", "#pragma once\n")
        self.assertEqual(check_layers.header_guard_errors(self.src, "ra/techno.h"), [])
        errors = check_layers.header_guard_errors(self.src, "engine/gfx/rgb.h")
        self.assertEqual(
            errors,
            [
                "src/engine/gfx/rgb.h:1: header guard CNC_RED_ALERT_ENGINE_RGB_H_ "
                "does not match the path; expected "
                "CNC_RED_ALERT_ENGINE_GFX_RGB_H_"
            ],
        )
        self.assertIn(
            "defines CNC_RED_ALERT_TD_UNITS_H_",
            check_layers.header_guard_errors(self.src, "td/unit.h")[0],
        )
        self.assertIn(
            "no header guard",
            check_layers.header_guard_errors(self.src, "base/types.h")[0],
        )

    def test_check_covers_games_and_guards(self):
        self.write("ra/filepcx.h", "#ifndef X\n#define X\n#endif\n")
        self.write("td/winstub.cc", '#include "ra/filepcx.h"\n')
        self.write("tools/mixdump.h", "#ifndef Y\n#define Y\n#endif\n")
        errors = check_layers.check(self.src)
        self.assertEqual(len(errors), 3)
        self.assertTrue(any("td may not include ra/" in e for e in errors))
        self.assertTrue(any("src/ra/filepcx.h:1: header guard X" in e for e in errors))
        self.assertTrue(any("src/tools/mixdump.h:1: header guard Y" in e for e in errors))

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
        self.write(
            "engine/gfx/shape.cc",
            '#include <SDL2/SDL_video.h>\n#include "SDL.h"\n',
        )
        self.write("engine/audio/audio_mixer.h", "#include <SDL_audio.h>\n")
        self.assertEqual(len(self.errors("engine/platform/timer.cc")), 1)
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
        self.write("engine/platform/env.cc", '#include "base/SDL_util.h"\n')
        self.write("engine/gfx/rgb.cc", '#include "engine/window/SDL_glue.h"\n')
        self.assertEqual(self.errors("engine/platform/env.cc"), [])
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
            "game/core/vector.h",
            "game/nowhere/x.cc",
        ]
        for path in paths:
            self.write(path)
        files = check_layers.engine_files(self.src)
        self.assertEqual(files, paths)
        errors = check_layers.folder_errors(files)
        self.assertEqual(
            errors,
            [
                "src/base/nowhere/x.h: not in a known library folder",
                "src/engine/nowhere/x.cc: not in a known library folder",
                "src/game/nowhere/x.cc: not in a known library folder",
            ],
        )

    def test_non_source_files_are_ignored(self):
        self.write("base/CMakeLists.txt")
        self.write("engine/gfx/README.md")
        self.assertEqual(check_layers.engine_files(self.src), [])


if __name__ == "__main__":
    unittest.main()
