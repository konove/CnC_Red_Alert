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
nothing else. The worst kernels (RSA bignum, Blowfish, SHA-1, LCW, the fading table, the window
blit) are compiled at `-O2` even in `Debug` via `optimize_in_debug()` in
`cmake/OptimizeInDebug.cmake`; add a file there only if a Debug profile shows it hot.

## Setup

### Required Dependencies

**All platforms:** SDL2, C++23 compiler, Ninja

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

ccache caches only the compile; clang-tidy runs as a separate pass in front of it and is cached by
`clang-tidy-cache` (below). IWYU is off by default, `STRICT_CHECKS` included: its suggestions never
fail a build, `misc-include-cleaner` already enforces `.cc` includes as an error, and it re-parses
every TU uncached (~950 CPU-s per full strict pass). Turn it on with `-DENABLE_IWYU=ON` to review
header includes by hand.

**The static analyzer runs shallow locally and deep in CI.** `CLANG_ANALYZER_MODE` (`shallow` by
default, or `deep`) sets clang-tidy's `-analyzer-config mode=`. Shallow halves the analyzer's cost
and takes `ra/ioobj.cc` from 62 s to 4 s; the CI lint job configures `deep`, so a path only deep
mode explores still fails there. To reproduce a CI-only analyzer finding, reconfigure a strict dir
with `-DCLANG_ANALYZER_MODE=deep` or run clang-tidy on the one TU with
`--extra-arg=-Xclang --extra-arg=-analyzer-config --extra-arg=-Xclang --extra-arg=mode=deep`.

**`clang-tidy-cache` only caches a translation unit whose preprocess is silent.** It derives its
hash by re-running the compiler to preprocess the TU and gives up on any compiler output to stderr
(`hash_inputs` returns `None`), and under `-Weverything` a single preprocessor warning in a widely
included header is enough to disable it tree-wide. The `_MAX_PATH`-style defines in the former
`src/port/ex_string.h` used to do exactly that: 14/40 RA TUs were cacheable. With them renamed
(`kMaxPath` and friends) and the reserved MIDL macros in `src/port/win32/win32_com.h` and
`src/ra/wolapi/wolapi.h` wrapped in
`#pragma clang diagnostic ignored "-Wreserved-macro-identifier"`, all 908 project TUs preprocess
silently. Measured 2026-09-12 over the first 40 RA objects, deleted before each run with a fresh
`CTCACHE_DIR`: 33.3 s cold, 8.7 s warm (compile from ccache and IWYU still run). Keep new headers
preprocessor-quiet, or the cache silently stops working for everything that includes them.

That bail-out is also what prints `ERROR:clang-tidy-cache:Error executing compile command: #[...]`
during a strict build — one line per translation unit whose preprocess warns. It is noise, not a
failure: the build continues uncached. Since every unit is quiet today, seeing it again means some
header started warning and caching is off for the units that include it.
`CTCACHE_LOG_LEVEL=CRITICAL` hides the message; `-DUSE_CTCACHE=OFF` drops the wrapper entirely.

The cache defaults to `/tmp/ctcache-$USER`, which does not survive a reboot; set
`CTCACHE_DIR=~/.cache/ctcache` to keep it.

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
port/        → Portability layer (string utilities) [standalone]
base/        → Header-only utilities: types.h (base::ssize), algorithm.h, trig.h [standalone]
sdllib/      → SDL2 abstraction (graphics, audio, input) [depends: SDL2, abseil]
winvq/       → VQA video codec (vqa32, vqm32, …; target name `vqa32`) [depends: port, SDL2]
tech/        → Compression, encryption, ByteSink/ByteSource streams [depends: sdllib, port, vqa32]
ra/          → Red Alert (~200 files) [depends: tech, sdllib, port, vqa32]
td/          → Tiberian Dawn (~288 files) [depends: tech, sdllib, port, vqa32]
```

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
| Classes / types         | `PascalCase`                        | `Cargo`, `RTTIType`                  |

Legacy code uses `PascalCase` methods/members (`How_Many()`, `Quantity`) — leave it unless you are
modernizing that code, then rename to the Google scheme. Do not mass-rename untouched legacy code.

### Includes

Chromium-style paths relative to the `src/` include root (configured via
`include_directories(${CMAKE_SOURCE_DIR}/src)`), so omit the `src/` prefix:

```cpp
#include "ra/object.h"           // Correct
#include "sdllib/include/gbuffer.h"
#include "object.h"              // WRONG - no relative paths
#include "src/ra/object.h"       // WRONG - src/ is the include root, don't repeat it
```

clang-tidy's `misc-include-cleaner` requires every `.cc` file to include the headers it uses
directly and nothing more. It misses two kinds of use: a complete type needed only for an implicit
derived-to-base conversion, and a header that supplies template definitions for an explicit
instantiation. Keep those with `#include "td/vector_impl.h"  // IWYU pragma: keep`.

### New Files

- NO Electronic Arts copyright header (only applies to original EA code)
- Use `#ifndef` guards: `<PROJECT>_<PATH>_<FILE>_H_` (e.g., `CNC_RED_ALERT_PORT_CHECK_H_`,
  `CNC_RED_ALERT_SDLLIB_INCLUDE_GBUFFER_H_`)

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
// strcpy + strcat                 →  port::SafeCopy / port::SafeAppend (port/safe_string.h)
// run-time format (string table) →  port::FormatRuntime(Text_String(id), args...) (port/format.h)
```

Game printers (`Fancy_Text_Print`, `Smart_Printf`, `Fatal`, ...) are variadic templates over the
same machinery, so their call sites look unchanged but are type-checked.

## Testing

All new code should have unit tests. Use the [Google Test](https://google.github.io/googletest/)
framework.

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
  TemplateType in `ra/defines.h`). Enums whose values are not 0..N-1 (bit flags, shape indices) are
  not index enums; give them constants instead.
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

| Tool       | Config File                          | Notes                                                 |
| ---------- | ------------------------------------ | ----------------------------------------------------- |
| clang-tidy | `.clang-tidy`                        | Many checks disabled for legacy code                  |
| IWYU       | `cmake/IWYU.cmake`, `.iwyu_mappings` | Opt-in (`-DENABLE_IWYU=ON`); advice only, never fails |

## Key Files

| Purpose      | File(s)                                                    |
| ------------ | ---------------------------------------------------------- |
| Build config | `CMakeLists.txt`, `ra/CMakeLists.txt`, `td/CMakeLists.txt` |
| Global state | `ra/externs.h`                                             |
| Streams      | `tech/byte_sink.h`, `tech/byte_source.h`                   |
| Graphics     | `sdllib/include/gbuffer.h`, `sdllib/include/drawbuff.h`    |
| Video        | `winvq/vqa32/vqaplay.h`                                    |

## Platform Notes

- **Linux/macOS:** Primary platforms. Requires `libsdl2-dev` (apt) or `sdl2` (brew).
- **Windows:** Links `wsock32`, includes `dde.cc`, `ccdde.cc`, `cc_icon.rc`.
- **Emscripten:** Experimental WebAssembly support.

## Git Commits

- Do NOT include `Co-Authored-By` lines in commit messages.
