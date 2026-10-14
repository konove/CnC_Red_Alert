# Union Access: Typed Payloads and Plain Fields Instead of Unions

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development
> (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use
> checkbox (`- [ ]`) syntax for tracking.

**Goal:** Remove every union from the project's code (not SDL's), so that
`cppcoreguidelines-pro-type-union-access` can be enabled in `.clang-tidy` with zero findings.

**Architecture:** The unions fall into two groups. The _wire unions_ (the event payload and the
network packet payloads) keep their exact bytes: each becomes `base::Payload<Arms...>`, a byte array
with the size and alignment of the union. Its arms become named structs, copied out with `Get<T>()`
and written back with `Update<T>(edit)`. The _in-memory unions_ (cell occupancy bits, trigger-event
data, team-mission argument, sync-bug trap pointer, network list nodes, two engine bit-packing
helpers) become plain fields, small classes or separate structs. Golden-byte tests written against
the unions come first and pin the wire bytes. The save fixture tests pin the saved bytes.

**Tech stack:** C++23, GoogleTest, the strict clang build, `tools/strict_tu.py`,
`tools/net_check.py`, the save/load smoke scripts.

**Spec:** this document's Findings, and the "Buffer, union and boolean conversion policy
(2026-09-12)" review in [CLANG_TIDY_PRIORITIES.md](CLANG_TIDY_PRIORITIES.md). This plan supersedes
that review's union paragraph.

## Findings

Measured 2026-10-14 on `63b317596`. Every `src/` TU in `cmake-build-strict-ra-clang`'s compile db
(655 files, both games) was run with
`clang-tidy --checks=-*,cppcoreguidelines-pro-type-union-access`: 2,936 reports, **1,165 unique
sites**. The 2026-09-12 figure of 1,438 counted header sites once per includer and still had the
coordinate unions, which [COORDINATE_TYPES_PLAN.md](COORDINATE_TYPES_PLAN.md) removed since.

Eight engine TUs (`aud_decoder.cc`, `lcw.cc`, `xor_delta.cc`, `blowfish.cc`, `pk.cc`,
`sha1_compress.cc`, `adpcm_decoders.cc`, `vq_decoder.cc`) failed to compile in that db because its
generated Rust bridge header was stale. None of them declares or uses a union, so the count stands.
Rerun the sweep from a fresh strict build anyway (Task 11).

`SDL_Event` is a union, but its accesses (`window_events.cc`, `keyboard.cc`, their tests) are not
reported: 0 sites, checked on `window_events_test.cc` directly. SDL's own types are out of scope.

### The unions

| Union                                  | Declared                                             | RA sites    | TD sites | Bytes fixed by                      |
| -------------------------------------- | ---------------------------------------------------- | ----------- | -------- | ----------------------------------- |
| `EventClass::Data`                     | `ra/net/event.h:146`, `td/net/event.h:135`           | 190         | 153      | wire (raw bytes, compared as bytes) |
| `GlobalPacketType`, `SerialPacketType` | `ra/net/session.h:320,387`, `td/core/defines.h:2368` | ~330        | 106      | wire (`sizeof()` raw sends)         |
| `NodeNameTag`                          | `ra/net/session.h:293`, `td/net/nodename.h:16`       | ~95         | 37       | RA save (Player arm only)           |
| `TEventClass::Data`                    | `ra/ai/tevent.h:159`                                 | 74          | —        | RA save (`Data.Value`, `int32_t`)   |
| `CellClass::Flag`                      | `ra/objects/cell.h:209`, `td/objects/cell.h:176`     | 42          | 36       | save (`Flag.Composite`, one byte)   |
| `TeamMissionType::argument_`           | `ra/ai/team_type.h:120`                              | 15          | —        | RA save (`argument_.value`)         |
| `TrapObjectType`                       | `ra/net/session.h:446`, `td/startup/conquer.cc:210`  | 14          | 13       | nothing (debugger aid)              |
| `PacketType`                           | `engine/codec/base64.cc:98`                          | 14 (engine) |          | nothing (bit packing)               |
| `Int`                                  | `engine/crypto/blowfish_reference_test_util.cc:24`   | 44 (engine) |          | nothing (test-only reference)       |

(~21 RA `Chat` sites could be either the node or the packet arm; the split does not matter for the
work.)

### What the bytes depend on

- **Events** go on the wire as raw bytes and are compared as raw bytes. Every `EventClass`
  constructor delegates to the default one, which zero-fills the whole object, padding included. RA
  sends only `EventLength[type]` bytes of `Data` (`ra/net/event.cc:118`); the table is written as
  `sizeof(Data.<arm>)`, and three entries use a prefix: `Specific.Type`, `FrameInfo.Delay`,
  `Variable.Size`. `static_assert(sizeof(EventClass) == 32)` (RA) and `== 24` (TD) hold the size.
  `HousesType` is `int8_t`, so arms like `Anim` have interior padding. It is zero only because the
  storage was zeroed before the fields were stored.
- **Packets** are zero-filled with `base::FillBytes(base::ObjectBytes(packet), 0, sizeof(...))`,
  then filled and sent with `sizeof()`. `static_assert(sizeof(SerialPacketType) == 160)` and
  `sizeof(GlobalPacketType) == 144` (RA) hold the size. Several arms hold bit-fields and
  `SpecialFlags`, which is a bit-field struct.
- **A union member store writes only that member's bytes.** Code that writes `Mission` after
  `MegaMission` keeps `MegaMission`'s tail. `Payload::Update<T>` keeps that rule: it copies `T`'s
  `sizeof(T)` bytes out and back, padding included, and leaves the rest of the storage alone. A
  rewrite therefore never changes a byte the old code did not change, provided every write goes
  through `Update` (or `Set` of a value that came from `Get`).
- **Saves:** `ar(Flag.Composite, Land)` (both `iomap.cc`), `ar(Event, ..., Data.Value)`
  (`ra/saveload/ioobj.cc:181`), `ar(mission_, argument_.value)` (`ra/ai/team_type.h:83`) and
  `NodeNameTag::Serialize` (`ra/saveload/saveglobals.cc:113`, Player arm only). Each replacement
  writes the same values in the same order. `ra_save_fixture_test` and `td_save_fixture_test` catch
  a difference.

