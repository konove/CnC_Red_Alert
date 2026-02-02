Migrate legacy integer types in $ARGUMENTS to portable fixed-width types from `<cstdint>`.

## Rules

Follow the conversion table in `docs/TYPE_MIGRATION.md`. Summary:

- **`int`**: Keep as-is.
- **`long`**: Replace with `int32_t` (if value fits 32 bits) or `int64_t` (if could be large).
- **`unsigned long`**: Replace with `uint32_t` if used for CRC/hash/bitfield/flags/version; replace with `int32_t`
  or `int64_t` if used as a number (count, timer, index, quantity).
- **`short`**: Replace with `int16_t` (if size matters) or `int` (if no size constraint).
- **`unsigned short`**: Replace with `uint16_t` if protocol/bitfield/flags; replace with `int16_t` or `int` if
  a number.
- **`long long`**: Replace with `int64_t`.
- **`unsigned long long`**: Replace with `uint64_t` (bitfield) or `int64_t` (number).
- **`size_t`** used as index, count, or size: replace with `base::ssize` (from `"base/types.h"`). Keep `size_t` only
  at STL/external API boundaries.

Unsigned is justified for: CRC/hash/checksum values, bitwise operations, flag masks, raw byte/pixel data, network
wire format fields, modular arithmetic, magic numbers/version identifiers compared as opaque values.

Unsigned should be converted to signed for: counts, indices, loop bounds, health, damage, speed, distance, timers,
return codes indicating a quantity.

When replacing legacy types, also replace C-style integer casts on affected lines with C++ alternatives. Prefer
`int32_t{value}` (brace init) for compile-time-safe conversions; use `static_cast<int32_t>(value)` when narrowing is
intentional.

## Process

1. **Read the target file** and identify every occurrence of `long`, `unsigned long`, `short`, `unsigned short`,
   `long long`, `unsigned long long`.

2. **Skip do-not-touch items**:
   - Game typedefs: `LEPTON`, `COORDINATE`, `CELL`, `TARGET` (and their definitions)
   - Enums with explicit underlying types (`enum Foo : type`)
   - `bool`, `char` for text, `size_t` (at STL boundaries), `ptrdiff_t`, `uintptr_t`, `base::ssize`
   - External API signatures (SDL2, system calls, POSIX)
   - Code already using fixed-width types (`int32_t`, `uint16_t`, etc.)

3. **For each occurrence**, determine if the value is a number or bit pattern by examining:
   - How the variable is used (arithmetic vs bitwise operations)
   - What it represents (read comments, variable names, surrounding context)
   - Whether it participates in comparisons with signed or unsigned values
   - If the value is an index, count, or size, prefer `base::ssize` over `int`/`int32_t`/`int64_t`
   - When converting an unsigned type to a signed type, verify the code does not rely on unsigned wraparound
     behavior. If it does (e.g., frame counters, modular arithmetic), keep the type unsigned.

4. **Apply the conversion** using the Edit tool. Change both declarations and corresponding function signatures.

5. **Add `#include <cstdint>`** if not already present and fixed-width types were introduced. Place it in the
   correct include order (C system headers section).

6. **Replace C-style integer casts** on lines being modified. Use brace initialization (`int32_t{value}`) for
   compile-time-safe conversions, or `static_cast<int32_t>(value)` when narrowing is intentional. Leave casts on
   unmodified lines alone.

7. **Warnings to emit** (report at end, do not skip the migration):

   - **Virtual/override functions**: If a changed function is `virtual` or `override`, list all files in the
     inheritance chain that also need updating. Search for the function name across the codebase to find them.
   - **Serialization patterns**: If the file contains `Pipe.Put`, `Pipe.Get`, `memcpy` with `sizeof`, or
     `#pragma pack`, warn that binary format will change.
   - **printf format specifiers**: If any `printf`, `sprintf`, `snprintf`, or `fprintf` calls reference changed
     types, flag them for migration to `absl::StrFormat`/`absl::StrCat`. Do not change printf calls in this pass
     unless trivial.

8. **Report summary** at the end:
   - Number of types changed, grouped by conversion (e.g., "3x `long` -> `int32_t`")
   - List of warnings (virtual chains, serialization, printf)
   - Any types that were skipped and why

## Allowed Tools

Read, Grep, Glob, Edit
