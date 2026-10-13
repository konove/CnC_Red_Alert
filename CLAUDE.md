# CLAUDE.md

C++23 port of EA's Command & Conquer Red Alert and Tiberian Dawn using SDL2. Legacy 1990s game code
being modernized incrementally.

## Quick Reference

```bash
# Set this once per terminal session (works on Linux and macOS):
JOBS=$(($(getconf _NPROCESSORS_ONLN) / 2))
```

| Target                 | Command                                                                          | Output                                     |
| ---------------------- | -------------------------------------------------------------------------------- | ------------------------------------------ |
| Both games             | `cmake -Bbuild -G Ninja && cmake --build build --parallel $JOBS`                 | `build/src/ra/rasdl`, `build/src/td/tdsdl` |
| Red Alert only         | `cmake --build build --parallel $JOBS --target rasdl`                            | `build/src/ra/rasdl`                       |
| Tiberian Dawn only     | `cmake --build build --parallel $JOBS --target tdsdl`                            | `build/src/td/tdsdl`                       |
| Fast build (no checks) | `cmake -Bbuild -G Ninja -DSTRICT_CHECKS=OFF`                                     | Disables clang-tidy and warnings           |
| Unoptimized (stepping) | `cmake -Bbuild -G Ninja -DCMAKE_BUILD_TYPE=Debug`                                | `-O0 -g`; see the warning below            |
| With ASan              | `cmake -Bbuild -G Ninja -DENABLE_ASAN=ON`                                        | Memory debugging                           |
| Clean rebuild          | `rm -rf build && cmake -Bbuild -G Ninja && cmake --build build --parallel $JOBS` |                                            |

**Build type defaults to `RelWithDebInfo`.** Do not run or benchmark a `Debug` build: the tree's
bounds checks (`base::At`, the checked container accessors, the span `CHECK`s) sit in the innermost
loops of LCW decompression, Blowfish, SHA-1 and the paletted blit, and unoptimized they are real
out-of-line calls. Measured on Red Alert 2026-09-17, `Debug` against `RelWithDebInfo`: loading a
saved game 483 ms vs 19 ms, startup bootstrap 1836 ms vs 45 ms, and the intro movie could not decode
in real time at all, stretching 10.5 s of video to 22 s. `Debug` is for stepping in a debugger,
nothing else. The worst C++ kernels (the fading table, the window blit) are compiled at `-O2` even
in `Debug` via `optimize_in_debug()` in `cmake/OptimizeInDebug.cmake`, and the Rust kernels in
`src/rust/` (LCW, the bignum, Blowfish, SHA-1, the sound and movie decoders) via
`[profile.dev] opt-level = 2` in `src/rust/Cargo.toml`; add a file there only if a Debug profile
shows it hot.

## Setup

### Required Dependencies

**All platforms:** SDL2, C++23 compiler, Ninja, and the Rust toolchain through rustup
(`curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh`); `rust-toolchain.toml` pins the
version and rustup installs it on first use.

**Linux (Debian/Ubuntu):**

```bash
sudo apt update
sudo apt install libsdl2-dev clang-tidy ninja-build
```

**macOS:**

```bash
brew install sdl2 llvm ninja
```

**Note:** If clang-tidy is not installed, either install it (above) or build with
`-DSTRICT_CHECKS=OFF` to disable static analysis.

**Optional: cpplint** (`pipx install cpplint==2.0.2`, the version CI pins) adds the `cpplint_test`
ctest; without it CMake skips the test.

### Optional: Faster Builds

`ccache` (compile cache), `clang-tidy-cache` (clang-tidy cache) and `mold` (linker) are picked up
automatically by `cmake/Speedup.cmake` when installed — no per-machine configuration, and the build
works unchanged without them.

```bash
sudo apt install ccache mold        # Linux
brew install ccache mold            # macOS (mold is Linux-only; the module skips it elsewhere)
```

