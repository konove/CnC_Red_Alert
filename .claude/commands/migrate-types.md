---
description: 'Migrate the legacy data types in a file to modern C++: long/short/unsigned integers to fixed-width types, raw C strings (char*, char[N], strcpy/strcmp) to std::string and std::string_view, and raw arrays, new[] buffers and pointer+length pairs to std::array, std::vector and std::span - while leaving alone what a save game, network packet or file format depends on. Use whenever the user asks to migrate, modernize or convert the types, strings, char buffers, arrays or buffers in a file, "get rid of char*", "use spans/vectors/strings", or runs /migrate-types.'
---

Migrate the legacy data types in $ARGUMENTS: integers to fixed-width types, C strings to
`std::string` / `std::string_view`, and raw arrays and buffers to `std::array` / `std::vector` /
`std::span`.

The point is that a type should carry what the code otherwise keeps in its head: the size of a
buffer, who owns a string, whether a number can be negative. A `char* name` says none of that; a
`std::string_view name` says "borrowed, bounded". Work through the three families below in order -
integers, then strings, then arrays - because the later ones often change function signatures and it
is easier to review them after the mechanical integer changes have settled.

## 0. What stays raw

Check these first; they decide most of the file. A type here is part of an external contract, and
changing it changes that contract, not just the code:

- **Anything a save game, recording or map save writes.** Each class's `Serialize()` feeds
  `ArchiveWriter`/`ArchiveReader` (`tech/archive.h`) for save games (`saveload.cc`, checked against
  `kSaveGameVersion`), multiplayer recordings (`init.cc`) and map state (`iomap.cc`). The archive
  handles scalars, enums, C arrays and `char[]` directly; nothing writes a `std::string` today, and
  the one vector (`Carryover`) goes through a hand-written count-plus-elements helper,
  `SerializeCarryover()`, that validates the count on load. So converting a serialized member is
  possible, but it changes the save format: a helper like that one, a `kSaveGameVersion` bump, and
  the save/load smoke test. That is its own change; list the member in the report instead.
- **Wire and file formats**: structs sent over the network, read from MIX/AUD/SHP/INI data, or
  copied with `memcpy`/`sizeof`, `#pragma pack`, `static_assert(sizeof(...))`. Their layout is the
  format.
- **Globals in `ra/externs.h` / `td/externs.h`** and anything declared in another file's header:
  they are that file's migration, and usually tree-wide. Exception: a global only this file uses.
- **Virtual signatures whose base is elsewhere**: the whole chain moves together or not at all.
- **Hot kernels** (the blit, LCW, SHA-1, Blowfish, anything in `optimize_in_debug()`): use a
  `std::span` checked once at the entry, but do not add per-element container calls to the inner
  loop.
- **A C API boundary** (SDL, POSIX, the `IO_*` routines): keep the raw type in the call and convert
  right beside it (`.c_str()`, `.data()`).

## 1. Integer types

Follow the conversion table in `docs/TYPE_MIGRATION.md`. Summary:

- **`int`**: Keep as-is.
- **`long`**: `int32_t` if the value fits 32 bits, `int64_t` if it could be large.
- **`unsigned long`**: `uint32_t` for CRC/hash/bitfield/flags/version; `int32_t` or `int64_t` for a
  number (count, timer, index, quantity).
- **`short`**: `int16_t` if size matters, `int` otherwise. **`unsigned short`**: `uint16_t` for
  protocol/bitfield/flags, `int16_t` or `int` for a number.
- **`long long`**: `int64_t`. **`unsigned long long`**: `uint64_t` (bits) or `int64_t` (number).
- **`size_t`** used as an index, count or size: `base::ssize` (`"base/types.h"`). Keep `size_t` only
  at STL/external API boundaries.

Unsigned is justified for CRC/hash/checksum values, bitwise operations, flag masks, raw byte/pixel
data, wire-format fields, modular arithmetic, and magic numbers compared as opaque values. Counts,
indices, loop bounds, health, damage, speed, distance and timers become signed - but first check the
code does not rely on unsigned wraparound (frame counters, modular arithmetic); if it does, keep it
unsigned.

