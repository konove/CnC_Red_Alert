---
description: 'Migrate the legacy data types in a file to modern C++: long/short/unsigned integers to fixed-width types, raw C strings (char*, char[N], strcpy/strcmp) to std::string and std::string_view, raw arrays, new[] buffers and pointer+length pairs to std::array, std::vector and std::span, and output parameters (a dest buffer, a span the function fills, a bool plus an out pointer) to return values - while leaving alone what a save game, network packet or file format depends on. Use whenever the user asks to migrate, modernize or convert the types, strings, char buffers, arrays or buffers in a file, "get rid of char*", "use spans/vectors/strings", "return it instead of filling a buffer", "get rid of the out parameters / dest buffers", or runs /migrate-types.'
---

Migrate the legacy data types in $ARGUMENTS: integers to fixed-width types, C strings to
`std::string` / `std::string_view`, raw arrays and buffers to `std::array` / `std::vector` /
`std::span`, and output parameters to return values.

The point is that a type should carry what the code otherwise keeps in its head: the size of a
buffer, who owns a string, whether a number can be negative. A `char* name` says none of that; a
`std::string_view name` says "borrowed, bounded". Work through the four families below in order -
integers, then strings, then arrays, then output parameters - because the later ones often change
function signatures and it is easier to review them after the mechanical integer changes have
settled. Read section 4 before converting any parameter in sections 2 and 3: a buffer the function
only fills should not become a `std::span` or a `std::string&` on its way to becoming a return
value.

## 0. What stays raw

Check these first; they decide most of the file. A type here is part of an external contract, and
changing it changes that contract, not just the code:

- **Anything a save game, recording or map save writes.** Each class's `Serialize()` feeds
  `ArchiveWriter`/`ArchiveReader` (`engine/stream/archive.h`) for save games (`saveload.cc`, checked
  against `kSaveGameVersion`), multiplayer recordings (`init.cc`) and map state (`iomap.cc`). The
  archive handles scalars, enums, C arrays and `char[]` directly; nothing writes a `std::string`
  today, and the one vector (`Carryover`) goes through a hand-written count-plus-elements helper,
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

Leave the data members of `Coordinate` (`packed_`), `Target` (`word_`) and `Cell` (`value_`) in
`ra/core/` and `td/core/` at their widths (RA 4/4/2 bytes, TD 4/2/2): `EventClass`'s union is sent
as raw bytes and saves write those widths, and the `static_assert`s next to each class and
`EventClass` pin them. Leave enums with explicit underlying types, and code already on fixed-width
types, alone too. On lines you change, replace C-style integer casts: `int32_t{value}` for a
conversion that must not narrow, `static_cast<int32_t>(value)` when narrowing is intended. Add
`#include <cstdint>` where fixed-width types appear.

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
| `strchr` / `strstr` / `strtok`                                    | `.find()`, `absl::StrSplit`, `base::Tokenizer`                                                                                                             |

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
| `T* data, int count` parameter pair the function reads or edits    | `std::span<const T>` when only read, `std::span<T>` when edited in place    |
| `void* buffer, int size` for bytes                                 | `std::span<const std::byte>` / `std::span<std::byte>`; no `void*` I/O       |
| Either of the two above that the function only fills               | A return value; see section 4                                               |
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

## 4. Output parameters

A function that writes its result into storage the caller hands it - `char* dest`,
`void* buffer, int size`, a `std::span<T>` it fills, a `T* out` beside a `bool` - is still a C
signature after sections 2 and 3 have given that storage a bounded type. The caller has to guess a
size, declare the object before it has a value (so it can never be `const`), and read the body to
learn which arguments are results. Return the result instead, owned or borrowed according to where
the data lives:

| Legacy                                                                  | Becomes                                                                         |
| ----------------------------------------------------------------------- | ------------------------------------------------------------------------------- |
| Fills a buffer whose size is known at compile time                      | Returns `std::array<T, N>` (`51ebbc35`: `FadingTable BuildFadingTable(...)`)    |
| Fills a buffer whose size depends on the input (decoded or loaded data) | Returns `std::vector<T>` (`5e581e48`, `624dd25c`: `ReadCpsImage()`)             |
| Builds text into `char* dest` / `std::span<char>`                       | Returns `std::string`                                                           |
| Copies out data that already lives somewhere that outlives the caller   | Returns `std::span<const T>` / `std::string_view` into it, and the copy goes    |
| `bool` (or a 0 / -1 status) plus one out parameter                      | Returns `std::optional<T>` (`ParseInteger()`, `ParseDestNet()`)                 |
| Byte count returned beside the buffer it filled                         | Returns the `std::vector`; its `.size()` is the count                           |
| Several out parameters                                                  | Returns a small named struct (`CpsImage`); with a status, `std::optional` of it |
| `T& out` / `T* out` for one value, `void` return                        | Returns `T`                                                                     |

A parameter is an output when the body writes it without reading it first and the callers pass a
fresh local or a scratch buffer they only look at afterwards. Check both: the callers are what tell
an output from an in/out.

