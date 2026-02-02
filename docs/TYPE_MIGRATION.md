# Integer Type Migration Guide

Migrate legacy C++ integer types (`long`, `short`, `unsigned long`, etc.) to portable fixed-width types from
`<cstdint>`, following the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).

## Background: Why This Migration Is Needed

The C++ standard only guarantees minimum sizes for built-in integer types. The actual sizes depend on the platform's
**data model**, and `long` in particular differs between Linux and Windows:

### Type Sizes Across Platforms

| Type        | DOS 16-bit | Win32 (ILP32) | Win64 (LLP64) | Linux x86_64 (LP64) | macOS ARM64 (LP64) |
|-------------|------------|---------------|---------------|---------------------|--------------------|
| `short`     | 16         | 16            | 16            | 16                  | 16                 |
| `int`       | 16         | 32            | 32            | 32                  | 32                 |
| **`long`**  | **32**     | **32**        | **32**        | **64**              | **64**             |
| `long long` | N/A        | 64            | 64            | 64                  | 64                 |
| `size_t`    | 16         | 32            | 64            | 64                  | 64                 |
| `void*`     | 16/32      | 32            | 64            | 64                  | 64                 |

The critical row is `long`: it is 32 bits on Windows x64 but 64 bits on Linux x86_64. Code that uses `long` assuming
a particular size will silently produce different behavior on different platforms. This affects:

- Binary serialization (save games, network packets)
- `sizeof()` computations
- Bitfield masks and shifts
- printf format specifiers (`%ld` vs `%d`)

This codebase was originally written for DOS/Win32 where `long` was always 32 bits. The migration to fixed-width
types makes the code correct on all platforms.

## Integer Overflow Behavior

Understanding overflow semantics is important when migrating between signed and unsigned types:

- **Signed integer overflow is undefined behavior (UB) in C++.** The compiler may assume it never happens and
  optimize accordingly. Code like `if (x + 1 < x)` (overflow check) can be silently removed by the optimizer.
- **Unsigned arithmetic wraps modulo 2^n** (defined behavior). `UINT32_MAX + 1` is guaranteed to produce `0`.

### Implications for Type Migration

When converting an `unsigned long` (which wraps on overflow) to `int32_t` (where overflow is UB), the conversion is
safe **as long as the value never actually overflows** in practice. This is the common case in this codebase — most
`unsigned long` variables hold counts, timers, or coordinates that stay well within 32-bit signed range.

