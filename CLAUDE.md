# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a fork of EA's Command & Conquer Red Alert and Tiberian Dawn, ported to use SDL2 for cross-platform compatibility. The codebase is C++23 with legacy C code from the original 1990s games.

## Build Commands

### Standard Build
```bash
cmake -Bbuild
cmake --build build
```

This produces two executables:
- `build/ra/rasdl` - Red Alert
- `build/td/tdsdl` - Tiberian Dawn

### Build Modes

**Quick iteration (disable strict checks and IWYU):**
```bash
cmake -Bbuild -DSTRICT_CHECKS=OFF -DENABLE_IWYU=OFF
cmake --build build
```

**With AddressSanitizer for memory debugging:**
```bash
cmake -Bbuild -DENABLE_ASAN=ON
cmake --build build
```

**Disable only IWYU (keep clang-tidy and warnings):**
```bash
cmake -Bbuild -DENABLE_IWYU=OFF
cmake --build build
```

### Rebuild After CMake Changes
```bash
cmake --build build
# or for clean rebuild:
rm -rf build && cmake -Bbuild && cmake --build build
```

### Build Specific Targets
```bash
# Build only Red Alert
cmake --build build --target rasdl

# Build only Tiberian Dawn
cmake --build build --target tdsdl

# Build specific library
cmake --build build --target tech
cmake --build build --target sdllib

# Build all header verification targets (checks headers are self-contained)
cmake --build build --target all_verify_interface_header_sets

# Build header verification for specific target
cmake --build build --target rasdl_verify_interface_header_sets
```

## Code Quality Tools

### Include What You Use (IWYU)

IWYU analyzes C++ headers and suggests include changes. It runs automatically during builds when `ENABLE_IWYU=ON` (default with `STRICT_CHECKS=ON`).

**Installation:**
```bash
# Linux
sudo apt install iwyu

# macOS
brew install include-what-you-use
```

**Configuration:**
- Settings: `cmake/IWYU.cmake`
- Mappings: `.iwyu_mappings` (project-specific header rules)
- Documentation: `docs/IWYU.md`

**Known Issues:**
- IWYU can segfault on complex headers (e.g., `ra/externs.h`)
- Verification targets automatically disable IWYU to avoid crashes
- IWYU warnings are informational and don't fail builds

### clang-tidy

Enabled automatically when `STRICT_CHECKS=ON` (default). Runs during compilation.

**Configuration:** `.clang-tidy` (extensive suppression list for legacy code)

**Note:** Many modern checks are disabled because this is 1990s-era game code being modernized incrementally.

### Abseil

