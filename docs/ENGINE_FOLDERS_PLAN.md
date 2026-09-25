# Engine Folders

The shared code under `src/` is grouped by where it came from, not by what it does. `port/` holds
Win32 emulation and sockets next to string helpers and an INI parser; `sdllib/` holds the SDL window
next to pure decoders that never touch SDL, 2D drawing, a serial modem driver and the string table;
`tech/` is 143 files of streams, archives, compression, crypto, bignums, image formats, the audio
mixer, network packets and fixed-point math; and the movie player sits in `winvq/vqa32/` because
that is what EA called it. Telling `port`, `sdllib`, `tech` and `base` apart takes reading the
files.

This plan moves all of it under `src/engine/`, one folder per domain, each folder its own CMake
library with a declared place in a dependency order. `ra/`, `td/`, `tools/` and `testing/` stay
where they are; the games' own layout is a separate, much larger question.

## Findings

Measured on the tree at 23e83b30.

- About 3,300 `#include` lines name the five libraries: `sdllib/` 1,102, `base/` 911, `tech/` 901,
  `port/` 303, `winvq/` 128.
- Mapping every file to its new folder and checking the include graph against the dependency order
  below finds exactly three violating includes, all in one place: `sdllib/pixel_buffer.cc` includes
  `sdllib/display.h` and `sdllib/ww_win.h`, and `sdllib/pixel_buffer.h` includes `sdllib/ww_win.h`.
  `display.cc` includes `pixel_buffer.h` in turn, so gfx and the window are a cycle today.
- The libraries' undefined symbols (from `nm` over the built archives) show four more cycles that go
  through the linker instead of a header -- engine code calling what only the games define:
  `Get_CD_Index` (`tech/search_paths.cc`), `SDL_Event_Handler` (`sdllib/ww_win.cc`), and
  `BigShapeBufferBytes`, `TheaterShapeBufferBytes`, `UseBigShapeBuffer` (`tech/2keyfbuf.cc`). Every
  test that links those libraries stubs them. The last three are dead: no game ever sets
  `UseBigShapeBuffer` (`ra/2keyfram.cc` says so), so the uncompressed-shape-cache branch of
  `2keyfbuf.cc` never runs.
- The event loop (`sdllib/ww_win.cc`) calls `Socket_Select()` on every pump, so the window depends
  on the sockets. Only RA's `wsproto`/`wspudp` use `net_select.h`; TD pumps RA's sockets for
  nothing.
- SDL headers are included by `timer`, `display`, `keyboard`, `ww_mouse`, `ww_win`, `pixel_buffer`,
  `audio_mixer`, `mixer_vqa_audio.h` and `vqa32/audio_output`.
- `port::` (22 files) and `tech::` (`number_parse`, `sha1_compress`) name where code came from, the
  same way the folders do.
- `tech` links `vqa32` only for two adapters, `game_file_vqa_io` and `mixer_vqa_audio`.
- Outside `sdllib`, the window-surface half of `PixelBuffer` (`BUFFER_VISIBLE`, `UpdatePalette`,
  `PresentScaledFrame`, `LockSurface`, `ReleaseSurfaces`, `window_page`) is used by four files:
  `ra/screen.cc`, `ra/interpal.cc`, `td/screen.cc`, `td/interpal.cc`.

## Target layout

```
src/
  engine/
    base/          header vocabulary types and small value classes
      strings/     string formatting, copying and splitting
    platform/      OS services: environment, time, sleep, memory, files on disk
      win32/       Win32 API emulation
    stream/        ByteSink / ByteSource / ByteStream and their adapters
    codec/         compression and encoding
    crypto/        ciphers, hashes, bignum public-key
    file/          disk files, game files, MIX archives, search paths, INI
    net/           packets, sockets
      serial/      null-modem driver
    gfx/           pixel buffers and everything that draws into them
    audio/         the audio mixer
    window/        the SDL window: presenting, palette, cursor, keyboard, event loop
    video/         glue between the movie player and file/ and audio/
      vqa/         the standalone VQA decoder and player
  ra/  td/  tools/  testing/
```

Includes follow the folders: `#include "engine/gfx/pixel_buffer.h"`. Include guards follow the path:
`CNC_RED_ALERT_ENGINE_GFX_PIXEL_BUFFER_H_`.

### Dependency order

A library may include and link only what it points at, and what those point at.