**When to keep unsigned:** If the code relies on wraparound behavior (e.g., a frame counter that intentionally wraps,
or modular arithmetic for checksums), the type must stay unsigned. The [decision flowchart](#decision-flowchart-is-this-unsigned-type-a-number-or-a-bit-pattern)
already handles this case: "modular arithmetic (intentional wraparound)" directs to `uint32_t`.

**When to use `int64_t`:** If a value could exceed 2^31 but does not rely on wraparound, widen to `int64_t` rather
than keeping it unsigned. This preserves defined behavior while accommodating large values.

## Core Rules

1. **`int` is the default** -- keep `int` for general-purpose integers. It is at least 32 bits on all supported
   platforms.
2. **Replace `short`, `long`, `long long`** -- use fixed-width types from `<cstdint>`.
3. **Avoid unsigned types for numbers** -- only use `uint32_t` etc. for bit patterns, flags, or modular arithmetic.
   Do not use unsigned merely to assert non-negative.
4. **Use `int64_t` for big values** -- when a value could exceed 2^31 or intermediate calculations could overflow.
5. **`size_t`** is acceptable in STL interfaces. For indices, counts, and sizes in project code, prefer
   **`base::ssize`** (from `"base/types.h"`), a signed type alias for `std::ptrdiff_t`.
6. **No `std::` prefix** on fixed-width types (`int32_t`, not `std::int32_t`).
7. **Use `absl::StrFormat`/`absl::StrCat`** for string formatting, not printf format specifiers.

## Conversion Table

| Legacy Type                          | Replacement                                                      | Rationale                                  |
|--------------------------------------|------------------------------------------------------------------|--------------------------------------------|
| `int`                                | **Keep as `int`**                                                | Portable (>= 32 bits everywhere)           |
| `unsigned int` (number)              | `int` + assertion if needed                                      | Unsigned should not mean "non-negative"    |
| `unsigned int` (bitfield/flags)      | `uint32_t`                                                       | Bitfields are a valid unsigned use case    |
| `long`                               | `int32_t` (if value fits 32 bits) or `int64_t` (if could be big) | `long` is 32-bit on Win64, 64-bit on Linux |
| `unsigned long` (number)             | `int32_t` or `int64_t`                                           | Avoid unsigned for numbers                 |
| `unsigned long` (CRC/hash/bitfield)  | `uint32_t`                                                       | Bit patterns are valid unsigned use        |
| `short`                              | `int16_t` or `int` (if no size constraint)                       | Use fixed-width when size matters          |
| `unsigned short` (number)            | `int16_t` or `int`                                               | Avoid unsigned for numbers                 |
| `unsigned short` (bitfield/protocol) | `uint16_t`                                                       | Bit patterns are valid unsigned use        |
| `long long`                          | `int64_t`                                                        | Use standard fixed-width type              |
| `unsigned long long`                 | `uint64_t` (bitfield) or `int64_t` (number)                      | Same rules apply                           |
| `size_t` (index/count/size)          | `base::ssize` (from `"base/types.h"`)                            | Signed; avoids signed/unsigned mismatches  |
| `size_t` (STL interface boundary)    | **Keep as `size_t`**                                             | Match STL/external API expectations        |

### Decision Flowchart: Is This Unsigned Type a Number or a Bit Pattern?

```
Is the variable used for any of these?
  - CRC, hash, or checksum computation
  - Bitwise operations (&, |, ^, ~, <<, >>)
  - Flag masks or packed bitfields
  - Raw byte data, color values, pixel data
  - Network protocol fields matching a wire format
  - Modular arithmetic (intentional wraparound)
  - Magic numbers or version identifiers compared as opaque values
    |
    YES --> Keep as unsigned: use uint16_t, uint32_t, or uint64_t
    |
    NO --> Is it a count, index, quantity, distance, speed,
           health, damage, timer, or other numeric value?
           |
           YES --> Convert to signed: int, int16_t, int32_t, or int64_t
           |
           NO --> Examine context more carefully. When in doubt, prefer signed.
```

### Codebase Examples

**Keep unsigned** (bit pattern / CRC):

```cpp
// src/ra/event.h -- CRC in network frame info
uint32_t CRC;

// src/ra/saveload.cc -- save game version (opaque identifier)
unsigned long version = SAVEGAME_VERSION;  // --> uint32_t version = SAVEGAME_VERSION;

// src/ra/version.h -- version numbers compared as opaque values
unsigned long Version_Number();            // --> uint32_t Version_Number();

// src/ra/nullmgr.h -- magic number for protocol identification
unsigned short MagicNum;                   // --> uint16_t MagicNum;
```

**Convert to signed** (numeric value):

```cpp
// src/ra/conquer.cc -- coordinate accumulators
long minx = 0x7FFFFFFFL;                  // --> int32_t minx = 0x7FFFFFFFL;

// src/ra/team.cc -- accumulated coordinates
long x = 0;                               // --> int32_t x = 0;
long y = 0;                               // --> int32_t y = 0;

// src/ra/nullmgr.h -- timing values
unsigned long RetryDelta;                  // --> int32_t RetryDelta;
unsigned long MaxRetries;                  // --> int32_t MaxRetries;
unsigned long Timeout;                     // --> int32_t Timeout;

// src/ra/score.h -- elapsed time counter
unsigned long ElapsedTime;                 // --> int32_t ElapsedTime;

// src/ra/jshell.cc -- file size
long size = file.Size();                   // --> int32_t size = file.Size();

// src/ra/cell.h -- cell identifier
short ID;                                  // --> int16_t ID;
```

**Convert to `base::ssize`** (index / count / size):

```cpp
// src/ra/vector.h -- container size and index type
int Length() const;                        // --> base::ssize Length() const;
int ID(const T& ptr);                      // --> base::ssize ID(const T& ptr);
base::ssize VectorMax;                     // capacity (include "base/types.h", link base)
```

## Type Conversion Casts

When migrating types, also replace C-style integer casts on the same lines with C++ alternatives. This is
opportunistic — only fix casts in code being touched for type migration. Do not go on a codebase-wide cast hunt.

### Cast Alternatives (Preferred Order)

1. **Brace initialization** (`int32_t{value}`) — safest option. The compiler will reject implicit narrowing
   conversions at compile time. Use for constant expressions and values known to fit the target type.

   ```cpp
   // Compile error if value doesn't fit — catches bugs at build time.
   int64_t y = int64_t{1} << 42;
   int16_t n = int16_t{NumBufs};  // Error if NumBufs could narrow
   ```

2. **`static_cast<T>(value)`** — use when narrowing is intentional or the value range has been verified at runtime.
   This is the standard replacement for C-style numeric casts.

   ```cpp
   // Intentional narrowing: we know the enum value fits in int.
   int speed = static_cast<int>(MPH_LIGHT_SPEED);
   // Intentional truncation: caller guarantees value fits 16 bits.
   int16_t count = static_cast<int16_t>(NumBufs);
   ```

3. **`reinterpret_cast<T>(value)`** — only for pointer-to-integer or integer-to-pointer conversions. Common in legacy
   DOS addressing patterns. Use `uintptr_t` as the integer type for pointer round-trips.

   ```cpp
   // Pointer to integer (legacy pattern).
   uintptr_t addr = reinterpret_cast<uintptr_t>(ListenHeader);
   ```

4. **`std::bit_cast<T>(value)`** (C++20) — for type punning, where you want to reinterpret the raw bits of one type
   as another type of the same size. Prefer over `reinterpret_cast<T&>` or `memcpy` for value types.

   ```cpp
   // Reinterpret the bits of a float as uint32_t.
   uint32_t bits = std::bit_cast<uint32_t>(some_float);
   ```

### Codebase Examples

```cpp
// Before (C-style cast)                        // After
(int)MPH_LIGHT_SPEED                            static_cast<int>(MPH_LIGHT_SPEED)
(short)NumBufs                                  static_cast<int16_t>(NumBufs)
(unsigned long)ListenHeader                     reinterpret_cast<uintptr_t>(ListenHeader)
(long)(center_x - x)                            static_cast<int32_t>(center_x - x)
```

## Do-Not-Touch List

The following should **not** be changed during type migration:

- **Game typedefs already using fixed-width types** (defined in `ra/defines.h`):
    - `LEPTON` (`unsigned short` -- game-specific unit, change only as a separate task)
    - `COORDINATE` (`uint32_t`)
    - `CELL` (`signed short` -- game-specific unit, change only as a separate task)
    - `TARGET` (`int32_t`)

- **Enums with explicit underlying types**:
    - `enum EventType : uint8_t`
    - `enum HousesType : int8_t`
    - `enum TemplateType : uint16_t`
    - `enum DirType : uint8_t`
    - Any `enum Foo : <type>` declaration

- **Standard type aliases**: `bool`, `char` (for text), `size_t` (at STL boundaries; project code should prefer
  `base::ssize` for indices/counts/sizes), `ptrdiff_t`, `uintptr_t`, `intptr_t`, `base::ssize`

- **External API signatures**: SDL2 functions, system calls, POSIX APIs. Match whatever the API expects.

- **Bitfield unions already correct**: `TARGET_COMPOSITE`, `COORD_COMPOSITE`, and similar packed unions.

- **Code in `wwflat32/`**: Legacy code, not actively compiled.

## Serialization-Sensitive Areas

These areas require **extra care** (not avoidance) when migrating. Changing the size of a type in a serialized
struct will change the binary format, invalidating old save games or breaking network compatibility. This is
**acceptable** for this project, but must be done deliberately:

### Save/Load (`src/ra/saveload.cc`)

The save game version hash includes `sizeof()` of many classes. Changing member sizes will change the version hash,
which is the correct behavior (old saves with old layouts should not load with new code).

Pattern to watch for:

```cpp
pipe.Put(&variable, sizeof(variable));  // sizeof changes if type changes
pipe.Get(&variable, sizeof(variable));
```

When changing types in serialized structs, verify that both the `Put` and `Get` paths use the same type.

### Network Events (`src/ra/event.h`)

`EventClass` uses a union of structs for multiplayer packets. These are compared with `memcmp` for equality.
All players in a multiplayer game must use the same struct layout.

Bit fields (`unsigned Frame : 26`) should not be changed without understanding the protocol.

### Network Connection Classes

- `src/ra/connect.h`, `src/ra/nullconn.h` -- connection header structs
- `src/ra/ipxconn.h`, `src/ra/ipxgconn.h` -- IPX packet structures
- `src/ra/noseqcon.h` -- non-sequenced connection stats

### Packed Structures (`#pragma pack`)

`src/ra/mixfile.h` uses `#pragma pack(push, 1)` for binary file headers:

```cpp
#pragma pack(push, 1)
struct FileHeader {
  std::int16_t count;  // Already migrated
  std::int32_t size;   // Already migrated
};
#pragma pack(pop)
```

Any struct within a `#pragma pack` block is reading/writing a binary format. Verify the intended byte sizes.

## printf Migration

When touching a file for type migration, also migrate printf-family format strings that reference changed types.
Replace with `absl::StrFormat` or `absl::StrCat`:

```cpp
// Before
sprintf(buf, "Score: %ld", score);

// After
std::string buf = absl::StrFormat("Score: %d", score);  // int32_t uses %d
// Or for simple concatenation:
std::string buf = absl::StrCat("Score: ", score);
```

Only migrate format strings in code you are already changing. Do not go on a project-wide printf hunt.

## Virtual Function Chains

When a virtual function uses a legacy type in its signature, the **entire inheritance chain** must be updated
consistently. The compiler will catch mismatches (the override will fail to match the base), but this means a
single-file migration may require touching multiple files.

Common chains in this codebase:

```
AbstractClass -> ObjectClass -> TechnoClass -> FootClass -> InfantryClass/AircraftClass/DriveClass
TechnoClass -> BuildingClass
```

When encountering a virtual function with a legacy type:

1. Find the base class declaration
2. Find all overrides (search for the function name across the codebase)
3. Change all of them in the same commit

## Step-by-Step: Migrating a Single File

1. **Read the file** and identify all legacy integer types (`long`, `unsigned long`, `short`, `unsigned short`,
   `long long`, `unsigned long long`).

2. **For each occurrence**, determine the category:
    - Is it in the do-not-touch list? Skip it.
    - Is it a number or a bit pattern? Apply the conversion table.
    - Is it in a serialized struct? Note for the summary but still convert.
    - Is it a virtual function signature? Note that the whole chain needs updating.

3. **Choose the replacement type**:
    - `long` that held values fitting 32 bits -> `int32_t`
    - `long` that could hold large values or was 64-bit on the original platform -> `int64_t`
    - `unsigned long` CRC/hash/version -> `uint32_t`
    - `unsigned long` counter/timer -> `int32_t`
    - `short` with size constraint -> `int16_t`
    - `short` without size constraint -> `int` (but `int16_t` is fine too for consistency)
    - `unsigned short` protocol/bitfield -> `uint16_t`
    - `unsigned short` number -> `int16_t` or `int`
    - `size_t` used as index/count/size -> `base::ssize` (include `"base/types.h"`, link `base`)

4. **Add `#include <cstdint>`** if not already present and fixed-width types were introduced.

5. **Replace C-style integer casts** on lines being modified. Use brace initialization (`int32_t{value}`) for
   compile-time-safe conversions, or `static_cast<int32_t>(value)` when narrowing is intentional. Leave casts on
   unmodified lines alone. See [Type Conversion Casts](#type-conversion-casts) for details.

6. **Update format strings** if any printf-family calls reference the changed types.

7. **Check for virtual/override** -- if the function is virtual, update all overrides.

8. **Build and test** to verify the change compiles.

## Migration Priority

1. **`long` / `unsigned long`** -- highest priority, these have different sizes on Win64 vs Linux x86_64.
2. **`short` / `unsigned short`** -- lower priority, same size everywhere but still non-portable by spec.
3. **`unsigned int` used for numbers** -- lowest priority, more of a style improvement.