### Decisions

- **Wire unions: typed payload bytes** (chosen by the user 2026-10-14 over `std::variant` plus
  field-wise wire encoding). The protocol and the bytes are unchanged.
- The arms of the event and packet payloads keep their legacy field names (`Whom`, `Credits`, ...)
  so the rewrite stays mechanical. Their struct names are new and get a `Data` suffix
  (`MegaMissionData`). A bare `Target` arm would hide `ra::Target` inside `EventClass`.
- Reading an arm the selector does not carry is a programmer error. `EventClass::data<T>()` and
  `UpdateData<T>()` `DCHECK` that `Type` carries `T`. The packets get the same check against
  `Command`. A `DCHECK` only fires in a Debug build, so Task 11 runs the Debug ctest.
- `TEventClass` keeps its union's partial-write behavior for the one narrow arm: `HousesType` is one
  byte, and `set_house()` replaces only the low byte of the stored word.

### Out of scope

- Renaming the payload fields to `snake_case`: a later `/rename-google-style` pass on `event.h` and
  `session.h`.
- Encoding the packets field by field: the alternative the user rejected.
- `SDL_Event`.
- T-360 (delete the C++ reference kernels): if it lands first, Task 2's Blowfish half disappears
  with `blowfish_reference_test_util.cc`.

## Global Constraints

- No change to any byte that goes on the wire or into a save. The golden tests, the save fixture
  tests, `tools/net_check.py` and both save/load smoke scripts must pass unchanged. Bump no
  `kSaveGameVersion`.
- Keep the existing `static_assert`s on `sizeof(EventClass)`, `sizeof(SerialPacketType)`,
  `sizeof(GlobalPacketType)` and `std::is_trivially_copyable_v<EventClass>`. Add `static_assert`s
  for TD's `GlobalPacketType` size and RA's `alignof(EventClass)` in Task 6, before the rewrite.
- Every write to a payload arm goes through `Update<T>` (or `Set<T>` of a value read with `Get<T>`).
  Never build a fresh `T` and `Set` it: its padding would be indeterminate.
- Set the selector (`Type`, `Command`) before writing the arm, or the `DCHECK` fires.
- Follow CLAUDE.md: Google naming for new code, `#ifndef` guards matching the path,
  `misc-include- cleaner`-clean includes, `const` locals, `base::ssize` for sizes, no
  `Co-Authored-By` in commits.
- Each task ends green on `tools/strict_tu.py <changed .cc files>` and on the ctests it names. The
  full strict build gates the last commit of each Part.

## Review Focus

1. **A pointer into an arm outlives the copy.** `Get<T>()` returns a copy, so `info.Scenario` points
   into a local. A call that keeps such a pointer (a message list that stores `const char*`, a
   `strtok` cursor, a list box item) would dangle where the union pointer did not. Check every
   `char[]` arm field that is passed to a function. Task 9's review step lists them.
2. **Two arms written in sequence.** Code that writes arm A and then arm B relies on B's write
   leaving A's tail alone. `Update` preserves that. A `Set` of a freshly built struct would not;
   grep the diff for `SetData` / `.Set<` and check each source.
3. **The selector set after the payload.** Constructors and packet senders that fill the arm before
   setting `Type`/`Command` trip the `DCHECK` only in Debug. Task 11 runs `ctest` in `build-debug`
   and one `net_check` round.
4. **`HousesType` events in the trigger editor.** `set_house(HOUSE_NONE)` must leave the upper three
   bytes as they were, or a saved scenario's `[Trigs]` value changes. Task 4 pins this.
5. **Bit-field layout in the occupancy byte.** `Center` is bit 0 and `Building` is bit 7 under the
   old bit-fields (GCC/Clang allocate from the low bit on every target we build). `OccupyFlags` must
   match, or old saves load with the wrong cells occupied. Task 5 pins this with a test and with the
   save fixtures.

---

## Part A: Shared helper and the in-memory unions

### Task 1: `base::Payload`

**Files:**