```
base <- platform <- stream <- codec  <- file <- gfx <- window <- games
                           <- crypto <-
base <- platform <- net                                   <- games
stream, codec <- video/vqa
file, audio, video/vqa <- video
file, codec <- audio
```

`codec` and `crypto` are siblings: neither includes the other. `video/vqa` stays independent of
`file` and `audio`; it reads through its own `VqaClient` and plays through `VqaAudioDevice`, and
`video/` holds the two adapters that plug the game's files and mixer into those.

Tests may link more than their library does. `fixed_test` exercising `Serialize()` over a stream
archive is fine even though `fixed.h` is in `base`.

SDL headers are allowed only in `window/`, `audio/`, `video/`, `video/vqa/` and `platform/timer.cc`.
Everything else, `gfx` included, compiles without them; tests are exempt.

A file-format parser lives with what it produces: pixels go to `gfx` (PCX, WSA, shapes), sound
samples that both movies and the mixer decode go to `codec` (`aud_decoder`), and formats that
produce neither (INI, the string table) go to `file`.

### Targets

| Folder             | Target            | Notes                                      |
| ------------------ | ----------------- | ------------------------------------------ |
| `engine/base/`     | `engine_base`     | STATIC now that `strings/` has `.cc` files |
| `engine/platform/` | `engine_platform` | includes `win32/`; links SDL for the timer |
| `engine/stream/`   | `engine_stream`   |                                            |
| `engine/codec/`    | `engine_codec`    | links `lzo`                                |
| `engine/crypto/`   | `engine_crypto`   |                                            |
| `engine/file/`     | `engine_file`     |                                            |
| `engine/net/`      | `engine_net`      | includes `serial/`; `wsock32` on Windows   |
| `engine/gfx/`      | `engine_gfx`      | no SDL once phase A is done                |
| `engine/audio/`    | `engine_audio`    | SDL audio                                  |
| `engine/window/`   | `engine_window`   | SDL video and events                       |
| `engine/video/vqa` | `engine_vqa`      |                                            |
| `engine/video/`    | `engine_video`    |                                            |

The `engine_` prefix keeps generic names like `file` and `stream` out of CMake's global target
namespace. Tests are one `add_gtest` per folder (`engine_gfx_test`, ...), next to the code, keeping
the existing split targets where they exist for a reason (`sdllib_keyboard_test` compiles
`keyboard.cc` without the rest of the library).

### File map

File names do not change, only folders. Tests go with the file they test.

| Destination            | Files (by current folder)                                                                                                                                                                                                                                                                                                         |
| ---------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `engine/base/`         | base: algorithm, array, attributes, buffer, clip, enum_array, flags, hsv, installed, numeric, trig, types; port: aligned_buffer, bytes_of, unaligned; tech: buff, fixed, listnode, random                                                                                                                                         |
| `engine/base/strings/` | port: format, safe_string, tokenizer; tech: number_parse                                                                                                                                                                                                                                                                          |
| `engine/platform/`     | port: env, platform, random_seed, sleep; port/win32 → `win32/`: win32_com, win32_registry, win32_system, win32_types; sdllib: file_system, mem, memflag, timer; tech: ftimer                                                                                                                                                      |
| `engine/stream/`       | base: seek_origin; tech: archive, byte_codec, byte_sink, byte_source, byte_stream, memory_stream, range_stream, readline, span_sink, span_source, stream_sink, stream_source, tee_sink, transform_sink, transform_source, vector_sink                                                                                             |
| `engine/codec/`        | sdllib: aud_decoder, compressed_block, lcw_uncompress, xor_delta; tech: base64, base64_codec, base64_sink, base64_source, block_backends, block_codec, lcw, lcw_sink, lcw_source, lzo_sink, lzo_source, lzw, lzw_sink, lzw_source; tests codec_corrupt, codec_state, lcw_comp, stream_error, stream_golden (link `engine_crypto`) |
| `engine/crypto/`       | tech: blowfish, blowfish_codec, blowfish_sink, blowfish_source, crc, digit_cursor, int, key_phrase_hash, mp, pk, pk_sink, pk_source, random_source, sha, sha1_codec, sha1_compress, sha1_sink, sha1_source; tests blowfish_stream, pk_stream                                                                                      |
| `engine/file/`         | port: profile_buffer; sdllib: string_table; tech: disk_file, disk_stream, file_access, game_file, mix_archive, search_paths                                                                                                                                                                                                       |
| `engine/net/`          | port: inet_text, socket_bytes; sdllib: net_select; tech: field, packet                                                                                                                                                                                                                                                            |
| `engine/net/serial/`   | sdllib: modemreg, wincomm                                                                                                                                                                                                                                                                                                         |
| `engine/gfx/`          | sdllib: bitmap, font, pixel_buffer, shape, stamp, tile, wwstd; tech: 2keyfbuf, glow_pulse, hsv, pcx_file, rect, rgb, wsa_animation; plus what phase A splits out (text windows, fading table, HSV conversion)                                                                                                                     |
| `engine/audio/`        | tech: audio_mixer                                                                                                                                                                                                                                                                                                                 |
| `engine/window/`       | sdllib: display, display_palette, keyboard, misc (what remains of it), ww_mouse, ww_win; test keyframe                                                                                                                                                                                                                            |
| `engine/video/`        | tech: game_file_vqa_io, mixer_vqa_audio                                                                                                                                                                                                                                                                                           |
| `engine/video/vqa/`    | all of winvq/vqa32                                                                                                                                                                                                                                                                                                                |