**What stays a parameter:**

- **The caller's storage is the point.** Drawing or decoding into a page, a blit destination, a
  scratch buffer a per-frame path reuses: a return value there adds an allocation or a large copy
  per call. Returning a `std::array` to initialize a local is free; assigning a big one over an
  existing member every frame is not.
- **Stream-style reads** - `ByteSource::Read(std::span<std::byte>)`, a socket `Read`. They fill part
  of the buffer and the caller loops; that is the interface, not a legacy habit.
- **In-place edits** (`memrev`, `CopyBytes`, a delta applied over the previous frame). The parameter
  is an input too.
- **A write at an offset into a larger buffer** the caller is assembling, such as a packet.
- **Virtual chains and wide fan-out**, as in sections 0 and 2: the `Get_Packet` /
  `Get_Private_Message` family runs through `engine/net` and both games, so it moves as one change
  or not at all. List it in the report.

A destination that is a serialized `char[]` member is not a reason to keep the parameter: return the
`std::string` and let the one caller that owns the member copy it in with `base::SafeCopy`.

**The traps:**

- **The buffer size was a limit.** `dest` of 12 bytes truncated the result; a `std::string` does
  not. If a caller relied on that (a name column, a save-game description), keep the cap where that
  caller stores the value.
- **An out parameter left alone on failure is a default.** `int value = 5; Get(&value);` becomes
  `Get().value_or(5)`, not `*Get()`. Read what each caller does when the call fails before choosing
  between `std::optional` and a plain value.
- **`std::optional` holds one kind of failure.** If the status told callers why it failed and one of
  them looks, keep that distinction (an enum result, or the status beside the value in a struct).
  And do not make "empty" mean "failed" where an empty result was valid.
- **A null out pointer meant "I don't want this one."** Return the whole struct anyway when it is
  cheap to compute; split the function when it is not.
- **A view must point at something that outlives the call**: the MIX data, a string table, a member
  of an object the caller holds - never a local, and never a static buffer the next call overwrites
  (section 2 turns that one into a `std::string`).
- **A caller that passed one buffer to several calls** was accumulating or reusing it. Appending
  becomes `absl::StrAppend` or `insert` at that caller; reuse in a loop may be the first bullet of
  "what stays".

Finish at the callers: the declaration and the call collapse into
`const auto table = BuildFadingTable(...);`, and the local buffer, its size constant and any `count`
argument that only described it go. A template over the array's size removes a `count` parameter the
same way (`TranslucencyTable<N>`). Update the declaration comment: "fills `dest` with" becomes
"returns", and says what an empty optional means.

## Process

1. Read the file. List every candidate under sections 1-4 and strike those that section 0 keeps raw,
   noting why.
2. For each signature you will change, grep its declarations and callers (`grep -rn '\bName *('` in
   the owning game, or all of `src/` for a shared directory). Count the callers that need edits and
   check which ones can pass null. For an output parameter, also read what each caller passes as the
   destination and what it does with it when the call fails.
3. Convert family by family. Change declarations, definitions and every caller together; fix the
   comments that describe the old type (a "pointer to buffer" parameter doc, a `-1 means none`
   sentinel that is now `.empty()`).
4. Add the includes each new type needs (`<string>`, `<string_view>`, `<span>`, `<array>`,
   `<vector>`, `<cstdint>`, `"base/types.h"`); `misc-include-cleaner` will insist.

## Verify

- If the file computes something others store or compare - a hash, a checksum, an encoded value -
  pin its outputs with a test before converting (`engine/crypto/key_phrase_hash_test.cc` holds the
  historical password codes), and run it after. A type change that alters one byte of the input
  changes every result.
- `git clang-format -f -- <touched files>`.
- `cmake --build build --parallel 22 && ctest --test-dir build --output-on-failure`; both games if a
  shared directory changed.
- The strict checks on the files you touched: `tools/strict_tu.py <touched files>` when a
  multi-stage run has deferred the full pass, otherwise `cmake --build build-strict --parallel 14`,
  in the foreground as its own command. New `std::string` members and spans are where
  `misc-include-cleaner`, the bounds checks and lifetime warnings speak up. Changing a header
  re-analyzes every file that includes it, which can surface an unused include that was there before
  (`init.cc`'s `base/numeric.h` when `init.h` gained `<string_view>`); removing it is part of the
  change.
- If anything a save touches changed despite section 0, run `tools/ra_saveload_smoke.sh` (or `td_`)
  against the build.

## Report

- Counts per conversion (`3x long -> int32_t`, `2x const char* -> std::string_view`,
  `1x T* + count -> std::span<T>`, `2x out-span -> returned std::array`).
- Output parameters kept, each with the reason from section 4 (caller's storage, stream read,
  in-place, virtual chain).
- What stayed raw and why, per section 0 - especially serialized members, which are a save-format
  change the user may want next.
- Signatures left alone because of their fan-out, with the caller count.
- Warnings: virtual chains, printf-family calls that reference changed types, callers that passed
  null, and any ownership bug the conversion exposed.