- Create: `src/base/payload.h`
- Create: `src/base/payload_test.cc`
- Modify: `src/base/CMakeLists.txt` (add `payload_test.cc` to `base_test`'s `SOURCES`)

**Interfaces:**

- Produces: `template <class... Arms> class base::Payload` with `Get<T>() const -> T`,
  `Update<T>(std::invocable<T&> auto&& edit)`, `Set<T>(const T&)`, `bytes() -> std::span<std::byte>`
  (and `const`), and `static constexpr bool kHolds<T>`. `sizeof` and `alignof` are those of
  `union { Arms... }`.

- [ ] **Step 1: Write the failing test**

```cpp
// Tests for base::Payload: union layout, copy-out reads, and writes that
// change only the written arm's bytes.

#include "base/payload.h"

#include <cstddef>
#include <cstdint>

#include "gtest/gtest.h"

namespace base {
namespace {

struct Pair {
  int32_t first;
  int8_t second;  // three padding bytes follow
};
struct Narrow {
  int16_t value;
};
struct Pointer {
  void* address;
};

static_assert(sizeof(Payload<Pair, Narrow>) == 8);
static_assert(alignof(Payload<Pair, Narrow>) == 4);
static_assert(sizeof(Payload<Narrow, Pointer>) == sizeof(void*));
static_assert(alignof(Payload<Narrow, Pointer>) == alignof(void*));
static_assert(Payload<Pair, Narrow>::kHolds<Narrow>);
static_assert(!Payload<Pair, Narrow>::kHolds<Pointer>);
static_assert(std::is_trivially_copyable_v<Payload<Pair, Narrow>>);

TEST(PayloadTest, StartsZeroed) {
  const Payload<Pair, Narrow> payload;
  for (const std::byte b : payload.bytes()) {
    EXPECT_EQ(b, std::byte{0});
  }
}

TEST(PayloadTest, UpdateThenGetReturnsTheArm) {
  Payload<Pair, Narrow> payload;
  payload.Update<Pair>([](Pair& p) {
    p.first = 0x01020304;
    p.second = 5;
  });
  const Pair p = payload.Get<Pair>();
  EXPECT_EQ(p.first, 0x01020304);
  EXPECT_EQ(p.second, 5);
}

TEST(PayloadTest, UpdateKeepsTheBytesPastTheArm) {
  Payload<Pair, Narrow> payload;
  payload.Update<Pair>([](Pair& p) {
    p.first = -1;
    p.second = 7;
  });
  payload.Update<Narrow>([](Narrow& n) { n.value = 0; });
  // A union store of the narrow member would leave bytes 2..7 alone too.
  EXPECT_EQ(payload.bytes()[0], std::byte{0});
  EXPECT_EQ(payload.bytes()[1], std::byte{0});
  EXPECT_EQ(payload.bytes()[2], std::byte{0xFF});
  EXPECT_EQ(payload.bytes()[3], std::byte{0xFF});
  EXPECT_EQ(payload.Get<Pair>().second, 7);
}

TEST(PayloadTest, UpdateKeepsZeroPadding) {
  Payload<Pair, Narrow> payload;
  payload.Update<Pair>([](Pair& p) {
    p.first = 1;
    p.second = 1;
  });
  EXPECT_EQ(payload.bytes()[5], std::byte{0});
  EXPECT_EQ(payload.bytes()[6], std::byte{0});
  EXPECT_EQ(payload.bytes()[7], std::byte{0});
}

TEST(PayloadTest, SetCopiesAnArmFromAnotherPayload) {
  Payload<Pair, Narrow> from;
  from.Update<Pair>([](Pair& p) { p.first = 42; });
  Payload<Pair, Narrow> to;
  to.Set(from.Get<Pair>());
  EXPECT_EQ(to.Get<Pair>().first, 42);
}

}  // namespace
}  // namespace base
```

- [ ] **Step 2: Run it and see it fail**

Run: `cmake --build build --parallel $JOBS --target base_test` Expected: compile error,
`base/payload.h` not found.

- [ ] **Step 3: Write `src/base/payload.h`**

```cpp
// Typed storage for a tagged union whose bytes are part of a wire or file
// format.

#ifndef CNC_RED_ALERT_BASE_PAYLOAD_H_
#define CNC_RED_ALERT_BASE_PAYLOAD_H_

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <span>
#include <type_traits>
#include <utility>

#include "base/unaligned.h"

namespace base {

// The bytes of one of several trivially copyable "arms", laid out as a union
// of them: the same size and alignment, every arm at offset 0. An arm is never
// accessed in place. Get() copies it out and Update() copies it back, so
// reading bytes another arm wrote reinterprets them, with no undefined
// behavior. The selector (an event type, a packet command) lives outside, as
// it did beside the union.
//
// Writing an arm changes only its own sizeof(T) bytes. The rest keep what they
// held, as with a union member store. Update() takes the arm's padding from the
// storage and writes it back, so storage that starts zeroed stays
// deterministic byte for byte, which matters to data compared or sent as raw
// bytes.
//
// Example:
//   struct Move { int32_t who; int32_t where; };
//   struct Speed { int16_t value; };
//   base::Payload<Move, Speed> data;
//   data.Update<Move>([](Move& m) { m.who = 3; m.where = 9; });
//   const int32_t where = data.Get<Move>().where;
template <class... Arms>
  requires(sizeof...(Arms) > 0 && (std::is_trivially_copyable_v<Arms> && ...))
class Payload {
 public:
  // True for the types this payload can hold.
  template <class T>
  static constexpr bool kHolds = (std::is_same_v<T, Arms> || ...);

  // Returns a copy of arm T as the storage holds it.
  template <class T>
    requires kHolds<T>
  [[nodiscard]] T Get() const {
    return ReadUnaligned<T>(bytes_);
  }

  // Copies arm T out, lets `edit` change it, and copies it back.
  template <class T, std::invocable<T&> Edit>
    requires kHolds<T>
  void Update(Edit&& edit) {
    T arm = Get<T>();
    std::invoke(std::forward<Edit>(edit), arm);
    WriteUnaligned(std::span(bytes_), arm);
  }

  // Stores `arm`, padding bytes included; the bytes past sizeof(T) are
  // unchanged. Only for a value read with Get(): a freshly built T has
  // indeterminate padding. Use Update() otherwise.
  template <class T>
    requires kHolds<T>
  void Set(const T& arm) {
    WriteUnaligned(std::span(bytes_), arm);
  }

  [[nodiscard]] std::span<const std::byte> bytes() const { return bytes_; }
  [[nodiscard]] std::span<std::byte> bytes() { return bytes_; }

 private:
  alignas(Arms...) std::array<std::byte, std::max({sizeof(Arms)...})> bytes_{};
};

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_PAYLOAD_H_
```

- [ ] **Step 4: Run the test**

Run:
`cmake --build build --parallel $JOBS --target base_test && ctest --test-dir build -R base_test`
Expected: PASS. Then `tools/strict_tu.py src/base/payload_test.cc`, which must be clean.

- [ ] **Step 5: Commit** — `Add base::Payload, typed storage for a union's bytes`

### Task 2: Engine bit packing without unions

**Files:**

- Modify: `src/engine/codec/base64.cc:91-110` (delete `PacketType`), `:160-330` (encode/decode
  loops)
- Modify: `src/engine/crypto/blowfish_reference_test_util.cc:18-35` (delete `Int`), `:280-345`
- Test: `src/engine/codec/base64_test.cc` (new, in `engine_codec_test`);
  `blowfish_differential_test.cc` (existing)

The old layouts are little-endian bit-fields. `PacketType` is `Raw = C3 | C2<<8 | C1<<16`, and the
six-bit codes are `O4 = Raw & 63`, `O3 = Raw>>6`, `O2 = Raw>>12`, `O1 = Raw>>18`. In `Int`, `C0` is
the most significant byte of `Long`: `Long = C0<<24 | C1<<16 | C2<<8 | C3`.

- [ ] **Step 1: Pin Base64 with RFC 4648 vectors.** Create `src/engine/codec/base64_test.cc` with
      `Base64_Encode`/`Base64_Decode` round trips for `""`, `"M"`→`"TQ=="`, `"Ma"`→`"TWE="`,
      `"Man"`→`"TWFu"`, `"foobar"`→`"Zm9vYmFy"`, and all 256 byte values. Add it to
      `engine_codec_test`'s `SOURCES` in `src/engine/codec/CMakeLists.txt`. Run it against the union
      code: PASS.

```cpp
std::string Encode(std::string_view text) {
  std::string out(((text.size() + 2) / 3) * 4, '\0');
  const int n = Base64_Encode(std::as_bytes(std::span(text)),
                              std::as_writable_bytes(std::span(out)));
  out.resize(static_cast<std::size_t>(n));
  return out;
}

TEST(Base64Test, EncodesRfc4648Vectors) {
  EXPECT_EQ(Encode(""), "");
  EXPECT_EQ(Encode("M"), "TQ==");
  EXPECT_EQ(Encode("Ma"), "TWE=");
  EXPECT_EQ(Encode("Man"), "TWFu");
  EXPECT_EQ(Encode("foobar"), "Zm9vYmFy");
}
```

(Write the matching `Decode` helper and `DecodesRfc4648Vectors`, plus `RoundTripsEveryByte` over a
256-byte buffer 0..255.)

- [ ] **Step 2: Rewrite `base64.cc` with shifts.** In the encoder, replace `PacketType packet` with
      `uint32_t word = 0;`, then `word |= C1 << 16` (and so on), and set `O1 = (word >> 18) & 0x3F`,
      `O2 = (word >> 12) & 0x3F`, `O3 = (word >> 6) & 0x3F`, `O4 = word & 0x3F`. In the decoder,
      `word |= code << 18` (O1), `<< 12`, `<< 6`, `<< 0`, and the output bytes are `word >> 16`,
      `>> 8`, `& 0xFF`. Delete the union and `PacketChars`' comment that describes it.
- [ ] **Step 3: Rewrite the Blowfish reference with a `uint32_t`.** Load `left` and `right` as
      big-endian words from `plaintext`. The S-box indices are `(x >> 24) & 0xFF`,
      `(x >> 16) & 0xFF`, `(x >> 8) & 0xFF` and `x & 0xFF` for `C0..C3`. Store `right`, then `left`,
      big-endian. Keep a small local lambda `Byte(uint32_t x, int i)` so the round lines stay
      readable.
- [ ] **Step 4: Run** `ctest --test-dir build -R 'engine_codec_test|engine_crypto_test'`: PASS (the
      Blowfish differential test compares the reference to the Rust kernel). Then
      `tools/strict_tu.py src/engine/codec/base64.cc src/engine/codec/base64_test.cc src/engine/crypto/blowfish_reference_test_util.cc`.
- [ ] **Step 5: Commit** — `Pack Base64 and the Blowfish reference words with shifts, not unions`

### Task 3: Team mission argument and the sync-bug trap pointer

**Files:**

- Modify: `src/ra/ai/team_type.h:76-125`, `src/ra/ai/team_type.cc:115-145`
- Modify: `src/ra/net/session.h:443-456,741`, `src/ra/net/session.cc:194,1050-1175`
- Modify: `src/td/startup/conquer.cc:205-221,2500-2640`
- Test: `src/ra/ai/team_type_test.cc` (existing, `ra_team_type_test`)

**Interfaces:**

- Produces: `TeamMissionType` keeps its accessors (`value()`, `quarry()`, `formation()`,
  `unit_mission()` and setters) over a private `int argument_ = 0;`. RA's `Session::TrapObject` and
  TD's `trap_object` become `ObjectClass*`; `TrapObjectType` is deleted in both games.

- [ ] **Step 1: Pin the argument.** In `team_type_test.cc`, add
      `TEST_F(TeamTypeTest, MissionArgumentReadsBackAsEveryType)`: set `set_quarry(QUARRY_ANYTHING)`
      and check `value() == static_cast<int>(QUARRY_ANYTHING)`; then `set_value(3)` and check
      `formation() == static_cast<FormationType>(3)`. Run it: PASS on the union.
- [ ] **Step 2: Replace the union** with `int argument_ = 0;`. The getters become
      `static_cast<E>(argument_)` and the setters `argument_ = static_cast<int>(e)`, and the
      constructor initializes `argument_(value)`. `Serialize` becomes `ar(mission_, argument_)`.
      `MissionType`, `QuarryType` and `FormationType` are `int`-based `enum class`es, so every byte
      is the same. Rewrite the six `argument_.x` reads in `team_type.cc` through the accessors.
- [ ] **Step 3: Replace the trap unions.** RA: delete `TrapObjectType` and declare
      `ObjectClass* TrapObject = nullptr;  // object being trapped, for the debugger; nullptr if none`.
      Every `TrapObject.Ptr.X = p` becomes `TrapObject = p`, and `TrapObject.Ptr.All = nullptr`
      becomes `TrapObject = nullptr`. Include the object headers the derived-to-base conversions
      need, with `// IWYU pragma: keep` where `misc-include-cleaner` misses them (CLAUDE.md
      Includes). TD: do the same for the file-static `trap_object` in `conquer.cc`.
- [ ] **Step 4: Run** `ctest --test-dir build -R 'ra_team_type_test|ra_save_fixture_test'` and
      `tools/strict_tu.py src/ra/ai/team_type.cc src/ra/net/session.cc src/td/startup/conquer.cc`.
- [ ] **Step 5: Commit** —
      `Store the team mission argument as int and the trap object as ObjectClass*`

### Task 4: Trigger event data

**Files:**

- Modify: `src/ra/ai/tevent.h:150-175`, `src/ra/ai/tevent.cc`, `src/ra/ai/trigger_type.cc`,
  `src/ra/ai/scenario.cc`, `src/ra/editor/trigger_type_dialog.cc`, `src/ra/saveload/ioobj.cc:181`
- Test: `src/ra/ai/trigger_type_test.cc`, `src/ra/editor/trigger_type_dialog_test.cc` (existing)

**Interfaces:**

- Produces: `TEventClass` has `private: int32_t data_ = 0;` and the accessors `structure()`,
  `unit()`, `infantry()`, `aircraft()`, `house()` and `value()`, each with a `set_` partner.

- [ ] **Step 1: Pin the house byte.** In `trigger_type_test.cc`, add
      `TEST_F(TriggerTypeTest, AHouseEventReplacesOnlyTheLowByte)`. It sets
      `event.Data.Value = 0x12345600`, then `event.Data.House = HOUSE_USSR`, and expects
      `event.Data.Value == 0x12345600 | static_cast<uint8_t>(HOUSE_USSR)`. Add a second case with
      `HOUSE_NONE` (-1): `0x123456FF`. Run both against the union: PASS. (They are rewritten in Step
      3 to use the accessors and must still pass.)
- [ ] **Step 2: Replace the union** with `int32_t data_ = 0;` and the accessors:

```cpp
  // The event's argument. Which accessor applies depends on Event; all of them
  // share one saved word, as the union they replace did.
  [[nodiscard]] StructType structure() const { return static_cast<StructType>(data_); }
  void set_structure(StructType s) { data_ = static_cast<int32_t>(s); }
  // ... unit(), infantry(), aircraft() and value() alike ...
  // HousesType is one byte; storing one keeps the other three, as the union
  // store did, so a saved [Trigs] value does not change.
  [[nodiscard]] HousesType house() const { return static_cast<HousesType>(static_cast<int8_t>(data_ & 0xFF)); }
  void set_house(HousesType h) {
    data_ = static_cast<int32_t>((static_cast<uint32_t>(data_) & ~0xFFU) |
                                 static_cast<uint8_t>(h));
  }
```

The constructors drop their `Data.Value = 0` bodies, since the initializer covers them. `Serialize`
(`ioobj.cc:181`) writes `data_`.

- [ ] **Step 3: Rewrite the 74 sites** in the files listed above: `x.Data.House` → `x.house()`, and
      `x.Data.House = h` → `x.set_house(h)`. Do the same for each arm.
- [ ] **Step 4: Run**
      `ctest --test-dir build -R 'ra_trigger_type_test|ra_trigger_type_dialog_test|ra_save_fixture_test|ra_saveglobals'`
      and `tools/strict_tu.py` on every changed `.cc`.
- [ ] **Step 5: Commit** — `Give TEventClass typed accessors over one saved word`

### Task 5: Cell occupancy flags

**Files:**

- Create: `src/game/sim/occupy_flags.h`, `src/game/sim/occupy_flags_test.cc` (add to `game_sim_test`
  in `src/game/sim/CMakeLists.txt`)
- Modify: `src/ra/objects/cell.h:203-222`, `src/td/objects/cell.h:170-188`, and the sites in
  `{ra,td}/objects/{cell,infantry,unit,drive,vessel,terrain}.cc`, `{ra,td}/saveload/iomap.cc`,
  `td/saveload/cellvalues.cc`, `td/startup/init.cc:1632`, and the tests
  `td/saveload/{savevalues_test,archive_roundtrip_test}.cc` and `ra/saveload/saveglobals_test.cc`

**Interfaces:**

- Produces:
  `enum class game::Occupant : uint8_t { kCenter, kNorthWest, kNorthEast, kSouthWest, kSouthEast, kVehicle, kMonolith, kBuilding }`
  (enumerator = bit index; the first five are the infantry spots, in the `spot_index` order).
  `class game::OccupyFlags` has `Has(Occupant)`, `Set(Occupant, bool)`, `AnySpot()` (any of the five
  infantry spots), `any()`, `bits()`, `static FromBits(uint8_t)`, `With(Occupant) const` (a copy
  with that bit set, for tests and initializers) and `Serialize(ar)` writing one `uint8_t`.

- [ ] **Step 1: Write the test**

```cpp
TEST(OccupyFlagsTest, EachOccupantIsItsOwnBitFromTheLowEnd) {
  // The order of the bit-fields this replaced, which the saves store.
  EXPECT_EQ(OccupyFlags().With(Occupant::kCenter).bits(), 0x01);
  EXPECT_EQ(OccupyFlags().With(Occupant::kSouthEast).bits(), 0x10);
  EXPECT_EQ(OccupyFlags().With(Occupant::kVehicle).bits(), 0x20);
  EXPECT_EQ(OccupyFlags().With(Occupant::kBuilding).bits(), 0x80);
}

TEST(OccupyFlagsTest, AnySpotSeesOnlyTheInfantrySpots) {
  EXPECT_FALSE(OccupyFlags::FromBits(0xE0).AnySpot());
  EXPECT_TRUE(OccupyFlags::FromBits(0x04).AnySpot());
}

TEST(OccupyFlagsTest, SetFalseClearsOnlyThatBit) {
  OccupyFlags flags = OccupyFlags::FromBits(0xFF);
  flags.Set(Occupant::kMonolith, false);
  EXPECT_EQ(flags.bits(), 0xBF);
}
```

(`With()` is a `constexpr` copy-and-set helper for tests and initializers.)

- [ ] **Step 2: Write `occupy_flags.h`**. `uint8_t bits_ = 0;` with `Has(o)` →
      `(bits_ >> static_cast<int>(o)) & 1`, `Set` using `base::Bit<uint8_t>`, `AnySpot()` →
      `(bits_ & 0x1F) != 0`, and
      `template <class Archive> void Serialize(Archive& ar) { ar(bits_); }`. Run the test: PASS.
- [ ] **Step 3: Swap the member** in both `cell.h` to `game::OccupyFlags Flag;` (the member name
      stays, to limit churn). Then rewrite the sites. `Flag.Occupy.Vehicle = true` becomes
      `Flag.Set(game::Occupant::kVehicle, true)`, and `Flag.Composite & 0x1F` becomes
      `Flag.AnySpot()`. `Flag.Composite` as a boolean becomes `Flag.any()`.
      `Flag.Composite |= base::Bit<uint8_t>(spot_index)` becomes
      `Flag.Set(static_cast<game::Occupant>(spot_index), true)`. The `ra/objects/cell.cc:2622` bit
      loop reads `Flag.bits()`. In both `iomap.cc`, `ar(Flag.Composite, Land)` becomes
      `ar(Flag, Land)`. Leave the commented-out editor lines alone.
- [ ] **Step 4: Run**
      `ctest --test-dir build -R 'game_sim_test|save_fixture|archive_roundtrip|saveglobals|savevalues'`,
      both smoke scripts (`tools/ra_saveload_smoke.sh build/src/ra/rasdl`,
      `tools/td_saveload_smoke.sh build/src/td/tdsdl`) and `tools/strict_tu.py` on the changed
      files.
- [ ] **Step 5: Commit** — `Replace the cell occupancy union with game::OccupyFlags`

### Task 6: Pin the wire bytes (before Part B touches them)

**Files:**

- Create: `src/ra/net/wire_format_test.cc`, `src/td/net/wire_format_test.cc`
- Modify: `src/ra/CMakeLists.txt` (new target `ra_wire_format_test`, registered like
  `ra_team_type_test`: `SOURCES net/wire_format_test.cc ${RA_GAME_TEST_UTIL} startup/startup.cc`,
  `LIBS ra_engine absl::core_headers`, `RA_NO_ENTRY_POINT=1`, `apply_dotenv_settings`,
  `apply_language_setting`); `src/td/CMakeLists.txt` likewise with `${TD_GAME_TEST_UTIL}` and
  `TD_NO_ENTRY_POINT=1`
- Modify: `src/td/core/defines.h` (add `static_assert(sizeof(GlobalPacketType) == N)` with today's
  size), `src/ra/net/event.h` (add `static_assert(alignof(EventClass) == N)`)

**Interfaces:**

- Produces: golden byte strings for one event per constructor and one packet per arm. Part B must
  keep them passing unchanged.

- [ ] **Step 1: Write the RA test with empty expectations.** A `GameFixture` subclass sets
      `ThePlayer() = base::NewPooled<HouseClass>(HOUSE_GREECE);` (include `ra/objects/house.h`,
      `base/pooled.h`). Each test builds one event through a constructor, sets
      `event.ID = 0; event.Frame = 0;`, and compares `Hex(base::ObjectBytes(event))` to a literal.

```cpp
std::string Hex(std::span<const std::byte> bytes) {
  std::string out;
  for (const std::byte b : bytes) {
    absl::StrAppendFormat(&out, "%02x", std::to_integer<int>(b));
  }
  return out;
}

TEST_F(WireFormatTest, MegaMissionEvent) {
  EventClass event(Target::FromPacked(0x01000005), MISSION_ATTACK,
                   Target::FromPacked(0x02000007), Target::FromPacked(0x03000009));
  event.ID = 0;
  event.Frame = 0;
  EXPECT_EQ(Hex(base::ObjectBytes(event)), "");  // filled in by Step 2
}
```

Cover every constructor in `ra/net/event.h:259-284`, using distinct non-zero values in every field
(e.g. `AnimType` 3, `HOUSE_USSR`, a coordinate `0x00120034`). The `(void*, uint32_t)` constructor
puts a pointer in the bytes, so compare only `EventLength.at(ADDPLAYER)` bytes of its payload. Also
add one case per `GlobalPacketType` and `SerialPacketType` arm. Zero-fill the packet, set `Command`,
and fill every field of the arm with a distinct value (strings via `base::SafeCopy`, every bit-field
set to 1). Finally, add a `MEGAMISSION` event whose arm is then overwritten through `Data.Mission`,
to pin the "second arm keeps the first arm's tail" rule.

- [ ] **Step 2: Record the bytes.** Run
      `ctest --test-dir build -R ra_wire_format_test --output-on-failure`. Each failure prints the
      actual hex. Paste each one into its literal, then rerun: PASS.
- [ ] **Step 3: Do the same for TD** (`td/net/event.h:217-236` constructors, TD `GlobalPacketType`
      arms, `ThePlayer() = base::NewPooled<HouseClass>(HOUSE_GOOD)`).
- [ ] **Step 4: Add the two `static_assert`s**, then build.
- [ ] **Step 5: Commit** — `Pin the event and packet bytes before replacing their unions`

### Task 7: Split the network list nodes

**Files:**

- Modify: `src/ra/net/session.h:282-306,717-719,755-756`, `src/ra/saveload/saveglobals.cc:112-123`,
  and every user in `ra/net/{netdlg,nulldlg,session,local_net_game}.cc`, `ra/wol/wol_gsup.cc`,
  `ra/editor/mapeddlg.cc`, `ra/screens/*`, `ra/saveload/saveglobals_test.cc`
- Modify: `src/td/net/nodename.h`, `src/td/net/network.h:149-155,278-279`, `td/net/netdlg.cc`,
  `td/net/nulldlg.cc`

**Interfaces:**

- Produces (RA):

```cpp
// A name and an address on the network: what every list node has.
struct NodeName {
  char Name[MPLAYER_NAME_MAX]{};
  engine::net::IPXAddressClass Address;
};

// A game announced on the network.
struct GameNode : NodeName {
  unsigned char IsOpen = 0;  // is the game open?
  int64_t LastTime = 0;      // last time we heard from it
};

// A player in this game. The only node a save holds.
struct PlayerNode : NodeName {
  template <class Archive>
  void Serialize(Archive& ar);

  HousesType House{};       // "ActLike" house
  PlayerColorType Color{};
  HousesType ID{};          // actual house
  int ProcessTime = 0;      // length of time to process the player's main loop
};

// A player in the chat lobby.
struct ChatNode : NodeName {
  int64_t LastTime = 0;
  unsigned char LastChance = 0;  // about to be removed from the list
  PlayerColorType Color{};
};
```

`Session::Games`, `Players` and `Chat` become `OwnedVector<GameNode>`, `<PlayerNode>` and
`<ChatNode>`. `NodeNameType` and `NodeNameTag` are deleted. TD: `NodeName`, `GameNode` (`Version`,
`IsOpen`, `LastTime`) and `PlayerNode` (`House`, `Color`), with `Network::games()` and `players()`
retyped.

- [ ] **Step 1: Replace the types** and let the compiler list the sites. `who->Player.House` becomes
      `who->House`, `who->Game.IsOpen` becomes `who->IsOpen`, and so on. Each
      `make_unique<NodeNameType>()` becomes the node type of the list it is added to. A function
      that takes `NodeNameType*` for nodes of more than one list takes `const NodeName&` if it only
      needs the name and address; otherwise give it one overload per node type.
- [ ] **Step 2: Move `Serialize`** to `PlayerNode` in `saveglobals.cc`, writing
      `ar(Name, Address, House, Color, ID, ProcessTime)` in the same order, with the same
      reset-on-read (`*this = {}` on the `PlayerNode` part, keeping `Name` and `Address` as the
      archive reads them).
- [ ] **Step 3: Run** `ctest --test-dir build -R 'ra_save_fixture_test|saveglobals|ra_editor'`,
      `tools/ra_saveload_smoke.sh build/src/ra/rasdl`, `tools/net_check.py`, and
      `tools/strict_tu.py` on the changed files.
- [ ] **Step 4: Commit** — `Split the network list node into game, player and chat nodes`

## Part B: The wire payloads

### Task 8: RA `EventClass` payload

**Files:**

- Modify: `src/ra/net/event.h:144-245` (union → arms + `base::Payload`), `src/ra/net/event.cc`
  (constructors, `EventLength`, `Execute`, `variable_bytes`, `Allocate_Variable`, `Free_Variable`),
  `src/ra/net/queue.cc`, and the other users the compiler finds (`ra/map/display.cc`,
  `ra/objects/*.cc`, `ra/startup/conquer.cc`, ...)
- Test: `src/ra/net/wire_format_test.cc` (Task 6), plus a new `EventArmsTest` in the same file

**Interfaces:**

- Consumes: `base::Payload` (Task 1).
- Produces: nested arm structs `EventClass::OptionsData`, `SellCellData`, `TargetData`, `AnimData`,
  `GeneralData`, `MegaMissionData`, `MegaMissionFormationData`, `MissionData`, `NavComData`,
  `TarComData`, `SpecificData`, `PlaceData`, `SpecialData`, `FrameInfoData`, `VariableData`,
  `TimingData`, `ProcessTimeData`, with the same fields as the union arms. Each has
  `static constexpr bool Carries(EventType type)`. Then
  `template <class T> [[nodiscard]] T data() const` and
  `template <class T, std::invocable<T&> Edit> void UpdateData(Edit&& edit)`, both of which
  `DCHECK(T::Carries(Type))`, and `template <class T> void SetData(const T& arm)` (for a value from
  another event's `data<T>()`). The payload is a private member `data_`, of type
  `base::Payload<...all arms...>`.

- [ ] **Step 1: Declare the arms** at the top of `EventClass` (after `EventType`), each with the
      fields of its old union arm. `Carries` lists the types from the `EventLength` table and from
      the constructors that write that arm:

```cpp
  struct MegaMissionData {
    static constexpr bool Carries(EventType type) { return type == MEGAMISSION; }
    ra::Target Whom;         // Whom to apply mission to.
    MissionType Mission;     // What mission to apply.
    ra::Target Target;       // Target to assign.
    ra::Target Destination;  // Destination to assign.
  };
```

To build each `Carries`, grep every write and read of the arm: the type in scope there is the one it
carries. Where a type reads two arms (`MEGAMISSION` then `Mission` in the overwrite case pinned in
Task 6), list it under both. Write `EventArmsTest.EveryTypeWithALengthCarriesAnArm`: for every
`EventType` with a non-zero `EventLength`, at least one arm's `Carries` is true.

- [ ] **Step 2: Replace the union** with `base::Payload<OptionsData, ..., ProcessTimeData> data_;`
      and the three member templates. `static_assert(sizeof(EventClass) == 32)` must still hold. The
      `EventLength` table becomes `sizeof(MegaMissionData)` and so on, with `sizeof(RTTIType)` for
      `SUSPEND`/`ABANDON`, `sizeof(FrameInfoData::Delay)` for `RESPONSE_TIME` (a one-byte prefix, as
      before), and `sizeof(VariableData::Size)` for `ADDPLAYER`.
- [ ] **Step 3: Rewrite the sites**, one function at a time:

```cpp
// Before
  Data.NavCom.Whom = src;
  Data.NavCom.Where = dest;
// After
  UpdateData<NavComData>([&](NavComData& d) {
    d.Whom = src;
    d.Where = dest;
  });

// Before
  if (event->Data.Variable.Size > cap) { ... event->Data.Variable.Size ... }
// After
  const uint32_t size = event->data<EventClass::VariableData>().Size;
  if (size > cap) { ... size ... }
```

Inside a `switch (Type)` in `Execute()`, read the arm once into a `const auto` at the top of each
`case`.

- [ ] **Step 4: Run** `ctest --test-dir build -R ra_wire_format_test` (golden bytes unchanged),
      `tools/net_check.py`, and `tools/strict_tu.py` on every changed `.cc`.
- [ ] **Step 5: Commit** — `Hold RA's event payload in base::Payload`

### Task 9: RA network packets

**Files:**

- Modify: `src/ra/net/session.h:310-440`, and the users:
  `ra/net/{netdlg,nulldlg,chat,session, sendfile,network,local_net_game}.cc`,
  `ra/wol/{wol_gsup,stats}.cc`, `ra/screens/{score,goptions}.cc`, `ra/editor/mapeddlg.cc`,
  `ra/saveload/saveload.cc`
- Test: `src/ra/net/wire_format_test.cc`

**Interfaces:**

- Consumes: `base::Payload`.
- Produces:
  `GlobalPacketType::{GameInfoData, PlayerInfoData, ScenarioInfoData, MessageData, ResponseTimeData, RejectData, ChatData}`
  and `SerialPacketType::{ScenarioInfoData, MessageData, ChatData}`, each with
  `static constexpr bool Carries(NetCommandType)` (or `SerialCommandType`). Each packet gets the
  same `data<T>()` / `UpdateData<T>()` / `SetData<T>()` trio as `EventClass`, over a private
  `data_`. `Command`, `Name` and `ID` stay public.

- [ ] **Step 1: Declare the arms and the trio.** As in Task 8, build each `Carries` by grepping the
      code that writes and reads the arm. Keep both size `static_assert`s.
- [ ] **Step 2: Rewrite the sites, one file per commit-sized chunk** (`netdlg.cc`, `nulldlg.cc`,
      `wol_gsup.cc`, then the rest). Senders set `Command` first, then fill the arm in one
      `UpdateData` lambda. Receivers copy the arm into a `const auto` once per command branch.

```cpp
// Before
  TheSession().GPacket.ScenarioInfo.Credits = Credits;
  TheSession().GPacket.ScenarioInfo.Seed = Seed;
// After
  TheSession().GPacket.UpdateData<GlobalPacketType::ScenarioInfoData>(
      [&](GlobalPacketType::ScenarioInfoData& info) {
        info.Credits = Credits;
        info.Seed = Seed;
      });
```

- [ ] **Step 3: Check Review Focus 1.** For each `char[]` arm field (`Scenario`, `ShortFileName`,
      `FileDigest`, `Message`, `Buf`) passed to a function, confirm the callee copies it rather than
      keeping the pointer. List each call and its callee in the commit message.
- [ ] **Step 4: Run** `ctest --test-dir build -R 'ra_wire_format_test|ra_save_fixture_test'`,
      `tools/net_check.py`, and `tools/strict_tu.py` on the changed files.
- [ ] **Step 5: Commit** (one per chunk) — `Hold RA's network packet payloads in base::Payload`

### Task 10: TD `EventClass` payload and `GlobalPacketType`

**Files:**

- Modify: `src/td/net/event.h:133-245`, `src/td/net/event.cc`, `src/td/net/queue.cc`,
  `src/td/core/defines.h:2355-2400`, `td/net/{netdlg,nulldlg}.cc`, `td/startup/conquer.cc`, and the
  other users the compiler finds
- Test: `src/td/net/wire_format_test.cc`, `src/td/net/queue_alignment_test.cc`

**Interfaces:** the same shape as Tasks 8 and 9, with TD's arms (no `MegaMission_F`, `SellCell` or
`Variable`; TD's `Special` has no `Where`), `td::Target`, and TD's `NetCommandType`.

- [ ] **Step 1: Events**: Task 8's Steps 1–3 for `td/net/event.h`. Keep
      `static_assert(sizeof(EventClass) == 24)`.
- [ ] **Step 2: Packets**: Task 9's Steps 1–3 for TD's `GlobalPacketType`.
- [ ] **Step 3: Run**
      `ctest --test-dir build -R 'td_wire_format_test|td_archive_roundtrip_test|td_save_fixture_test'`
      and `tools/strict_tu.py` on the changed files. No `net_check` exists for TD: start two
      `tdsdl -NETGAME` instances by hand, or record that this check is still open in the progress
      table.
- [ ] **Step 4: Commit** — `Hold TD's event and packet payloads in base::Payload`

## Part C: Enforce

### Task 11: Enable the check

**Files:**

- Modify: `.clang-tidy:79` (delete `-cppcoreguidelines-pro-type-union-access,`)
- Modify: `docs/CLANG_TIDY_PRIORITIES.md:132` (row → `Enabled`, naming this plan) and the union
  paragraph of "Buffer, union and boolean conversion policy" (superseded note, as the bounds
  paragraph has)
- Possibly modify: `CMakeLists.txt:239` (`-Wno-nested-anon-types`) and
  `CLANG_TIDY_PRIORITIES.md:252`

- [ ] **Step 1: Re-sweep** with the command from Findings over a freshly configured strict compile
      db. Expect 0 sites and no `clang-diagnostic-error` lines (grep `error:`, since
      `WarningsAsErrors` turns findings into errors).
- [ ] **Step 2: Measure `nested-anon-types`.** The 20 anonymous structs inside anonymous unions are
      gone with the unions. Count what is left by compiling every TU with its own flags plus
      `-Wnested-anon-types -fsyntax-only` (not `-Wno-everything -W...`, which under-reports). If the
      count is 0, drop `-Wno-nested-anon-types` and set that row to `Enabled` too.
- [ ] **Step 3: Edit `.clang-tidy` and the docs.**
- [ ] **Step 4: Full verification** (see below).
- [ ] **Step 5: Commit** — `Enable cppcoreguidelines-pro-type-union-access`

## Verification

Run after Task 7, after Task 10 and in Task 11:

- Full strict build: `cmake --build build-strict --parallel 14` (foreground; more jobs get
  OOM-killed).
- `ctest --test-dir build --output-on-failure -j $JOBS`.
- `ctest --test-dir build-debug --output-on-failure -j $JOBS` (Debug, so the `DCHECK`s in
  `data<T>()` run).
- `tools/net_check.py` (RA two-player determinism).
- `tools/ra_saveload_smoke.sh build/src/ra/rasdl` and
  `tools/td_saveload_smoke.sh build/src/td/tdsdl`.
- `tools/ui_check.py` (the network dialogs draw from the packet and node fields).

## Progress

| Task | State | Commit |
| ---- | ----- | ------ |
| 1    |       |        |
| 2    |       |        |
| 3    |       |        |
| 4    |       |        |
| 5    |       |        |
| 6    |       |        |
| 7    |       |        |
| 8    |       |        |
| 9    |       |        |
| 10   |       |        |
| 11   |       |        |