Leave `LEPTON`, `COORDINATE`, `CELL`, `TARGET`, enums with explicit underlying types, and code
already on fixed-width types alone. On lines you change, replace C-style integer casts:
`int32_t{value}` for a conversion that must not narrow, `static_cast<int32_t>(value)` when narrowing
is intended. Add `#include <cstdint>` where fixed-width types appear.

## 2. Strings

Decide by what the code does with the characters:

| Legacy                                                            | Becomes                                                                                                                                                    |
| ----------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `const char*` parameter that is only read                         | `std::string_view`                                                                                                                                         |
| `const char*` parameter the callee keeps after returning          | `std::string` (take by value and move, or `const std::string&` if it only copies)                                                                          |
| `char buf[N]` local built with `SNPrintF` / `SafeCopy` / `strcat` | `std::string` from `absl::StrFormat` / `absl::StrCat`                                                                                                      |
| `char name[N]` member, not serialized                             | `std::string`                                                                                                                                              |
| `char*` returned from a function                                  | `std::string` if it pointed at a static or temporary buffer; `std::string_view` if it points into data that outlives the caller (a string table, MIX data) |
| `strcmp` / `stricmp` / `strncmp`                                  | `==`, `absl::EqualsIgnoreCase`, `absl::StartsWith`                                                                                                         |
| `strlen`                                                          | `.size()`                                                                                                                                                  |
| `strchr` / `strstr` / `strtok`                                    | `.find()`, `absl::StrSplit`, `port::Tokenizer`                                                                                                             |

The traps, all of which have happened here:

- **A `string_view` is not NUL-terminated.** Never pass `view.data()` to anything that reads to the
  NUL (`IO_*`, SDL, `strlen`, a legacy printer). Convert with `std::string(view).c_str()` at that
  call, or keep that parameter a `std::string`.
- **`nullptr` does not convert.** A `const char*` caller that can pass null (a default argument, an
  optional name) is undefined behaviour as a `string_view`. Find those callers first; the empty view
  usually takes over the meaning of null (`e4f79a6b`: `FileName()` returns an empty view where it
  used to return nullptr), and the null checks in the body become `.empty()` - unless null and the
  empty string meant different things. `Obfuscate(nullptr)` returned 0 while `Obfuscate("")` hashes
  the padded empty phrase; no real caller passed null, so the null check went and `""` kept its
  code, and the test that passed `nullptr` was changed rather than the result.
- **Aliasing.** A view of a member that the function is about to overwrite dangles mid-call; copy it
  first (`e4f79a6b`, `CDFileClass::SetName`).
- **A fixed buffer can be a limit, not just storage.** `char name[12]` for a player name or an edit
  field caps the length the player can type. If the size is behaviour, keep the cap explicitly (a
  `kMaxNameLength` check) when moving to `std::string`, or keep the buffer.
- **Text laid out in a struct for the wire or a save** stays a `char[]`; see section 0.

Changing a parameter from `const char*` to `std::string_view` leaves existing callers compiling, so
it is usually cheap. A signature whose callers would all need editing is not: when that fan-out is
large (`a4259c88` left `Add_Message()`, 59 callers, and `WWMessageBox::Process()`, 155, alone),
convert inside the file only and list the signature in the report.

## 3. Arrays and buffers

| Legacy                                                             | Becomes                                                                     |
| ------------------------------------------------------------------ | --------------------------------------------------------------------------- |
| `T* data, int count` parameter pair                                | `std::span<T>` (`std::span<const T>` when only read)                        |
| `void* buffer, int size` for bytes                                 | `std::span<std::byte>` / `std::span<const std::byte>`; no `void*` I/O       |
| `T arr[N]` local or non-serialized member                          | `std::array<T, N>`                                                          |
| `T arr[N]` indexed by an enum                                      | `base::EnumArray<Enum, T>`                                                  |
| `new T[n]` / `delete[]`, `malloc` / `free`, `realloc`-grown arrays | `std::vector<T>`                                                            |
| Pointer into someone else's data kept as a member                  | `std::span<const T>` (`1a751f35`: type images are a span into the MIX data) |
| Array plus a separate `count` member                               | `std::vector<T>`, if the count is only its size                             |