`src/testing/gtest_main.cc` stays; `cmake/Testing.cmake` links `cnc_gtest_main` to whichever new
target replaces `port`.

## Phase A: break the cycles

In the old folders, before anything moves, so each change reviews as an ordinary refactor. One
commit each.

### A1. `PixelBuffer` stops being the window

`PixelBuffer::Init(..., BUFFER_VISIBLE)` creates an SDL texture and an 8-bit SDL surface, and the
class then presents (`Present`), uploads palettes (`UpdatePalette`), shows stretched movie frames
(`PresentScaledFrame`, `DropScaledFrame`), arms the display's redraw timer and runs the event loop.
Drawing code has no business with any of that.

- gfx gains a `PixelSurface` interface: `Lock()` returns the pixels (a span and a pitch) or fails,
  `Unlock()` gives them back. `PixelBuffer::LockSurface()` / `UnlockSurface()` delegate to an
  attached `PixelSurface*`; a buffer with none is plain memory, as every buffer but one is today.
- `Display` implements `PixelSurface` itself and takes over everything SDL now in `PixelBuffer`: the
  window texture and 8-bit palette surface (created and destroyed with the video mode), present on
  unlock, the redraw timer, `UpdatePalette`, `palette()`, and `PresentScaledFrame` /
  `DropScaledFrame` with their texture. `Display` already owns the window and outlives every page,
  so no third class is needed, and it stops holding a `PixelBuffer*`: the page points at the
  surface, not the other way round. `AttachPage` and `~PixelBuffer`'s detach go away.
- `BUFFER_VISIBLE` and `PixelBufferFlags` go away. `ra/screen.cc`, `td/screen.cc`, `ra/interpal.cc`
  and `td/interpal.cc` attach the window page to `TheDisplay()` and call the palette and movie
  functions on `Display`.
- `pixel_buffer.cc` loses its SDL includes and `display.h`; the SDL half of `pixel_buffer_test`
  moves to `display_test`.

This is the one change the unit tests cannot fully see. Verify it by running both games: the menus,
a palette fade, the intro movie (`PresentScaledFrame`) and an in-game screen.

### A2. Text windows out of `ww_win.h`

`WindowList`, the `kWindow*` column indices and `WinX`/`WinY`/`Window` are the games' text-window
geometry; `PixelView::DrawStamp` clips against them. Move them into a gfx header (`text_window.h`,
storage in a matching `.cc`). `ww_win.h` keeps `SDL_Event_Loop`, `SDL_Send_Quit` and
`Change_Window`, and `pixel_buffer.h` stops including it. Only the declarations move; renaming
`WindowList` and friends to say "text window" is left for later (35 files).

### A3. Split `misc.h`

Used by 61 files. By destination:

| Declarations                                                                                                            | Goes to                               |
| ----------------------------------------------------------------------------------------------------------------------- | ------------------------------------- |
| `Build_Fading_Table`, `Convert_RGB_To_HSV`, `Convert_HSV_To_RGB`                                                        | gfx (`fading_table.h`, `hsv.h`)       |
| `Random()`, `RandNumb`                                                                                                  | `tech/random.h` (→ `engine/base/`)    |
| `Delay`, `Wait_Vert_Blank`, `Wait_Blit`, `Set_Video_Mode`, `Shake_Screen`, `Prog_End`, `Misc_Focus_*`                   | stay in `misc.h` (→ `engine/window/`) |
| `SurfaceMonitorClass`, `AllSurfaces`, `OverlappedVideoBlits`, `AllowHardwareBlitFills`, and anything else with no users | deleted                               |

`optimize_in_debug(misc.cc ...)` follows the fading-table builder to its new file.

### A4. Game-supplied symbols become installed ones

- `SearchPaths` gets a CD probe the game installs (`SearchPaths::SetCdProbe`), replacing the
  `extern int Get_CD_Index(int, int)` declaration in `search_paths.cc`. With none installed, `?:`
  entries are skipped.
- `SDL_Event_Loop` calls a handler the game registers (`SetEventHandler`) instead of the extern
  `SDL_Event_Handler`. The test stubs of `SDL_Event_Handler` go away.
- `SDL_Event_Loop` stops calling `Socket_Select()`. It calls a pump handler the game registers
  (`SetPumpHandler`) once per pass instead; RA registers `Socket_Select`, TD registers nothing.
  `ww_win.cc` drops `net_select.h`, and `window` no longer depends on `net`.
- The uncompressed-shape cache goes: `2keyfbuf.cc`'s `use_new_draw` branch, and
  `BigShapeBufferBytes`, `TheaterShapeBufferBytes`, `UseBigShapeBuffer`, `UseOldShapeDraw`, the two
  `*ShapeBufferStart` pointers and `IsTheaterShape` in both games. Nothing ever set them.

After A4, each library links with only the libraries below it; no test defines a game symbol to
satisfy a library.

### A5. The checker

`tools/engine_layout.py` holds the one file map (the table above) and the allowed dependency table;
the checker and the move script both import it, so the two cannot drift. `tools/check_layers.py`
maps each file to its folder (after phase B, just the path) and fails on any include that goes
against the dependency table, and on any SDL header outside the folders allowed one. It runs as a
ctest (`engine_layers_test`) so the order holds after this plan; linking alone would not catch a
header-only include from a lower folder, since every target shares the `src/` include root. During
phase A it runs against the file map above and must pass before phase B starts.

## Phase B: move

One folder per commit, bottom up, so every commit builds and each new target is proven to link with
only its declared dependencies:

1. `base` + `base/strings`
2. `platform` + `platform/win32`
3. `stream`
4. `codec`
5. `crypto`
6. `file`
7. `net` + `net/serial`
8. `gfx`
9. `audio`
10. `window`
11. `video/vqa` + `video`
12. Delete the emptied `port/`, `sdllib/`, `tech/`, `winvq/` and their targets; tidy the checker's
    rule table to plain paths and drop the file map from `engine_layout.py`.
13. Namespaces follow folders: what was in `port::` or `tech::` takes its new folder's name
    (`base::`, `platform::`, `net::`, `file::`, `crypto::`; the Win32 registry goes to `platform::`
    with the rest of `win32/`). Compiler-driven, one commit. Code with no namespace today stays
    without one.

Until step 12 the old targets keep whatever has not moved yet and link the new targets for what has.

### The move script

`tools/move_engine_files.py <folder>` reads the file map from `tools/engine_layout.py` and, for the
named destination:

- `git mv`s each file, so history and blame follow (`git log --follow`);
- rewrites `#include "old/x.h"` to `#include "engine/<folder>/x.h"` across `src/`, skipping string
  literals and comments;
- regenerates each moved header's include guard from its new path;
- writes the folder's `CMakeLists.txt` and the `add_subdirectory` line;
- runs `git clang-format` on the touched files to re-sort include blocks.

These commits contain moves and include lines only. Anything that needs a hand edit (a CMake link
list, an `optimize_in_debug` call) goes in the same commit but is called out in the message.

### Configuration and documentation

In the step that makes them stale:

- `.clang-tidy` `HeaderFilterRegex` gains `engine`, and loses `port|sdllib|tech|winvq` in step 12.
- `optimize_in_debug()` calls move with their files into the new `CMakeLists.txt`.
- `CLAUDE.md`: the Architecture tree, the Key Files table, and paths in examples
  (`sdllib/pixel_buffer.h`, `tech/byte_sink.h`, `winvq/vqa32/vqa_player.h`, `port/safe_string.h`,
  `port/format.h`).