`clang-tidy-cache` has no distro or PyPI package — install it from
[matus-chochlik/ctcache](https://github.com/matus-chochlik/ctcache) and put `clang-tidy-cache` on
`PATH` (`ctcache` is also accepted as the binary name).

Configure output confirms them (`-- ccache enabled: ...`, `-- clang-tidy cache enabled: ...`,
`-- mold linker enabled: ...`). Disable any of them with `-DUSE_CCACHE=OFF` / `-DUSE_CTCACHE=OFF` /
`-DUSE_MOLD=OFF`. Check the compile cache with `ccache -s`; resize with `ccache -M 25G`.

**Removing a system header package can make every build rebuild everything.** ccache restores the
depfile it stored with a cached object, and its hash does not cover headers that were only probed
with `__has_include` and are now missing. After `libtbb-dev` was purged (2026-09-13), libstdc++'s
`pstl_config.h` probe of `<tbb/tbb.h>` kept hitting old cache entries whose `.d` files still listed
`/usr/include/tbb/tbb.h`; ninja treated the missing file as dirty, rebuilt, and got the same depfile
back, so no build ever reached `ninja: no work to do`. Check with
`ninja -C <build-dir> -t deps | grep -c /usr/include/tbb/tbb.h` (or whichever header was removed),
and fix with one `CCACHE_RECACHE=1 cmake --build <build-dir>`, which recompiles and replaces those
entries.

**Build a directory with the ninja that configured it.** Ninja versions hash commands differently in
`.ninja_log`, so alternating two of them marks every object "command line changed" and rebuilds the
tree. CLion-configured directories record CLion's bundled ninja (1.13.2 as of 2026-09-14) in
`CMAKE_MAKE_PROGRAM`, while `/usr/bin/ninja` is 1.11.1. `cmake --build` always uses the recorded
one; don't run bare `ninja` in those directories, and pass ninja flags through instead:
`cmake --build <build-dir> -- -d explain`. Also, `ninja -n` always prints "Re-running CMake..."
because a dry run can't check whether the glob verification changed anything; only a real build
shows whether CMake re-runs.

Corrosion's cargo step runs on every build and lets cargo decide what is stale (under 0.1 s), so
`cmake --build` never prints `ninja: no work to do`; that is not the rebuild loop described above.

ccache caches only the compile; clang-tidy runs as a separate pass in front of it and is cached by
`clang-tidy-cache` (below). IWYU is off by default, `STRICT_CHECKS` included: its suggestions never
fail a build, `misc-include-cleaner` already enforces `.cc` includes as an error, and it re-parses
every TU uncached (~950 CPU-s per full strict pass). Turn it on with `-DENABLE_IWYU=ON` to review
header includes by hand.

**The static analyzer runs shallow locally and deep in CI.** `CLANG_ANALYZER_MODE` (`shallow` by
default, or `deep`) sets clang-tidy's `-analyzer-config mode=`. Shallow halves the analyzer's cost
and takes `ra/saveload/ioobj.cc` from 62 s to 4 s; the CI lint job configures `deep`, so a path only
deep mode explores still fails there. To reproduce a CI-only analyzer finding, reconfigure a strict
dir with `-DCLANG_ANALYZER_MODE=deep` or run clang-tidy on the one TU with
`--extra-arg=-Xclang --extra-arg=-analyzer-config --extra-arg=-Xclang --extra-arg=mode=deep`.

**`clang-tidy-cache` only caches a translation unit whose preprocess is silent.** It derives its
hash by re-running the compiler to preprocess the TU and gives up on any compiler output to stderr
(`hash_inputs` returns `None`), and under `-Weverything` a single preprocessor warning in a widely
included header is enough to disable it tree-wide. The `_MAX_PATH`-style defines in the former
`src/port/ex_string.h` used to do exactly that: 14/40 RA TUs were cacheable. With them renamed
(`kMaxPath` and friends) and the reserved MIDL macros in `src/engine/platform/win32/win32_com.h` and
`src/ra/wol/wolapi.h` wrapped in `#pragma clang diagnostic ignored "-Wreserved-macro-identifier"`,
all 908 project TUs preprocess silently. Measured 2026-09-12 over the first 40 RA objects, deleted
before each run with a fresh `CTCACHE_DIR`: 33.3 s cold, 8.7 s warm (compile from ccache and IWYU
still run). Keep new headers preprocessor-quiet, or the cache silently stops working for everything
that includes them.

That bail-out is also what prints `ERROR:clang-tidy-cache:Error executing compile command: #[...]`
during a strict build — one line per translation unit whose preprocess warns. It is noise, not a
failure: the build continues uncached. Since every unit is quiet today, seeing it again means some
header started warning and caching is off for the units that include it.
`CTCACHE_LOG_LEVEL=CRITICAL` hides the message; `-DUSE_CTCACHE=OFF` drops the wrapper entirely.

**Every strict build dir shares one clang-tidy cache,** `~/.cache/ctcache` (set another with
`-DCTCACHE_DIR=...`). `cmake/Speedup.cmake` runs `clang-tidy-cache` under `cmake -E env` with
`CTCACHE_STRIP` (the build dir), `CTCACHE_STRIP_SRC` and `CTCACHE_EXCLUDE_HASH_REGEX` (`-O*`, `-g*`,
color flags), so the same source state hashes the same in `build-strict` and CLion's strict dir, and
whichever builds second skips clang-tidy. Sharing needs the same compiler, analyzer mode and
optimized-or-not build type (`-O0` changes `__OPTIMIZE__`, `Release` skips `.env`), so keep both
dirs on `RelWithDebInfo`. `clang-tidy-cache --show-stats` reports on its server mode, not this
cache; to check that two dirs agree, compare `CTCACHE_DUMP=1 CTCACHE_DUMP_DIR=<dir>` dumps of one
TU.

**To check one file without re-analyzing the tree,** run `tools/strict_tu.py <files>`. It builds
only the objects those sources produce, with the strict directory's own flags, caches and ninja —
seconds, against the four minutes a change to a header with a 372-TU fan-out
(`engine/gfx/pixel_buffer.h`) costs at `-j14`. A header is checked through its sibling `.cc`. It
proves nothing about the other includers of a changed header, so it is for the edit loop; the full
strict build still gates the commit.

Editing `.clang-tidy` re-checks the whole tree on the next build — its hash rides along in the
clang-tidy command line, so a config change makes every object stale. No `clean` needed (and `clean`
is expensive: it throws away objects ccache can restore for free, but nothing can restore the
analysis).

**CLion:** nothing to configure — reload the CMake project (_File | Reload CMake Project_) and check
the CMake tool window for the status lines. Ensure CLion's toolchain PATH sees `/usr/bin`; if
`ccache` shows as not found there but works in a terminal, set the full path in _Settings | Build,
Execution, Deployment | CMake | Environment_ via `CMAKE_CXX_COMPILER_LAUNCHER=/usr/bin/ccache`.

## Architecture

All source lives under `src/`:

```
base/        → Vocabulary types and small value classes, shared by the engine, the games and the
               tools: types.h (base::ssize), numeric.h, fixed.h, random.h; strings/ holds format,
               safe_string, tokenizer, number_parse (target `base`) [depends: abseil, magic_enum]
engine/      → The shared libraries, one folder per domain (docs/ENGINE_FOLDERS_PLAN.md)
  platform/  → OS services: environment, time, sleep, memory, files on disk; win32/ holds the Win32
               API emulation (target `engine_platform`) [depends: base]
  stream/    → ByteSink/ByteSource/ByteStream and their adapters: archive, memory/range streams,
               span/stream sinks and sources, tee, readline (target `engine_stream`)
               [depends: engine_platform]
  codec/     → Compression and encoding: LCW, LZW, base64, the block codec backends, the .AUD
               decoders and the animated-cursor XOR delta (target `engine_codec`; links `lzo`, and
               `engine_rs_cxx`, whose Rust kernels the LCW, XOR delta and .AUD entry points call)
               [depends: engine_stream]
  crypto/    → Ciphers, hashes and public-key: Blowfish, SHA-1 and PKey, each a C++ class or
               function over a Rust kernel (target `engine_crypto`; links `engine_rs_cxx`; sibling
               of engine/codec/, neither includes the other) [depends: engine_stream]
  file/      → Disk files, game files, MIX archives, search paths, the string table and the INI
               database (target `engine_file`) [depends: engine_codec, engine_crypto]
  net/       → Packets and sockets; net/serial/ holds the null-modem driver (target `engine_net`;
               wsock32 and ws2_32 on Windows) [depends: engine_platform]
  gfx/       → Pixel buffers and everything that draws into them: bitmap, font, shape, stamp, tile,
               text windows, the fading table, PCX/WSA images and HSV/RGB conversion (target
               `engine_gfx`; no SDL) [depends: engine_file]
  audio/     → The audio mixer (target `engine_audio`; links SDL2 for audio output)
               [depends: engine_file, engine_codec]
  window/    → The SDL2 window: presenting, palette, cursor, keyboard, event loop (target
               `engine_window`; links SDL2) [depends: engine_gfx]
  video/     → The movie-file glue: adapts the game's files and audio mixer for VQA playback
               (target `engine_video`; links SDL2) [depends: engine_file, engine_audio,
               engine_vqa]
    vqa/     → The standalone VQA video codec, independent of engine/file and engine/audio
               (target `engine_vqa`) [depends: engine_stream, engine_codec, SDL2]
game/        → The code both games share, in namespace game::, above the engine and below the games
               (docs/GAME_NAMESPACES_PLAN.md)
  core/      → OwnedVector, the object pools (ObjectHeapBase/ObjectHeap), QueueClass,
               the frame clock, path-overlap bits, cell/pixel sizes (target `game_core`)
               [depends: base, engine_stream]
  sim/       → Simulation helpers: the direction types (Facing, Direction), Heading, the shared
               simulation enums (ArmorType, MoveType, BStateType, ...),
               UnitTrackerClass, FlasherClass, Door, StageCounter (target `game_sim`)
               [depends: game_core]
  ui/        → Input, GadgetList, the gadget base classes, the widgets as templates over each
               game's UiSkin (ra/ui/ui_skin.h, td/ui/ui_skin.h), aliased per game, and the names of
               the sixteen standard palette colors (target `game_ui`)
               [depends: game_core]
rust/        → engine_rs, the engine kernels written in Rust (LCW, XOR delta, the sound and VQ
               decoders, SHA-1, Blowfish, the public-key step), linked through the cxx bridge in
               src/lib.rs; CMake target engine_rs_cxx (Corrosion); crates `sha1` and `num-bigint`
               [depends: nothing in the tree]
ra/          → Red Alert (~420 files) [depends: game_core, game_sim, game_ui, engine_net, engine_window, engine_video]
td/          → Tiberian Dawn (~330 files) [depends: game_core, game_sim, game_ui, engine_net, engine_window, engine_video]
```

Each game's code is in one flat namespace, `ra::` or `td::` (only `main`/`WinMain` stay global).
Both games use the same subfolders, for navigation only: `objects/` (the object hierarchy, types,
houses), `data/` (the `*data.cc` tables, rules, INI), `map/` (the display chain, layers,
pathfinding, the world), `ai/` (teams, triggers, scenario), `ui/` (widgets and dialog helpers),
`screens/` (menus and full-screen dialogs), `net/` (multiplayer), `editor/` (scenario editor),
`saveload/`, `assets/` (palettes, audio, screen pages, image files), `startup/` (main, init, the
main loop, platform stubs), `core/` (defines, inline helpers, heaps, vectors) and `debug/`; Red
Alert also has `wol/` (Westwood Online).

The `[depends: ...]` list on each folder above is enforced, not just documented: an engine file may
only include a folder its own library links, per `tools/check_layers.py` (see Key Files). The same
check keeps `ra/` and `td/` from including each other, and the shared game code that
docs/GAME_NAMESPACES_PLAN.md moves to `src/game/` (`game_core`, `game_sim`, `game_ui`, above the
engine) from including either game.

**Class hierarchy:**
`AbstractClass → ObjectClass → TechnoClass → FootClass → InfantryClass/AircraftClass/DriveClass` and
`TechnoClass → BuildingClass`. Heavy virtual function usage.

**Naming (legacy convention):** Existing classes end in `Class`, type definitions end in `Type` or
`TypeClass`, and enums often end in `Type`. This describes the original code — new code is not
required to follow these suffixes (see [Naming](#naming) below).

## Code Style & Documentation

**Follow [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)** (primary) and
[C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines). If they
conflict, ask user.

### Naming

Use [Google C++ naming](https://google.github.io/styleguide/cppguide.html#Naming) for new and
modernized code:

| Entity                  | Style                               | Example                              |
| ----------------------- | ----------------------------------- | ------------------------------------ |
| Functions               | `PascalCase`                        | `AttachObject()`, `DetachObject()`   |
| Accessors / mutators    | named like the variable             | `attached_count()`, `set_count(int)` |
| Variables (local/param) | `snake_case`                        | `cargo_hold`, `target_cell`          |
| Class data members      | `snake_case_` (trailing underscore) | `attached_count_`, `cargo_hold_`     |
| Constants               | `kPascalCase`                       | `kMaxPassengers`                     |
| Enumerators             | `kPascalCase`, always qualified     | `MultiStyle::kAnd`                   |
| Classes / types         | `PascalCase`                        | `Cargo`, `RTTIType`                  |

Legacy code uses `PascalCase` methods/members (`How_Many()`, `Quantity`) — leave it unless you are
modernizing that code, then rename to the Google scheme. Do not mass-rename untouched legacy code.

New and modernized enums are `enum class` with `kPascalCase` enumerators written `Enum::kName`; do
not add `using enum`. Legacy enums still carry `SCREAMING_CASE` enumerators hoisted by a
`using enum` after the definition; `/rename-google-style` converts them.

### Classes

In a `class`, every data member is private (not protected) and reached through accessors; only
constants may be public. A type that is only passive data is a `struct` with public `snake_case`
fields. Declarations follow the style guide's
[Declaration Order](https://google.github.io/styleguide/cppguide.html#Declaration_Order), and a
`.cc` defines its functions in the order its header declares them. `/restructure-class` applies this
to legacy classes.

### Includes

Chromium-style paths relative to the `src/` include root (configured via
`include_directories(${CMAKE_SOURCE_DIR}/src)`), so omit the `src/` prefix:

```cpp
#include "ra/objects/object.h"      // Correct
#include "engine/gfx/pixel_buffer.h"
#include "object.h"                 // WRONG - no relative paths
#include "src/ra/objects/object.h"  // WRONG - src/ is the include root, don't repeat it
```

clang-tidy's `misc-include-cleaner` requires every `.cc` file to include the headers it uses
directly and nothing more. It misses three kinds of use: a complete type needed only for an implicit
derived-to-base conversion, a header that supplies template definitions for an explicit
instantiation, and a comparison C++20 rewrites to a header's `operator<=>` (`ra/map/power.cc`'s
`Power_Fraction() < 1`). Keep those with `#include "base/fixed.h"  // IWYU pragma: keep`.

### New Files

- NO Electronic Arts copyright header (only applies to original EA code)
- Use `#ifndef` guards: `<PROJECT>_<PATH>_<FILE>_H_` (e.g., `CNC_RED_ALERT_BASE_TYPES_H_`,
  `CNC_RED_ALERT_ENGINE_GFX_PIXEL_BUFFER_H_`); `engine_layers_test` fails on a guard that does not
  match its path, so a moved header needs its guard renamed

### Documentation (Google Style - REQUIRED for new code)

```cpp
// File: Brief description of the collection of abstractions.

// Class: Purpose and usage. Include example for complex APIs.
//
// Example:
//   MyClass obj(args);
//   obj.DoThing();
class MyClass {

// Function declaration (.h): What it does, inputs, outputs, nullptr handling.
// Returns the cell at coordinates, or nullptr if out of bounds.
CellClass* Get_Cell_At(int x, int y);

// Function definition (.cc): HOW it works, not WHAT (don't repeat .h comment).
// IMPORTANT: If a .cc file has a "what" comment on a function that belongs
// in the .h declaration, MOVE it to the .h file—don't just delete it.

// Variables: Document sentinel values.
int max_units_;      // -1 means unlimited
ObjectClass* sel_;   // nullptr if nothing selected

// Implementation: Explain WHY, not WHAT.
// Fixed-point math prevents multiplayer desync.
int dist = IsqrtFixed(dx * dx + dy * dy);

// TODO: bug 12345 - Remove after v2 migration.
```

### Safe String Pattern

The C printf family is a clang-tidy error (`cppcoreguidelines-pro-type-vararg`). Format with Abseil,
which checks a literal format against its arguments at compile time:

```cpp
// sprintf(buf, "%s.INI", src)     →  absl::SNPrintF(buf, sizeof(buf), "%s.INI", src);
// std::string result              →  absl::StrFormat("%s.INI", src);
// printf / fprintf(fp, ...)       →  absl::PrintF(...) / absl::FPrintF(fp, ...);
// strcpy + strcat                 →  base::SafeCopy / base::SafeAppend (base/strings/safe_string.h)
// run-time format (string table) →  base::FormatRuntime(Text_String(id), args...) (base/strings/format.h)
```

Game printers (`Fancy_Text_Print`, `Smart_Printf`, `Fatal`, ...) are variadic templates over the
same machinery, so their call sites look unchanged but are type-checked.

## Testing

All new code should have unit tests. Use the [Google Test](https://google.github.io/googletest/)
framework.

`tools/coverage.py <files>` reports the lines and branches of those files that no test reaches. It
keeps its own clang build with source-based coverage in `build-coverage` (configured on first use,
about 2.5 minutes cold, seconds after that) and runs every gtest executable; `--tests <regex>`
narrows the run to some of them. `/cover-files` uses it to take a file to 100%. `--release-too` also
measures a build without the scenario editor and the cheat keys (in `build-coverage.release`) and
counts a line covered when either build covers it: with `BUILD_VERSION=internal` in `.env`, the
release side of an `if constexpr (config::kScenarioEditorEnabled)` is otherwise never compiled and
the tests written for it skip themselves.

Code that blocks in its own loop until the player answers (a dialog, a menu, a timed screen) is
tested with `UiFixture` (`ra/startup/ui_test_util.h`, `td/startup/ui_test_util.h`): a real `Game` on
a headless screen, a clock that moves a tick a frame and a `ScriptedPlayer` whose clicks and keys
the loop reads. Queue the script, call the dialog, check what it did; see
`ra/screens/special_options_test.cc`. A loop the script cannot end throws `ScriptOverrun` instead of
hanging.

`tools/ui_check.py` plays the scripts in `src/<game>/testdata/ui/` into the whole game
(`-SCRIPT<file>`, offscreen, about a second each) and compares their screenshots with the recorded
hashes; `--accept` records a change that was meant. It needs the game data, so it is not a ctest:
run it after changing what a screen looks like. `docs/TEST_HARNESS_PLAN.md` has the design.

The `ra_save_fixture_test` and `td_save_fixture_test` ctests each load their game's
`src/<game>/testdata/SAVEGAME.SCG01EA`, a save an earlier binary wrote
(`tools/<game>_saveload_smoke.sh --load-fixture`); they are skipped where there is no game data. One
fails when a change to what a save holds comes without a new `kSaveGameVersion`: bump the version
and regenerate the file as `src/<game>/testdata/README.md` says.

`tools/net_check.py` plays a two-player Red Alert network game between two instances on this machine
(`-NETGAME<1|2>`, no lobby) and requires their per-frame logs of object positions to be identical.
Run it after touching `ra/net/`, `EventClass` or anything a packet carries.

## Abseil

Auto-fetched via CMake. Prefer Abseil over std/custom implementations.

| Header                 | Usage                                                                  |
| ---------------------- | ---------------------------------------------------------------------- |
| `absl/log/log.h`       | `DLOG(INFO)`, `DLOG(WARNING)` (debug-only), `LOG(ERROR)`, `LOG(FATAL)` |
| `absl/log/check.h`     | `CHECK(x)`, `CHECK_EQ/NE/LT/GT`, `DCHECK` (debug-only)                 |
| `absl/strings/`        | `StrCat`, `StrSplit`, `StrFormat`                                      |
| `absl/strings/ascii.h` | `AsciiStrToLower`, `AsciiStrToUpper`                                   |
| `absl/container/`      | `flat_hash_map`, `flat_hash_set`                                       |

**String manipulation:** Never use `strdup`/`free` for temporary strings—use `std::string` with
Abseil functions instead. Example: `std::string lower = absl::AsciiStrToLower(input);`

**Logging rule:** Use `DLOG` for debug messages (compiled out in release). Original game excluded
most logging from release builds—follow this pattern. Use `CHECK` for programmer errors/invariants,
NOT for user input validation.

**CMake linking:** `target_link_libraries(mytarget PRIVATE absl::log absl::check absl::strings)`

## magic_enum

Auto-fetched via CMake (`magic_enum::magic_enum`, linked into `rasdl`). Enums hold only real values:
no `X_FIRST`/`X_COUNT` sentinels and no aliases. A negative `X_NONE` is fine; the build sets
`MAGIC_ENUM_RANGE_MIN=0`, so reflection sees exactly the index values 0..N-1 (and
`enum_name(X_NONE)` is empty).

- Array size or count: `magic_enum::enum_count<E>()`. It is a `size_t`; when comparing with an `int`
  index write `static_cast<int>(magic_enum::enum_count<E>())` rather than adding a sign-compare
  warning.
- Iteration: `for (E e : magic_enum::enum_values<E>())`. First/last value:
  `enum_values<E>().front()` / `.back()`.
- Names in debug output: `magic_enum::enum_name(value)` in `DLOG`.
- Enums with values above 127 need an `enum_range` specialization next to the enum (see VocType and
  TemplateType in `ra/core/defines.h`). Enums whose values are not 0..N-1 (bit flags, shape indices)
  are not index enums; give them constants instead.
- Each reflected enum costs ~40 ms of compile time per translation unit; `defines.h` includes the
  header, `.cc` files that reflect include `magic_enum/magic_enum.hpp` themselves.

## Legacy Code

You will encounter: `strcpy`/`strcat`/`sprintf`, raw `new`/`delete`, C-style casts, globals in
`ra/externs.h`, missing const, `WIN32`/`PORTABLE` ifdefs.

**Acceptable changes:** Safe string functions, buffer overflow fixes, add `override`, IWYU fixes,
self-contained headers, fixed-width integer types (`long` → `int32_t`, etc.).

**Const:** Declare locals, range-for variables and references `const` when they are never modified;
clang-tidy's `misc-const-correctness` enforces it. Write `const` before the type (`const int x`,
`const T* p`); `.clang-format` sets `QualifierAlignment: Left`. Make pointers point to `const` when
the target is never written, though the check does not require it: LLVM 23 misses writes through
`*p++`, so pointee warnings are off. Declare member functions `const` when they leave the object's
state alone; `readability-make-member-function-const` enforces it. A function that exists to change
game, network or UI state stays non-const even if it writes no field of its own — suppress the check
with a comment naming that state.

**Avoid unless requested:** Class hierarchy refactoring, smart pointers everywhere, STL containers
everywhere, removing globals.

## Integer Types

Use `int` as the default integer type. For other sizes, use fixed-width types from `<cstdint>`
(`int16_t`, `int32_t`, `int64_t`). Do not use `short`, `long`, or `long long`.

Avoid unsigned types (`uint32_t`, etc.) unless representing bit patterns, flags, or modular
arithmetic. Do not use unsigned merely to indicate a value is non-negative — use assertions instead.

Use `int64_t` for values that could exceed 2^31, including intermediate calculations.

A color that names a palette slot is an `engine::gfx::PaletteIndex` (`engine/gfx/palette_index.h`),
not an integer: the drawing functions, the text printers and the `game::k...` colors
(`game/ui/standard_colors.h`) all use it. Write a literal slot as `PaletteIndex{12}`, a computed one
as `PaletteIndex::FromInt(n)`, and "no color" as `std::optional<PaletteIndex>`, not `-1`. Pixel
data, remap tables and fading tables stay bytes; a player's color choice (`PlayerColorType`) is an
id, not a slot.

For indices, counts, and sizes, use `base::ssize` (defined in `base/types.h` as `std::ptrdiff_t`).
Prefer this over `size_t` to avoid signed/unsigned comparison issues and to allow negative sentinel
values. Include `"base/types.h"` and link the `base` library.

| Legacy Type                 | Replacement                                           |
| --------------------------- | ----------------------------------------------------- |
| `int`                       | Keep as `int`                                         |
| `long` / `long int`         | `int32_t` or `int64_t`                                |
| `unsigned long`             | `uint32_t` (bitfield) or `int32_t`/`int64_t` (number) |
| `short`                     | `int16_t` or `int`                                    |
| `unsigned int/short`        | Prefer signed; `uint*_t` only for bit patterns        |
| `size_t` (index/count/size) | `base::ssize`                                         |

When converting between integer types, use brace initialization (`int32_t{value}`) for safe
conversions that should fail on narrowing, or `static_cast<int32_t>(value)` when narrowing is
intentional. Do not use C-style casts like `(int)value`. See
[Type Conversion Casts](docs/TYPE_MIGRATION.md#type-conversion-casts) for details.

Omit the `std::` prefix on fixed-width types. See `docs/TYPE_MIGRATION.md` for full details.

## Tools Configuration

| Tool       | Config File                          | Notes                                                                                          |
| ---------- | ------------------------------------ | ---------------------------------------------------------------------------------------------- |
| clang-tidy | `.clang-tidy`                        | Many checks disabled for legacy code                                                           |
| IWYU       | `cmake/IWYU.cmake`, `.iwyu_mappings` | Opt-in (`-DENABLE_IWYU=ON`); advice only, never fails                                          |
| cpplint    | `CPPLINT.cfg` (root)                 | `cpplint_test` ctest via `tools/run_cpplint.py`; filters and ratchet in `docs/CPPLINT_PLAN.md` |

## Key Files

| Purpose         | File(s)                                                                                                                                                             |
| --------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Build config    | `CMakeLists.txt`, `ra/CMakeLists.txt`, `td/CMakeLists.txt`                                                                                                          |
| Global state    | `ra/externs.h`                                                                                                                                                      |
| Streams         | `engine/stream/byte_sink.h`, `engine/stream/byte_source.h`                                                                                                          |
| Graphics        | `engine/gfx/pixel_buffer.h`, `engine/gfx/palette_index.h` (the type of a color)                                                                                     |
| Engine layering | `tools/check_layers.py` (enforces the dependency order and header guards), `tools/engine_layout.py` (the folder/library table it reads), `engine_layers_test` ctest |
| Video           | `engine/video/vqa/vqa_player.h`                                                                                                                                     |

## Platform Notes

- **Linux/macOS:** Primary platforms. Requires `libsdl2-dev` (apt) or `sdl2` (brew).
- **Windows:** Links `wsock32`, includes `dde.cc`, `ccdde.cc`, `cc_icon.rc`.

## TODO List

Open work is kept by CLion's Not My TODO plugin in `.todos/items.json`, not in a Markdown file. Read
and change it only through the IDE's MCP tools, `mcp__clion__todo_*` (deferred: load all eight with
one `ToolSearch` `select:`; pass `projectPath`). Never edit or script the JSON, and do not grep it
to search: the tools do both, keep the anchors and save in one write. `/add-todo` has the
conventions (kind and area tags, `p2` for bugs and `p3` otherwise, effort, anchors).

| To                                   | Call                                                                                                                                                                                                    |
| ------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Survey or look for a duplicate       | `todo_list` with `compact: true` and filters (`text`, `anyText`, `path`, `tags`, `withoutTags`, `status`, `priority`, `effort`, `parent`, `blocked`); without `compact` it returns every item's details |
| Read one item with its anchored code | `todo_get`                                                                                                                                                                                              |
| Add one item                         | `todo_create`                                                                                                                                                                                           |
| Add or change two or more items      | one `todo_batch` (entries with an `id` change, entries without create; all or nothing)                                                                                                                  |
| Add a finding or progress note       | `todo_comment` (appends; `todo_update` with `details` replaces the whole text)                                                                                                                          |
| Make an item wait for the user       | `toDecide` on create/update (it adds the `needs-decision` tag itself)                                                                                                                                   |
| Record the user's answer             | `todo_decided`, before doing the work; it takes the `needs-decision` tag away                                                                                                                           |
| Remove a probe or a mistaken item    | `todo_delete` (only an item an agent made, with no parts and no links to it)                                                                                                                            |
| Finish an item                       | `todo_update` with `status: fixed`, `resolution` and, once committed, `fixedIn`; `wont_fix` with `resolution` to close it unchanged                                                                     |
| Re-attach after moving code          | `todo_list` with `lost: true`, then `todo_update` with `path`/`startLine`                                                                                                                               |

An item tagged `needs-decision` waits for the user: do not fix it unasked; ask with AskUserQuestion
when it is the task at hand. If a fix turns out to need the user's call, set `toDecide` instead of
guessing. Finished items are set to `fixed` for review, never deleted. Where an item comes from (a
commit or commit range) goes in `source`, not in the details. To pick what to work on:
`status: open`, `blocked: false`, `withoutTags: needs-decision`, highest priority first, least
effort within it.

**Closing an item costs one call and no commit of its own.** Commit the code first, then make one
`todo_update` with `status: fixed`, `resolution`, `fixedIn: "HEAD"` (stored as that commit's id)
and, if the code moved, the new anchor. Do not set `fixed` before the commit and `fixedIn` after it,
and do not write "Not committed" in a resolution: a missing `fixedIn` already says so. The change
this leaves in `.todos/items.json` goes into the next commit that is made anyway, not into a "Record
the commit that fixed T-n" commit. That file also holds what the user did in the IDE meanwhile
(items set to `done`, new items, shifted anchor lines); it goes along, so `git diff --stat` is
enough before committing it, not the diff.

**Record what you notice, in every session, not only in `/modernize-file`.** Something seen while
doing another task and not fixed as part of it (a bug, a suspicious unchecked value, duplicated
code, a missing test, a stale comment or doc, a tool that misbehaved) becomes a TODO item rather
than a line in the final report that is then lost. When the user is there to answer, ask whether it
belongs in the list (one question for everything noticed so far, not one per finding) and carry on
with the task meanwhile; in auto mode, or when the session runs unattended, add it without asking.
Either way check for an existing item first, follow `/add-todo`, and name the new ids in the report.

## Git Commits

- Do NOT include `Co-Authored-By` lines in commit messages.