Keep what the code relies on:

- **Indexing stays checked, with the tree's idiom per container.** `cppcoreguidelines-pro-bounds-*`
  is enforced. A C array or span goes through `base::At(arr, i)`, which takes any integer. A
  `std::array`, `std::vector` or `base::EnumArray` uses `.at()`, which takes `size_t`, so an `int`
  index is `arr.at(base::ToSize(i))` (336 sites) - a bare `arr.at(i)` fails `-Wsign-conversion` in
  the strict build. A modulo index becomes `arr.at(base::ToSize(i) % arr.size())`. If a local is
  indexed by `int` everywhere and never passed anywhere, weigh whether `std::array` buys anything
  over the C array that `base::At` already checks.
- **A `std::array` local needs `{}`** (`cppcoreguidelines-pro-type-member-init`), even when the next
  line fills it.
- **A vector reallocates.** Code that keeps a pointer or span into an array across an insertion
  breaks when the array becomes a growing `std::vector`; reserve, or keep indices instead.
- **Ownership should end up in one place** (`cd997c9d`: `ListClass` owns its strings instead of
  every dialog allocating them). If converting a buffer shows that two owners freed it, or none did,
  that is a bug: note it and fix it as its own change.
- **Don't dress dead parameters up as spans.** A pointer parameter nobody reads is dead code
  (`a4259c88` dropped `argc`/`argv` rather than converting them); list it for `/remove-dead-code`.

## Process

1. Read the file. List every candidate under sections 1-3 and strike those that section 0 keeps raw,
   noting why.
2. For each signature you will change, grep its declarations and callers (`grep -rn '\bName *('` in
   the owning game, or all of `src/` for a shared directory). Count the callers that need edits and
   check which ones can pass null.
3. Convert family by family. Change declarations, definitions and every caller together; fix the
   comments that describe the old type (a "pointer to buffer" parameter doc, a `-1 means none`
   sentinel that is now `.empty()`).
4. Add the includes each new type needs (`<string>`, `<string_view>`, `<span>`, `<array>`,
   `<vector>`, `<cstdint>`, `"base/types.h"`); `misc-include-cleaner` will insist.

## Verify

- If the file computes something others store or compare - a hash, a checksum, an encoded value -
  pin its outputs with a test before converting (`obfuscate_test.cc` holds the historical password
  codes), and run it after. A type change that alters one byte of the input changes every result.
- `git clang-format -f -- <touched files>`.
- `cmake --build build --parallel 22 && ctest --test-dir build --output-on-failure`; both games if a
  shared directory changed.
- `cmake --build build-strict --parallel 14`, in the foreground as its own command. New
  `std::string` members and spans are where `misc-include-cleaner`, the bounds checks and lifetime
  warnings speak up. Changing a header re-analyzes every file that includes it, which can surface an
  unused include that was there before (`init.cc`'s `base/numeric.h` when `init.h` gained
  `<string_view>`); removing it is part of the change.
- If anything a save touches changed despite section 0, run `tools/ra_saveload_smoke.sh` (or `td_`)
  against the build.

## Report

- Counts per conversion (`3x long -> int32_t`, `2x const char* -> std::string_view`,
  `1x T* + count -> std::span<T>`).
- What stayed raw and why, per section 0 - especially serialized members, which are a save-format
  change the user may want next.
- Signatures left alone because of their fan-out, with the caller count.
- Warnings: virtual chains, printf-family calls that reference changed types, callers that passed
  null, and any ownership bug the conversion exposed.