- `tools/strict_tu.py`'s docstring example path.
- Older `docs/*_PLAN.md` files keep their paths; they record what was true then.

## Verification

- Every commit: `cmake --build build --parallel $JOBS` (both games, all tests, `mixdump`, `vqaplay`)
  and `ctest` in `build`.
- After phase A and after phase B step 12: a full strict build (`build-strict`, `--parallel 14`),
  since every translation unit's include lines change.
- After A1: `tools/ra_saveload_smoke.sh` and `tools/td_saveload_smoke.sh`, and both games run by
  hand as described in A1.
- `engine_layers_test` passes from A5 on.

## Out of scope, noted for later

- File renames that the folders make more obvious: `ww_mouse` → mouse cursor, `ww_win` → event loop,
  `2keyfbuf`, `wwstd`, `misc`. These belong to `/modernize-file` passes.
- Two LCW decoders (`codec/lcw` and `codec/lcw_uncompress`) and two HSV headers (`base/hsv.h`,
  `gfx/hsv.h`), now side by side.
- Renaming `WindowList`, `WinX`, `WinY` and `Window` to say "text window".
- The timer on `std::thread`/`std::chrono` instead of `SDL_AddTimer`, so that nothing below
  `audio`/`window` links SDL.
- Namespaces for engine code that has none today (`PixelBuffer`, `Display`, the streams).
- Subfolders inside `ra/` and `td/`.
- The unbuilt EA reference trees at the repository root (`win32lib/`, `wwflat32/`, `vq/`, `ipx/`,
  `launcher/`, `launch/`).

## Outcome

Execution mostly followed the plan above; where it differed:

`port/`, `sdllib/`, `tech/` and `winvq/` were deleted as each one emptied out, not held to step 12
as planned. `base/CMakeLists.txt` went with the `stream` move (step 3), `port/CMakeLists.txt` with
`net` (step 7), `sdllib/CMakeLists.txt` with `window` (step 10), and `tech/CMakeLists.txt` and
`winvq/CMakeLists.txt` both with `video/vqa` (step 11), since that move took the last files each
root had left. Step 12 was left with only the move script and the file map in
`tools/engine_layout.py` to drop, plus the checker's rule table to tidy to plain paths; that
happened in one commit (`489fdde3`).

`keyframe_test` ended up with `gfx` rather than `window` as the file map planned: the test exercises
the key-frame/delta chain through `PixelBuffer` and `2keyfram.cc`, nothing in `window`, so it moved
with its dependency instead. The HSV conversion functions (`Convert_RGB_To_HSV`,
`Convert_HSV_To_RGB`) that A3 pulled out of `misc.h` landed in `engine/gfx/hsv.h`, reusing the name
of the old `tech/hsv.h` they came from rather than a new header name; `engine/base/hsv.h`, the HSV
value type from the old `base/hsv.h`, is a different file that happens to share a basename one
folder over.

The move script (`tools/move_engine_files.py`, deleted in step 12) formatted only the lines it
changed with plain `clang-format --lines`, not `git clang-format`: many legacy files were not
clang-format clean to begin with, and `git clang-format` would have reformatted them whole just
because one line in the diff touched them.

A3 and A4 each took more than the one commit the plan sketched. A3 (splitting `misc.h`) went out as
a dead-code deletion first (`6be19256`, the DirectDraw surface monitor and constant blit flags,
which had no callers left) and then the actual split (`02e91bed`), so the split diff would not also
be a deletion diff. A4 (game-supplied symbols) went out one hook per commit: the CD probe
(`0f171ec9`), the event handler (`bd043a26`), the pump handler (`d97607dc`), and the uncompressed
shape cache removal, once per game.

One thing the plan did not anticipate: `ra_engine` and `td_engine` now link `SDL2::SDL2` directly.
`sdlstub.cc` in both games includes SDL headers, and until the final review that only worked because
SDL arrived transitively through `engine_window`/`engine_video`'s public link; see the "final
whole-branch review" fix for this branch.

Outstanding from A1: the real-display check (menus, a palette fade, the intro movie via
`PresentScaledFrame`, and an in-game screen) still needs a human to run both games and look, not
just the unit tests A1 could reach.