The project uses [Abseil](https://abseil.io/) (fetched automatically via CMake FetchContent). New code should prefer Abseil utilities over standard library alternatives or custom implementations when available.

**Common Abseil libraries:**
- `absl/log/check.h` - CHECK/DCHECK assertions
- `absl/strings/` - String utilities (StrCat, StrSplit, StrFormat, etc.)
- `absl/container/` - Containers (flat_hash_map, flat_hash_set, etc.)
- `absl/types/` - Type utilities (optional, span, variant)
- `absl/time/` - Time utilities

**CHECK/DCHECK macros** ([Chromium style](https://chromium.googlesource.com/chromium/src/+/main/styleguide/c++/checks.md)):
```cpp
#include "absl/log/check.h"

CHECK(ptr != nullptr);           // Crashes in all builds if false
CHECK_NE(divisor, 0);            // Crashes if divisor == 0
CHECK_EQ(a, b);                  // Crashes if a != b
CHECK_LT(index, size);           // Crashes if index >= size
DCHECK(expensive_check());       // Only evaluated in debug builds
```

**When to use CHECK:**
- Use `CHECK` for invariants that should never be violated (programmer errors)
- Use `CHECK` to guard against impossible states (e.g., division by zero that "can't happen")
- Use `DCHECK` for expensive checks only needed during development
- Do NOT use for validating user input or external data (use normal error handling)

**Linking:** Targets using Abseil must link to the appropriate targets:
```cmake
target_link_libraries(mytarget PRIVATE absl::check absl::log absl::strings)
```

### Header Verification

CMake can verify all headers compile standalone (following Google C++ Style Guide):

```bash
# Verify all headers
cmake --build build --target all_verify_interface_header_sets
```

**How it works:** For targets with `VERIFY_INTERFACE_HEADER_SETS` property enabled, CMake generates synthetic `.cxx` files that only include one header each, ensuring headers are self-contained.

## High-Level Architecture

### Component Structure

```
port/           - Minimal portability layer (string utilities)
sdllib/         - SDL2 graphics/audio abstraction
tech/           - Data transformation (compression, encryption, Pipe/Straw pattern)
jshell/         - Sprite rendering and rotation
winvq/vqa32/    - VQA video codec (Westwood proprietary format)
ra/             - Red Alert game-specific code (~200+ files)
td/             - Tiberian Dawn game-specific code (~288 files)
```

### Dependency Graph

```
abseil-cpp (fetched via FetchContent)
port (standalone)
  └─ sdllib (depends: SDL2, abseil)
      ├─ tech (depends: sdllib, port, vqa32)
      ├─ jshell (depends: sdllib, port)
      └─ vqa32 (depends: port, SDL2)
          ├─ rasdl (depends: tech, jshell, port, sdllib, vqa32, abseil)
          └─ tdsdl (depends: port, sdllib, vqa32)
                    └─ Note: TD doesn't use tech or jshell
```

### Shared Libraries

**tech/** - Core utilities and data transformation:
- **Pipe/Straw pattern**: Composable data transformation chains
  - `Pipe`: Output-driven (push data through filters)
  - `Straw`: Input-driven (pull data through filters)
  - Implementations: LCW/LZO/LZW/PK compression, Blowfish encryption, Base64, CRC
- File I/O: `WWFile`, `RAMFile`, `CDFile`
- Containers: `Buffer`, `Rect`, color utilities
- Math: Fixed-point arithmetic
- Timers, random numbers, checksums

**jshell/** - Graphics rendering:
- `Rotate_Bitmap()`: Fast bitmap rotation using 256-entry sine/cosine lookup tables
- Sprite manipulation

**sdllib/** - SDL2 abstraction layer:
- Graphics: `GraphicViewPortClass`, `GraphicBufferClass`, drawing functions
- Audio: AUD file format support
- Input: Keyboard and mouse handling
- System: Palette, fonts, timers, file I/O, shape loading

**vqa32/** - Video codec:
- VQA (Westwood's proprietary video format) playback
- Single-threaded and multi-threaded players
- Audio/video synchronization

### Game-Specific Code

**ra/** (Red Alert):
- Uses all libraries: tech, jshell, sdllib, vqa32, port
- Defines: `ENGLISH=1`, `PORTABLE=1`
- ~200+ source files

**td/** (Tiberian Dawn):
- Uses: sdllib, vqa32, port (no tech or jshell)
- Defines: `PORTABLE=1`, `TD=1`
- Imports some files from ra/ (`field.cpp`, `packet.cpp`, `2keyfbuf.cpp`, `winasm.cpp`, `face.cpp`)
- More selective source list (explicit files vs. GLOB)

### Include Path Convention

**Chromium-style includes**: All paths are relative to project root.

```cpp
// Correct
#include "ra/object.h"
#include "sdllib/include/gbuffer.h"
#include "tech/pipe.h"

// Incorrect
#include "object.h"
#include "../include/gbuffer.h"
```

This is enforced by `include_directories(${CMAKE_SOURCE_DIR})` in root CMakeLists.txt.

### Class Hierarchy Pattern

Game objects follow a deep inheritance hierarchy:

```
AbstractClass (base for all game objects)
  └─ ObjectClass (placeable map objects)
      ├─ TechnoClass (combat units/buildings)
      │   ├─ FootClass (mobile units)
      │   │   ├─ InfantryClass
      │   │   ├─ AircraftClass
      │   │   └─ DriveClass (ground vehicles → UnitClass)
      │   └─ BuildingClass
      ├─ AnimClass (animations)
      ├─ TemplateClass (map terrain templates)
      ├─ OverlayClass
      ├─ TerrainClass
      └─ [other object types]
```

Each class has heavy use of virtual functions for polymorphic behavior.

### Naming Conventions

- **Classes**: Suffix `Class` (e.g., `ObjectClass`, `GraphicBufferClass`)
- **Type definitions**: Suffix `Type` or `TypeClass` (e.g., `BuildingTypeClass`)
- **Enums**: Often suffix `Type` (e.g., `HousesType`, `MissionType`)

### CMakeLists.txt Patterns

**RA approach** (ra/CMakeLists.txt):
```cmake
# Use GLOB_RECURSE then exclude unwanted files
file(GLOB_RECURSE RA_SOURCES CONFIGURE_DEPENDS "*.cpp")
list(REMOVE_ITEM RA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/unwanted.cpp" ...)

file(GLOB_RECURSE RA_HEADERS CONFIGURE_DEPENDS "*.h")
list(REMOVE_ITEM RA_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/unwanted.h" ...)
```

**TD approach** (td/CMakeLists.txt):
```cmake
# Explicit file list
add_executable(tdsdl WIN32
    super.cpp
    abstract.cpp
    aircraft.cpp
    ...
)
```

**Header verification setup:**
```cmake
set_target_properties(rasdl PROPERTIES
    ENABLE_EXPORTS ON
    VERIFY_INTERFACE_HEADER_SETS ON
)
```

## Common Code Patterns

### Legacy Code Considerations

This is 1990s game code being modernized. You will encounter:

- **Old-style string functions**: `strcpy`, `strcat`, `sprintf` (being replaced with `strncpy`, `strncat`, `snprintf`)
- **Raw pointers**: Extensive use of `new`/`delete` and pointer arithmetic
- **C-style casts**: Legacy code uses C-style casts extensively
- **Global variables**: Many globals in `ra/externs.h`
- **No const-correctness**: Many functions should be const but aren't
- **Platform #ifdefs**: `WIN32`, `PORTABLE`, conditional compilation

### When Modernizing Code

**Acceptable changes:**
- Replace unsafe string functions with safe versions
- Fix obvious buffer overflows or memory leaks
- Add `override` to virtual functions
- Fix include issues flagged by IWYU
- Make headers self-contained

**Avoid unless explicitly requested:**
- Refactoring class hierarchies
- Replacing raw pointers with smart pointers globally
- Adding const everywhere
- Modernizing to use STL containers
- Removing global variables

The project is being modernized incrementally. Keep changes focused and don't over-engineer.

### Code Style

New code and rewritten code should closely follow these style guides:
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)

If the two guides disagree on a particular point, ask the user which approach to follow, providing pros and cons of each option.

### New Files

When creating new files, do NOT add the Electronic Arts copyright header. That header only applies to original EA code.

**Header guards:** Use `#ifndef` guards (not `#pragma once`) following [Google's naming convention](https://google.github.io/styleguide/cppguide.html#The__define_Guard). The format is `<PATH>_<FILE>_H_` based on the file's path from project root:

```cpp
// File: port/check.h
#ifndef PORT_CHECK_H_
#define PORT_CHECK_H_

// ... content ...

#endif  // PORT_CHECK_H_
```

```cpp
// File: sdllib/include/gbuffer.h
#ifndef SDLLIB_INCLUDE_GBUFFER_H_
#define SDLLIB_INCLUDE_GBUFFER_H_

// ... content ...

#endif  // SDLLIB_INCLUDE_GBUFFER_H_
```

### Safe String Replacement Pattern

```cpp
// Old (unsafe)
char buffer[128];
strcpy(buffer, source);
strcat(buffer, ".INI");

// Better
char buffer[128];
strncpy(buffer, source, sizeof(buffer) - 1);
buffer[sizeof(buffer) - 1] = '\0';
strncat(buffer, ".INI", sizeof(buffer) - strlen(buffer) - 1);

// Best
char buffer[128];
size_t len = strlen(source);
snprintf(buffer, sizeof(buffer), "%s.INI", source);
```

For `strncat`: `strncat(dest, src, sizeof(dest) - strlen(dest) - 1)`

### IWYU Segfaults

If IWYU crashes during header verification builds:

1. This is a known issue with complex headers (e.g., `ra/externs.h`)
2. Verification targets automatically disable IWYU
3. IWYU still runs on actual source files (not synthetic verification files)
4. The crashes don't affect normal builds

## Platform-Specific Notes

### Windows
- Links `wsock32` library
- Includes platform-specific files: `dde.cpp`, `ccdde.cpp`, `cc_icon.rc`
- MSVC disables specific warnings for legacy code (see root CMakeLists.txt)

### Linux/macOS
- Primary development platform
- Requires SDL2: `sudo apt install libsdl2-dev` (Linux) or `brew install sdl2` (macOS)
- Uses GCC or Clang

### Emscripten (Experimental)
- WebAssembly builds with special linker flags
- See `EMSCRIPTEN` conditionals in ra/CMakeLists.txt

## Compiler Flags

**With STRICT_CHECKS=ON** (default):
- Extensive warning flags: `-Wall -Wextra -Wpedantic -Wshadow -Wconversion ...` (20+ warning flags)
- clang-tidy enabled
- IWYU enabled

**With STRICT_CHECKS=OFF**:
- All warnings disabled: `-w`
- No clang-tidy
- No IWYU
- Fast iteration builds

## Key Files Reference

- **Root build**: `CMakeLists.txt`
- **RA build**: `ra/CMakeLists.txt`
- **TD build**: `td/CMakeLists.txt`
- **IWYU config**: `cmake/IWYU.cmake`, `.iwyu_mappings`
- **clang-tidy config**: `.clang-tidy`
- **Global externs**: `ra/externs.h` (massive header with all game globals)
- **Pipe/Straw pattern**: `tech/pipe.h`, `tech/straw.h`
- **Graphics core**: `sdllib/include/gbuffer.h`, `sdllib/include/drawbuff.h`
- **Video codec**: `winvq/vqa32/vqaplay.h`
