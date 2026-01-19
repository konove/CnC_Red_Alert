# CLAUDE.md

C++23 port of EA's Command & Conquer Red Alert and Tiberian Dawn using SDL2. Legacy 1990s game code being modernized
incrementally.

## Quick Reference

| Target                 | Command                                                         | Output                              |
|------------------------|-----------------------------------------------------------------|-------------------------------------|
| Both games             | `cmake -Bbuild && cmake --build build`                          | `build/ra/rasdl`, `build/td/tdsdl`  |
| Red Alert only         | `cmake --build build --target rasdl`                            | `build/ra/rasdl`                    |
| Tiberian Dawn only     | `cmake --build build --target tdsdl`                            | `build/td/tdsdl`                    |
| Fast build (no checks) | `cmake -Bbuild -DSTRICT_CHECKS=OFF`                             | Disables clang-tidy, IWYU, warnings |
| With ASan              | `cmake -Bbuild -DENABLE_ASAN=ON`                                | Memory debugging                    |
| Header verification    | `cmake --build build --target all_verify_interface_header_sets` | Checks headers are self-contained   |
| Clean rebuild          | `rm -rf build && cmake -Bbuild && cmake --build build`          |                                     |

## Setup

### Required Dependencies

**All platforms:** SDL2, C++23 compiler

**Linux (Debian/Ubuntu):**

```bash
sudo apt update
sudo apt install libsdl2-dev clang-tidy
```

**macOS:**

```bash
brew install sdl2 llvm
```

**Note:** If clang-tidy is not installed, either install it (above) or build with `-DSTRICT_CHECKS=OFF` to disable
static analysis.

## Architecture

```
port/        → Portability layer (string utilities) [standalone]
sdllib/      → SDL2 abstraction (graphics, audio, input) [depends: SDL2, abseil]
tech/        → Compression, encryption, Pipe/Straw pattern [depends: sdllib, port, vqa32]
jshell/      → Sprite rendering, bitmap rotation [depends: sdllib, port]
vqa32/       → VQA video codec [depends: port, SDL2]
ra/          → Red Alert (~200 files) [uses: ALL libraries]
td/          → Tiberian Dawn (~288 files) [uses: sdllib, vqa32, port only - NO tech/jshell]
```

**Class hierarchy:** `AbstractClass → ObjectClass → TechnoClass → FootClass → InfantryClass/AircraftClass/DriveClass`
and `TechnoClass → BuildingClass`. Heavy virtual function usage.

**Naming:** Classes end in `Class`, type definitions end in `Type` or `TypeClass`, enums often end in `Type`.

## Code Style & Documentation

**Follow [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)** (primary) and
[C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines). If they conflict, ask user.

### Includes

Chromium-style paths relative to project root:

```cpp
#include "ra/object.h"           // Correct
#include "sdllib/include/gbuffer.h"
#include "object.h"              // WRONG - no relative paths
```

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

// Function definition (.cpp): HOW it works, not WHAT (don't repeat .h comment).

// Variables: Document sentinel values.
int max_units_;      // -1 means unlimited
ObjectClass* sel_;   // nullptr if nothing selected

// Implementation: Explain WHY, not WHAT.
// Fixed-point math prevents multiplayer desync.
int dist = IsqrtFixed(dx * dx + dy * dy);

// TODO: bug 12345 - Remove after v2 migration.
```

### Safe String Pattern

```cpp
// Old (unsafe)           →  snprintf(buf, sizeof(buf), "%s.INI", src);
// strcpy + strcat        →  Or: strncpy + null-terminate + strncat(dest, src, sizeof(dest)-strlen(dest)-1)
```

## Testing

All new code should have unit tests. Use the [Google Test](https://google.github.io/googletest/) framework.

## Abseil

Auto-fetched via CMake. Prefer Abseil over std/custom implementations.

| Header                 | Usage                                                                  |
|------------------------|------------------------------------------------------------------------|
| `absl/log/log.h`       | `DLOG(INFO)`, `DLOG(WARNING)` (debug-only), `LOG(ERROR)`, `LOG(FATAL)` |
| `absl/log/check.h`     | `CHECK(x)`, `CHECK_EQ/NE/LT/GT`, `DCHECK` (debug-only)                 |
| `absl/strings/`        | `StrCat`, `StrSplit`, `StrFormat`                                      |
| `absl/strings/ascii.h` | `AsciiStrToLower`, `AsciiStrToUpper`                                   |
| `absl/container/`      | `flat_hash_map`, `flat_hash_set`                                       |

**String manipulation:** Never use `strdup`/`free` for temporary strings—use `std::string` with Abseil functions
instead. Example: `std::string lower = absl::AsciiStrToLower(input);`

**Logging rule:** Use `DLOG` for debug messages (compiled out in release). Original game excluded most logging from
release builds—follow this pattern. Use `CHECK` for programmer errors/invariants, NOT for user input validation.

**CMake linking:** `target_link_libraries(mytarget PRIVATE absl::log absl::check absl::strings)`

## Legacy Code

You will encounter: `strcpy`/`strcat`/`sprintf`, raw `new`/`delete`, C-style casts, globals in `ra/externs.h`,
missing const, `WIN32`/`PORTABLE` ifdefs.

**Acceptable changes:** Safe string functions, buffer overflow fixes, add `override`, IWYU fixes, self-contained
headers.

**Avoid unless requested:** Class hierarchy refactoring, smart pointers everywhere, const everywhere, STL containers
everywhere, removing globals.

## Tools Configuration

| Tool       | Config File                          | Notes                                                      |
|------------|--------------------------------------|------------------------------------------------------------|
| clang-tidy | `.clang-tidy`                        | Many checks disabled for legacy code                       |
| IWYU       | `cmake/IWYU.cmake`, `.iwyu_mappings` | Can segfault on `ra/externs.h`; warnings don't fail builds |

## Key Files

| Purpose      | File(s)                                                    |
|--------------|------------------------------------------------------------|
| Build config | `CMakeLists.txt`, `ra/CMakeLists.txt`, `td/CMakeLists.txt` |
| Global state | `ra/externs.h`                                             |
| Pipe/Straw   | `tech/pipe.h`, `tech/straw.h`                              |
| Graphics     | `sdllib/include/gbuffer.h`, `sdllib/include/drawbuff.h`    |
| Video        | `winvq/vqa32/vqaplay.h`                                    |

## Platform Notes

- **Linux/macOS:** Primary platforms. Requires `libsdl2-dev` (apt) or `sdl2` (brew).
- **Windows:** Links `wsock32`, includes `dde.cpp`, `ccdde.cpp`, `cc_icon.rc`.
- **Emscripten:** Experimental WebAssembly support.

## Git Commits

- Do NOT include `Co-Authored-By` lines in commit messages.
