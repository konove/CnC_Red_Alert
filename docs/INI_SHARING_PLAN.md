# Shared INI Database Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development
> (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use
> checkbox (`- [ ]`) syntax for tracking.

Written 2026-10-02 against `main` at 578e1d94.

**Status: complete (2026-10-03)**, 583feb2b..83c358c4, except the checks that need a display or a
second machine (see Outcome). Both games read and write every INI file through
`engine::file::IniFile`; `INIClass`, `CCINIClass`, the `WWGetPrivateProfile*` wrappers and the
`ReadProfile`/`WriteProfile` text scanner are gone. Headless frame logs of both games match the
pre-plan binary line for line, and no shipped INI file of either game reads differently. The task
text below is the plan as written; where execution departed from it, the Outcome section says how.

**Goal:** One INI database class, `engine::file::IniFile`, used by both games: Red Alert's
`INIClass` moves to the engine, `CCINIClass` dissolves into free functions, and Tiberian Dawn's
`WWGetPrivateProfile*` text scanning is replaced by it.

**Architecture:** `INIClass` is already generic apart from one RA container and two stray includes,
so it moves to `engine/file` first, unchanged in behaviour. Three semantic gaps between it and TD's
profile reader (case, line length, duplicates) are then closed in the engine class, each pinned by a
test, and a comparison tool proves on the shipped TD files that both readers return the same values.
Only then does TD migrate, readers before writers, one subsystem per commit. The game-typed
accessors never become shared: RA's become free functions over the public API, and TD keeps its
existing `X::From_Name(buf)` calls.

**Tech Stack:** C++23, GoogleTest, Abseil (`flat_hash_map`), the strict clang build
(`cmake-build-strict-ra-clang`), the GCC build (`build/`), `tools/strict_tu.py`,
`tools/td_saveload_smoke.sh`, `mixdump`.

**Spec:** the user's requests of 2026-10-02 ("can we roll all changes into INIClass?", "how about
sharing it with TD", "write the plan to docs"); this document is the spec.

## Global Constraints

- Every commit builds with GCC (`build/`, `--parallel 22`) and with the strict clang build
  (`--parallel 14`, foreground), and passes `ctest` in both.
- No `Co-Authored-By` or other trailers in commit messages.
- New files carry no EA header, use `CNC_RED_ALERT_<PATH>_H_` guards and Google-style comments.
- New and renamed identifiers follow the Google naming table in `CLAUDE.md`; `base::ssize` for
  counts, no `short`/`long`, no printf family.
- `engine/file` may include only `base`, `engine/platform`, `engine/stream`, `engine/codec` and
  `engine/crypto` (`tools/check_layers.py`, the `engine_layers_test` ctest).
- Save-game formats do not change. INI text written by either game stays loadable by the pre-plan
  binary.
- During a task's edit loop use `tools/strict_tu.py <files>`; run one full strict build before each
  commit.
- Format with `git clang-format`, never `clang-format -i`.

## Review Focus

1. **A TD value that differs only because of reader semantics.** A key spelled `[BASIC]` in a file
   and `"Basic"` in code, or a team line over 127 characters, must read the same after the
   migration. Pinned by Task 3's tests and Task 4's comparison over every shipped TD INI.
2. **`key=` with an empty value.** The profile reader finds the key and returns `""`; `INIClass`
   drops the entry at load and returns the caller's default. Task 4's tool reports every such key in
   the shipped files and its step 4 decides the engine behaviour from that report.
3. **A scenario CRC that changes.** TD hashes the raw scenario text into `scenario_crc()` for
   multiplayer sync. The text must still be hashed before parsing, byte for byte (Task 6, checked by
   the headless logic log).
4. **`CONQUER.INI` losing what the player wrote.** `IniFile::Save` re-emits sections and keys but
   drops comments and blank-value keys. A hand-edited `CONQUER.INI` must keep every key the game
   does not own (Task 8's round-trip test).
5. **A stale digest.** `Get_Unique_ID()` after a `Put_*` must reflect the edit (Task 2's test).

---

## Context

Red Alert reads INI files through two classes in `src/ra/data/`:

- `INIClass` (`ini.h`, `ini.cc`, 1,260 lines): a parsed database of sections and entries with
  `Get_/Put_` for strings, ints, hex, bools, `Fixed`, text blocks, UU-encoded blocks and `PKey`.
- `CCINIClass : INIClass` (`ccini.h`, `ccini.cc`, 1,607 lines): about 40 `Get_/Put_` methods for
  game enums and pointers (`UnitType`, `HousesType`, `TriggerTypeClass*`, ...) and a SHA-1 digest
  (`Load/Save(..., withdigest)`, `Get_Unique_ID()`), which multiplayer uses to compare `rules.ini`.

Tiberian Dawn has neither. It keeps the INI file as text and re-scans it on every lookup through
`WWGetPrivateProfileString/Int/Hex` and `WWWritePrivateProfileString/Int` (`td/data/profile.*`),
thin wrappers over `engine::file::ReadProfile`/`WriteProfile` (`engine/file/profile_buffer.*`).

## Findings

### What ties `INIClass` to Red Alert

| Dependency                                 | Where                                   | Resolution                                                                                                                                                      |
| ------------------------------------------ | --------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `IndexClass<T>` (`ra/core/search.h`)       | `SectionIndex`, `EntryIndex`            | Replace with `absl::flat_hash_map`; `search.h` then has no user but its own test and is deleted.                                                                |
| `ra/core/defines.h`, `ra/objects/object.h` | `ini.h` includes                        | Needed only by `Scan_Place_Object(ObjectClass*, Cell)`, declared at `ini.h:204` and defined in `ra/ai/scenario.cc`. Move the declaration to `ra/ai/scenario.h`. |
| `strtrim`, `Read_Line`                     | `engine/stream/readline.h`              | Already engine.                                                                                                                                                 |
| `Fixed`, `PKey`, base64, `CrcEngine`       | `base`, `engine/crypto`, `engine/codec` | Already below `engine/file`.                                                                                                                                    |

`data/ini.cc` is compiled on its own into `ra_ini_number_test` and `ra_assets_test`
(`src/ra/CMakeLists.txt:92`, `:141`); `src/tools/game_data.cc` relies on `Get_PKey(true)`.

### Users

| Code                                                | Count                                                                          |
| --------------------------------------------------- | ------------------------------------------------------------------------------ |
| RA: plain `INIClass` objects and parameters         | about 30, in 10 files                                                          |
| RA: `CCINIClass` references outside `ccini.*`       | 107, mostly `Read_INI(CCINIClass&)` signatures                                 |
| RA: typed accessor calls outside `ccini.cc`         | 56                                                                             |
| RA: generic `Get_/Put_` method calls                | about 530                                                                      |
| RA: `WWGetPrivateProfile*` (live)                   | 2, both in `screens/expand.cc` (the 20 in `wol/wol_gsup.cc` are commented out) |
| TD: `WWGetPrivateProfile*`/`WWWritePrivateProfile*` | about 260 in 25 files                                                          |

TD call sites by file: `net/mplayer.cc` 48, `data/options.cc` 40, `data/ini.cc` 22,
`net/internet.cc` 21, `map/display.cc` 16, `objects/house.cc` 10, `startup/startup.cc` 8,
`ai/teamtype.cc` 7, `ai/prebuilt_base.cc` 6, `editor/mapeddlg.cc` 6, five each in
`objects/{unit,terrain,template,smudge,overlay,infantry,building,aircraft}.cc`, `ai/trigger.cc` 4,
`screens/expand.cc` 2, `editor/mapedit.cc` 2, `net/stats.cc` 1, plus `data/profile.cc` itself and
`data/number_parse_test.cc`.

`ra/data/profile.*` also holds `Read_Bin_*`/`Write_Bin_*` (28 callers); those stay.

### Semantic differences between the two readers

| Behaviour                | `INIClass`                                                      | `ReadProfile` (TD)                                                                 |
| ------------------------ | --------------------------------------------------------------- | ---------------------------------------------------------------------------------- |
| Name comparison          | Case-sensitive (lookup by CRC of the name as written)           | Case-insensitive                                                                   |
| Line length              | `kMaxLineLength = 128`; `Read_Line` silently drops the rest     | Unlimited; TD reads team lines into `char buf[500]` (`ai/teamtype.cc:175`, `:430`) |
| Duplicate section or key | Both stored; lookup returns whichever the sorted index hits     | First in the file                                                                  |
| `key=` (empty value)     | Entry dropped at load; getter returns the default               | Not measured; Task 4 reports it                                                    |
| Section with no entries  | Dropped at load                                                 | Present                                                                            |
| Enumerating a section    | `Entry_Count(section)` + `Get_Entry(section, i)`                | Null key returns NUL-separated key names (27 TD call sites)                        |
| Comments                 | Stripped at load (`;` to end of line), not written back         | Preserved in the text                                                              |
| Writing                  | Re-emits `[section]`, `key=value`, CRLF, blank line per section | Edits the text in place                                                            |

The first three are closed in the engine class (Task 3). The rest are handled at the call sites or
accepted (see Decisions).

### Digest

- `CCINIClass::Load` (`ccini.cc:186`) returns `true` when the stored digest does not match. The
  original returned a distinct code (2) that `Read_Scenario_INI` used to reject tampered official
  scenarios while exempting multiplayer maps 1-24. The comment at `ra/ai/scenario.cc:2044` claims a
  mismatch is a plain failure; the code accepts it. So `withdigest` verification does nothing today.
- The cached digest is invalidated only in `Load`. `Put_*` are non-virtual `INIClass` methods and
  leave it stale.

### TD specifics

- `Read_Scenario_Ini` (`td/data/ini.cc:344`) hashes every byte of the scenario text into
  `TheWorld().scenario_crc()` before reading from it.
- Every `Read_INI(char* buffer)` / `Write_INI(std::string& buffer)` in TD (13 classes, plus
  `DisplayClass` and `MapEditClass` virtuals) passes the raw text along.
- TD reads enums as `WWGetPrivateProfileString(..., buf, buffer); X::From_Name(buf)`. It has no
  typed-accessor layer and does not need one.

## Decisions

1. **Name and place:** `engine::file::IniFile` in `src/engine/file/ini_file.{h,cc}`, test
   `ini_file_test.cc` in `engine_file_test`.
2. **Typed accessors stay per game.** RA's become free functions in `ra/data/ini_types.{h,cc}`
   (`GetUnitType(const IniFile&, section, entry, default)`); TD keeps `From_Name` at the call site.
   No `game/` layer for them: the enums differ per game.
3. **Lookups become ASCII case-insensitive, first definition wins.** This matches TD's reader and
   makes RA's duplicate handling deterministic. The stored spelling is kept, so saved text and the
   digest do not change.
4. **No line-length limit.** Lines are read into `std::string`. RA values longer than 127 characters
   were truncated before; none are expected in shipped data and Task 3 measures it.
5. **Digest moves into `IniFile`**, invalidated by every mutation. `Load` reports a mismatch through
   a three-valued result; callers keep accepting a mismatch (today's behaviour), and the scenario
   comment is corrected. Rejecting tampered official scenarios again is a separate decision, listed
   under Out of scope.
6. **Written files lose comments.** `CONQUER.INI` and editor-saved scenarios are re-emitted, as RA
   already does with `REDALERT.INI`. Keys the game does not know are kept.
7. **Method names are modernized once, before TD migrates,** so TD's 260 new call sites are written
   against the final names.

## File Structure

| File                                                                                                             | Responsibility                                                              | Task    |
| ---------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------- | ------- |
| `src/engine/file/ini_file.h`, `.cc`                                                                              | The INI database: parse, query, edit, save, digest                          | 1-3, 5  |
| `src/engine/file/ini_file_test.cc`                                                                               | Its tests (absorbs `ra/data/ini_number_test.cc`)                            | 1-3     |
| `src/ra/data/ini_types.h`, `.cc`                                                                                 | RA's game-typed INI accessors and the `*_From_Name` helpers (was `ccini.*`) | 2       |
| `src/ra/ai/scenario.h`                                                                                           | Gains `Scan_Place_Object`                                                   | 1       |
| `src/tools/ini_compare.cc`                                                                                       | Compares `IniFile` with `ReadProfile` on loose INI files                    | 4       |
| `src/td/data/ini.cc` and TD `Read_INI`s                                                                          | Take `const IniFile&`                                                       | 6-7     |
| `src/td/data/options.cc`, `net/*.cc`, `startup/startup.cc`, `editor/*.cc`                                        | Read and write through `IniFile`                                            | 8       |
| Deleted: `src/ra/data/ini.*`, `ccini.*`, `ra/core/search.*`, `td/data/profile.*`, `engine/file/profile_buffer.*` |                                                                             | 1, 2, 9 |

---

### Task 1: Move `INIClass` to `engine/file` as `engine::file::IniFile`

Behaviour does not change in this task. Method names stay legacy; only the class name, namespace,
location and index container change.

**Files:**

- Create: `src/engine/file/ini_file.h`, `src/engine/file/ini_file.cc` (from `src/ra/data/ini.*` with
  `git mv`)
- Create: `src/engine/file/ini_file_test.cc` (from `src/ra/data/ini_number_test.cc` with `git mv`)
- Modify: `src/engine/file/CMakeLists.txt` (test list, `absl::flat_hash_map`),
  `src/ra/CMakeLists.txt:89-94` and `:139-143`, `src/ra/ai/scenario.h`, `src/ra/data/ccini.h`, every
  RA file naming `INIClass` or including `ra/data/ini.h`, `src/tools/game_data.cc` (comment)
- Delete: `src/ra/core/search.h`, `src/ra/core/search_test.cc` and its `add_gtest` entry

**Interfaces:**

- Produces: `engine::file::IniFile` in `engine/file/ini_file.h` with the public interface of today's
  `INIClass` (`Load`, `Save`, `Clear`, `Get_String`, `Get_Int`, `Get_Hex`, `Get_Bool`, `Get_Fixed`,
  `Get_TextBlock`, `Get_UUBlock`, `Get_PKey`, the matching `Put_*`, `Entry_Count`, `Get_Entry`,
  `Section_Count`, `Section_Present`, `Is_Present`, `Is_Loaded`, `Line_Count`, `Size`).

- [ ] **Step 1: Move `Scan_Place_Object` out of `ini.h`.** Delete line 204 of `src/ra/data/ini.h`
      and add to `src/ra/ai/scenario.h`, in namespace `ra`:

  ```cpp
  // Places `obj` on `cell` or the nearest free cell around it. Returns whether
  // it was placed.
  bool Scan_Place_Object(ObjectClass* obj, Cell cell);
  ```

  Build `rasdl`; add `#include "ra/ai/scenario.h"` to any TU that now fails. Then remove
  `ra/core/defines.h` and `ra/objects/object.h` from `ini.h` and fix what `misc-include-cleaner`
  reports in the includers.

- [ ] **Step 2: Write a failing test for the index replacement.** Add to
      `src/ra/data/ini_number_test.cc`:

  ```cpp
  TEST(IniNumberTest, ReplacingAnEntryKeepsTheOthersReachable) {
    INIClass ini;
    for (int i = 0; i < 50; ++i) {
      ASSERT_TRUE(ini.Put_Int("S", absl::StrCat("K", i).c_str(), i));
    }
    ASSERT_TRUE(ini.Put_Int("S", "K7", 700));
    ASSERT_TRUE(ini.Put_String("S", "K8", nullptr));  // removes K8
    EXPECT_EQ(ini.Entry_Count("S"), 49);
    EXPECT_EQ(ini.Get_Int("S", "K7", -1), 700);
    EXPECT_EQ(ini.Get_Int("S", "K8", -1), -1);
    EXPECT_EQ(ini.Get_Int("S", "K49", -1), 49);
    ASSERT_TRUE(ini.Clear("S"));
    EXPECT_FALSE(ini.Section_Present("S"));
  }
  ```

  Run `cmake --build build --target ra_ini_number_test && ctest --test-dir build -R ra_ini_number`.
  Expected: PASS (it characterizes current behaviour; it must still pass after step 3).

- [ ] **Step 3: Replace `IndexClass` with `absl::flat_hash_map`.** In `ini.h`:

  ```cpp
  absl::flat_hash_map<uint32_t, INIEntry*> EntryIndex;     // by CRC of the name
  absl::flat_hash_map<uint32_t, INISection*> SectionIndex; // by CRC of the name
  ```

  In `ini.cc` replace `Add_Index(id, p)` with `try_emplace(id, p)` (first definition wins, which
  makes the duplicate case deterministic), `Remove_Index(id)` with `erase(id)`, `Is_Present(id)` +
  `Fetch_Index(id)` with one `find`, `Count()` with `SectionList.size()` /`EntryList.size()`, and
  drop the `static_cast<int>` on the CRCs (`Index_ID()` returns `uint32_t`). When an entry that is
  indexed is erased and a later duplicate exists, re-point the index at the next entry of that name
  in `EntryList`, or lookups would miss it.

  Run the step 2 test. Expected: PASS.

- [ ] **Step 4: Move the files.**

  ```bash
  git mv src/ra/data/ini.h src/engine/file/ini_file.h
  git mv src/ra/data/ini.cc src/engine/file/ini_file.cc
  git mv src/ra/data/ini_number_test.cc src/engine/file/ini_file_test.cc
  git rm src/ra/core/search.h src/ra/core/search_test.cc
  ```

  In the moved files: guard `CNC_RED_ALERT_ENGINE_FILE_INI_FILE_H_`, namespace `engine::file`, class
  `IniFile` (nested `IniEntry`, `IniSection`), no EA header change (it is original EA code), test
  suite `IniFileTest`. Add `ini_file_test.cc` to `engine_file_test` and `absl::flat_hash_map` to
  `engine_file`'s `PRIVATE` libraries. In `src/ra/CMakeLists.txt` drop
  `data/ini_number_test.cc data/ini.cc` from `ra_ini_number_test` (rename the target
  `ra_profile_buffer_test` if only `profile_buffer_test.cc` is left) and `data/ini.cc` from
  `ra_assets_test`; remove the `search_test` entry.

- [ ] **Step 5: Retarget RA.** Replace `#include "ra/data/ini.h"` with
      `#include "engine/file/ini_file.h"` and `INIClass` with `engine::file::IniFile` in
      `ra/data/ccini.h` (base class), `ra/data/options.{h,cc}`, `ra/data/options_test.cc`,
      `ra/net/session.cc`, `ra/net/nulldlg.cc`, `ra/ai/scenario.cc`, `ra/startup/init.cc`,
      `ra/startup/startup.cc`, `ra/assets/assets.cc`. Update the comments in
      `src/tools/game_data.cc:36` and `src/engine/codec/stream_golden_test.cc:204`.

  ```bash
  grep -rnE "\bINIClass\b|ra/data/ini\.h|ra/core/search\.h" src   # expect: only CCINIClass matches
  ```

- [ ] **Step 6: Verify.**

  ```bash
  cmake --build build --parallel 22 && ctest --test-dir build --output-on-failure
  cmake --build cmake-build-strict-ra-clang --parallel 14
  ctest --test-dir cmake-build-strict-ra-clang --output-on-failure   # includes engine_layers_test
  tools/ra_saveload_smoke.sh build/src/ra/rasdl
  ```

  Expected: all pass. Then the headless logic check from Verification (RA, SCU05EA) against the
  pre-plan binary: identical logs.

- [ ] **Step 7: Commit** — `Move Red Alert's INIClass to engine/file as IniFile`.

### Task 2: Dissolve `CCINIClass`

**Files:**

- Create: `src/ra/data/ini_types.h`, `src/ra/data/ini_types.cc` (`git mv` from `ccini.*`)
- Modify: `src/engine/file/ini_file.{h,cc}`, `ini_file_test.cc`, and the RA files naming
  `CCINIClass` (list: `grep -rlE "\bCCINIClass\b" src/ra`)

**Interfaces:**

- Consumes: `engine::file::IniFile` from Task 1.
- Produces, in `engine::file`:

  ```cpp
  enum class IniLoadResult { kFailed, kLoaded, kDigestMismatch };

  class IniFile {
   public:
    // Loads the database. With `verify_digest`, a [Digest] section is removed
    // and compared with the SHA-1 of the remaining data.
    IniLoadResult LoadVerified(ByteStream& file);
    IniLoadResult LoadVerified(ByteSource& file);
    // Saves with a [Digest] section holding the SHA-1 of the data.
    bool SaveWithDigest(ByteStream& file) const;
    bool SaveWithDigest(ByteSink& sink) const;
    // A CRC of the data's SHA-1; equal for equal databases.
    [[nodiscard]] int Get_Unique_ID() const;
  };
  ```

- Produces, in `ra` (`ra/data/ini_types.h`): for every method of today's `CCINIClass`, a free
  function with the INI as the first parameter, `const engine::file::IniFile&` for getters and
  `engine::file::IniFile&` for putters:

  ```cpp
  UnitType GetUnitType(const engine::file::IniFile& ini, const char* section,
                       const char* entry, UnitType defvalue);
  bool PutUnitType(engine::file::IniFile& ini, const char* section,
                   const char* entry, UnitType value);
  ```

  The full set: `Buildings`, `UnitType`, `AnimType`, `ArmorType`, `BulletType`, `HousesType`,
  `Lepton`, `MPHType`, `OverlayType`, `SourceType`, `TerrainType`, `TheaterType`, `ThemeType`,
  `TriggerType`, `VQType`, `VocType`, `WarheadType`, `WeaponType`, `Owners`, `CrateType` (`Get` +
  `Put` each). `Owner_From_Name`, `Crate_From_Name`, `Theater_From_Name`, `Source_From_Name` stay as
  they are.

- [ ] **Step 1: Write the failing digest tests** in `ini_file_test.cc`:

  ```cpp
  TEST(IniFileTest, UniqueIdFollowsEdits) {
    IniFile ini;
    ASSERT_TRUE(ini.Put_Int("S", "K", 1));
    const int before = ini.Get_Unique_ID();
    ASSERT_TRUE(ini.Put_Int("S", "K", 2));
    EXPECT_NE(ini.Get_Unique_ID(), before);
    ASSERT_TRUE(ini.Put_Int("S", "K", 1));
    EXPECT_EQ(ini.Get_Unique_ID(), before);
  }

  TEST(IniFileTest, DigestRoundTripsAndDetectsEdits) {
    IniFile ini;
    ASSERT_TRUE(ini.Put_Int("S", "K", 1));
    std::vector<std::byte> bytes;
    VectorSink sink(bytes);  // engine/stream/vector_sink.h
    ASSERT_TRUE(ini.SaveWithDigest(sink));
    EXPECT_FALSE(ini.Section_Present("Digest"));

    IniFile same;
    SpanSource source(bytes);
    EXPECT_EQ(same.LoadVerified(source), IniLoadResult::kLoaded);
    EXPECT_FALSE(same.Section_Present("Digest"));

    std::string text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    text.replace(text.find("K=1"), 3, "K=2");
    IniFile edited;
    SpanSource edited_source(std::as_bytes(std::span(text)));
    EXPECT_EQ(edited.LoadVerified(edited_source), IniLoadResult::kDigestMismatch);
    EXPECT_EQ(edited.Get_Int("S", "K", 0), 2);  // still loaded
  }
  ```

  (Use `base::ObjectBytes`/the project's byte-view helper in place of `reinterpret_cast` if
  `pro-type-reinterpret-cast` fires.) Run: FAIL, the functions do not exist.

- [ ] **Step 2: Move the digest into `IniFile`.** Move `Calculate_Message_Digest`,
      `Invalidate_Message_Digest`, `Get_Unique_ID`, `IsDigestPresent` and `Digest` from `ccini.*`.
      `LoadVerified` is today's `CCINIClass::Load(..., true)` returning `kDigestMismatch` where it
      returns `true` after a failed compare; `SaveWithDigest` is today's `Save(..., true)` made
      `const` by saving a copy of the digest section through a local sink instead of
      `Put_UUBlock`/`Clear("Digest")` on `*this`, or kept non-const if that needs more than a few
      lines. Call the invalidation from `Load`, `Clear`, `Put_String`, `Put_TextBlock` and
      `Put_UUBlock` (every other `Put_*` goes through `Put_String`). Run step 1's tests: PASS.

- [ ] **Step 3: Convert the typed accessors.** `git mv` `ccini.h`/`ccini.cc` to `ini_types.*`,
      delete the class, turn each method into the free function above (`Get_String(...)` inside a
      body becomes `ini.Get_String(...)`). Rewrite the 56 call sites: `ini.Get_UnitType(s, e, d)` →
      `GetUnitType(ini, s, e, d)`. Find them with the compiler: after deleting the class, every
      remaining call is an error on `IniFile`.

- [ ] **Step 4: Replace the type.** `CCINIClass` → `engine::file::IniFile` in the 107 references
      (signatures such as `static void Read_INI(CCINIClass& ini)`); `Load(x, false)` → `Load(x)`,
      `Save(x, false)` → `Save(x)`, `Load(x, true)` → `LoadVerified(x) != IniLoadResult::kFailed`,
      `Save(x, true)` → `SaveWithDigest(x)`. The `true` sites are `ra/ai/scenario.cc:2040`, `:2376`,
      `:2424` and `ra/saveload/saveload.cc:1090`. Replace the comment at `scenario.cc:2044` with:

  ```cpp
  // A digest mismatch is accepted: LoadVerified still loads the data. The
  // original rejected tampered official scenarios here and exempted
  // multiplayer maps 1-24.
  ```

- [ ] **Step 5: Verify** as Task 1 step 6, plus `grep -rnE "\bCCINIClass\b|data/ccini" src` → no
      matches.

- [ ] **Step 6: Commit** — two commits: `Move the INI digest into IniFile` (steps 1-2),
      `Replace CCINIClass with free typed accessors` (steps 3-4).

### Task 3: Close the semantic gaps in `IniFile`

**Files:** `src/engine/file/ini_file.{h,cc}`, `src/engine/file/ini_file_test.cc`

**Interfaces:**

- Produces: unchanged signatures; names compare without ASCII case, lines have no length limit, the
  first of duplicate sections or keys answers lookups. New accessor:

  ```cpp
  // Returns the name of section `index` in file order, or nullptr when out of
  // range.
  const char* Get_Section(int index) const;
  ```

- [ ] **Step 1: Write the failing tests.**

  ```cpp
  IniFile Parse(std::string_view text) {  // returned by pointer if IniFile stays immovable
    ...
  }

  TEST(IniFileTest, NamesCompareWithoutCase) {
    const auto ini = Parse("[BASIC]\r\nPlayer=GoodGuy\r\n");
    char buf[32];
    EXPECT_GT(ini->Get_String("Basic", "PLAYER", "", buf, sizeof(buf)), 0);
    EXPECT_STREQ(buf, "GoodGuy");
    EXPECT_TRUE(ini->Section_Present("basic"));
    EXPECT_STREQ(ini->Get_Section(0), "BASIC");  // stored spelling is kept
  }

  TEST(IniFileTest, LongLinesAreNotTruncated) {
    const std::string value(400, 'x');
    const auto ini = Parse("[TeamTypes]\nT1=" + value + "\n");
    std::array<char, 500> buf{};
    EXPECT_EQ(ini->Get_String("TeamTypes", "T1", "", buf, 500), 400);
  }

  TEST(IniFileTest, FirstDuplicateWins) {
    const auto ini = Parse("[S]\nK=1\nK=2\n[S]\nK=3\n");
    EXPECT_EQ(ini->Get_Int("S", "K", 0), 1);
  }

  TEST(IniFileTest, AcceptsBareLineFeeds) {
    const auto ini = Parse("[S]\nA=1\nB=2\n");
    EXPECT_EQ(ini->Entry_Count("S"), 2);
  }
  ```

  `Parse` builds a `SpanSource` over the text, calls `Load` on a `std::unique_ptr<IniFile>` and
  `CHECK`s the result. Run: the first three FAIL.

- [ ] **Step 2: Case-insensitive keys.** Key both maps by `CrcEngine::Compute` of
      `absl::AsciiStrToUpper(name)`; `Index_ID()` and the two `Find_*` are the only places that
      compute it. Run: test 1 PASS.

- [ ] **Step 3: Unlimited lines.** In `Load`, replace `char buffer[kMaxLineLength]` + `Read_Line`
      with a local `std::string line` filled by a file-local
      `ReadLine(ByteSource&, std::string&,     bool& eof)` (same rules as `Read_Line`: stop at LF,
      skip CR, trim). Split with `std::string_view::find('=')` and `absl::StripAsciiWhitespace`
      instead of writing NULs into the buffer. `Put_Int`/`Put_Hex` keep small local buffers. Delete
      `kMaxLineLength` if nothing else uses it (`Get_TextBlock`/`Put_TextBlock` use it for wrapping;
      keep it there, renamed `kTextBlockLineLength`). Run: test 2 PASS.

- [ ] **Step 4: First duplicate wins on load** is already true after Task 1 (`try_emplace`); a
      duplicate section's entries stay in `SectionList` so the saved text and digest are unchanged.
      Run: test 3 PASS.

- [ ] **Step 5: Measure RA.** With the pre-task and post-task `rasdl`, compare `rules.ini`'s
      identity and the logic log:

  ```bash
  mixdump "$RA_CD" RULES.INI "$SCRATCH/RULES.INI"
  awk 'length > 127' "$SCRATCH/RULES.INI" | wc -l      # expect 0
  ```

  and run the headless logic check (Verification) on SCU05EA: identical logs. If the `awk` count is
  not 0, list the lines in this document under Progress; they now read whole instead of truncated.

- [ ] **Step 6: Verify** as Task 1 step 6. **Commit** —
      `Compare INI names without case and lift     the line limit`.

### Task 4: Prove the two readers agree on TD's files

**Files:**

- Create: `src/tools/ini_compare.cc`
- Modify: `src/tools/CMakeLists.txt`

**Interfaces:**

- Consumes: `engine::file::IniFile` (Task 3), `engine::file::ReadProfile`, `ReadProfileText`.
- Produces: executable `ini_compare <file.ini>...`; exit status 0 when no differences are printed.

- [ ] **Step 1: Write the tool.** For each file: read the text once (`OpenDiskFile` +
      `ReadProfileText`), parse it into an `IniFile` from a `SpanSource`, then
  1. for every section `s` (`Get_Section(i)`) and every entry `k` (`Get_Entry(s, j)`): read `k`
     through `ReadProfile(text, s, k, "\x01", out)` with a 1,024-byte `out` and through
     `ini.Get_String(s, k, "\x01", ...)`; print `VALUE file [s] k: profile="..." ini="..."` when
     they differ;
  2. for every section `s`: enumerate keys with `ReadProfile(text, s, nullptr, nullptr, keys)` (64
     KB buffer), and print `MISSING file [s] k` for each key `IniFile` does not have (`Is_Present`),
     with the profile's value, and `ORDER file [s]` when the common keys come in a different order.

  ```cmake
  add_executable(ini_compare ini_compare.cc)
  target_link_libraries(ini_compare PRIVATE engine_file base absl::str_format)
  ```

  Print with `absl::PrintF`. Return 1 if any line was printed.

- [ ] **Step 2: Extract TD's INI files.** TD data is at
      `TD_CD="$HOME/.local/share/Steam/steamapps/common/Command & Conquer"`. MIX archives store name
      CRCs, not names, so extract by the known names: `SC[GB][0-9][0-9][EW][ABC].INI` and
      `SCM[0-9][0-9]EA.INI` (generate the candidate list with a shell loop;
      `mixdump "$TD_CD"     --list <names>` prints only those that exist), `MISSION.INI`, and the
      loose `CONQUER.INI`. If `mixdump` cannot open TD's archives, record that under Progress and
      extend `tool_game_data` to register TD's containers before continuing; do not skip the
      comparison.

- [ ] **Step 3: Run it** over every extracted file and over RA's `RULES.INI` and one RA scenario.
      Expected for TD: no `VALUE` or `ORDER` lines. `MISSING` lines are the `key=` empty-value and
      malformed-line cases.

- [ ] **Step 4: Decide from the output and record it here** under Progress, in a table
      `file / section / key / profile value`. Rule:
  - no `MISSING` lines: nothing to do;
  - `MISSING` lines whose profile value is empty: for each key, read the TD call site's default. If
    the default is not `""`/`nullptr` and the result is used, make `IniFile::Load` keep empty-valued
    entries (remove the `divider`-empty `continue`), add `TEST(IniFileTest, KeepsEmptyValues)`, and
    re-run the RA checks of Task 3 step 5;
  - any `VALUE` or `ORDER` line: stop and fix `IniFile` (with a test) before Task 6.

- [ ] **Step 5: Commit** — `Add ini_compare to check IniFile against the profile reader`.

### Task 5: Give `IniFile` Google-style names

Run `/modernize-file src/engine/file/ini_file.h` (dead code, comments, types, names, simplification,
IDE inspections; the file is already named and namespaced). The rename table:

| Old                                     | New                                                                                                                      |
| --------------------------------------- | ------------------------------------------------------------------------------------------------------------------------ |
| `Load`, `Save`, `Clear`                 | unchanged                                                                                                                |
| `LoadVerified`, `SaveWithDigest`        | unchanged                                                                                                                |
| `Get_String(sec, key, def, span, size)` | `GetString(sec, key, def, span)` (the span carries the size) and `std::string GetString(sec, key, std::string_view def)` |
| `Get_Int/Hex/Bool/Fixed`                | `GetInt/GetHex/GetBool/GetFixed`                                                                                         |
| `Put_String/Int/Hex/Bool/Fixed`         | `PutString/PutInt/PutHex/PutBool/PutFixed`                                                                               |
| `Get_TextBlock`, `Put_TextBlock`        | `GetTextBlock`, `PutTextBlock`                                                                                           |
| `Get_UUBlock`, `Put_UUBlock`            | `GetBase64Block`, `PutBase64Block`                                                                                       |
| `Get_PKey`, `Put_PKey`                  | `GetPublicKey`, `PutPublicKey`                                                                                           |
| `Entry_Count`, `Get_Entry`              | `EntryCount`, `EntryName`                                                                                                |
| `Section_Count`, `Get_Section`          | `SectionCount`, `SectionName`                                                                                            |
| `Section_Present`, `Is_Present`         | `HasSection`, `HasEntry` (two functions instead of the defaulted `entry`)                                                |
| `Is_Loaded`, `Line_Count`, `Size`       | `is_loaded`, `LineCount`, `SavedSize`                                                                                    |
| `Get_Unique_ID`                         | `UniqueId`                                                                                                               |
| `const char*` name parameters           | `std::string_view`                                                                                                       |

- [ ] **Step 1:** Run the skill; it commits per stage. The roughly 530 RA call sites change with the
      names. These names are shared with other classes (`Get_Int` is not, `Size` and `Clear` are):
      rename compiler-driven, per the project's shared-method-name procedure, not by `sed`.
- [ ] **Step 2: Verify** as Task 1 step 6.

### Task 6: TD scenario reading on `IniFile`

**Files:** `src/td/data/ini.cc` (`Read_Scenario_Ini`), `src/td/data/ini.h`, and the `Read_INI` of
`td/ai/teamtype`, `td/ai/trigger`, `td/ai/prebuilt_base`,
`td/objects/{house,unit,infantry, aircraft,building,terrain,template,overlay,smudge}`,
`td/map/display`, `td/editor/mapedit` (`.h` and `.cc` each).

**Interfaces:**

- Consumes: `engine::file::IniFile` with Task 5's names.
- Produces: `static void X::Read_INI(const engine::file::IniFile& ini)` for the 12 classes, and
  `virtual void DisplayClass::Read_INI(const engine::file::IniFile& ini)` with its `MapEditClass`
  override. `Write_INI(std::string&)` is untouched in this task.

- [ ] **Step 1: Parse once, keep the CRC.** In `Read_Scenario_Ini`, after the existing
      `ReadProfileFile(fname)` and the `scenario_crc()` loop over the raw text (leave both exactly
      as they are), add:

  ```cpp
  engine::file::IniFile scenario;
  SpanSource source(std::as_bytes(std::span(*ini)));
  if (!scenario.Load(source)) {
    return false;
  }
  ```

  and keep `char* buffer` alive until every reader below has moved.

- [ ] **Step 2: Convert the readers one class per commit-sized edit,** in the order they are called
      from `Read_Scenario_Ini`. The three patterns:

  ```cpp
  // Value:
  WWGetPrivateProfileString("Basic", "Theme", "No Theme", buf, buffer);
  // becomes
  scenario.GetString("Basic", "Theme", "No Theme", buf);

  // Integer:
  x = WWGetPrivateProfileInt("Basic", "BuildLevel", def, buffer);
  // becomes
  x = scenario.GetInt("Basic", "BuildLevel", def);

  // Enumeration (teamtype.cc:175 and the 26 like it): the key_storage vector,
  // key_cursor and the `while (*tbuffer != '\0')` loop become
  const int count = ini.EntryCount(INI_Name());
  for (int i = 0; i < count; ++i) {
    const char* entry = ini.EntryName(INI_Name(), i);
    ini.GetString(INI_Name(), entry, "", buf);
    ...
  }
  ```

  The destination buffer sizes stay (`char buf[500]` in `teamtype.cc`); passing
  `std::span(buf).first(sizeof(buf) - 1)` becomes the whole `buf`, since `GetString` terminates
  within the span. After each class: `tools/strict_tu.py src/td/<file>.cc` and
  `tools/td_saveload_smoke.sh build/src/td/tdsdl SCG01EA <fixture>` for the fixture that covers it
  (`--team` for team types and triggers, `--building`, `--mobile` for units/infantry/aircraft,
  `--map` for display/terrain/template/overlay/smudge, `--world` for houses).

- [ ] **Step 3: Convert the other readers of scenario text in `td/data/ini.cc`** (`MISSION.INI` at
      `:575`, the briefing lookup) the same way, and `td/screens/expand.cc` (2 calls reading
      `[Basic] Name` from each expansion scenario).

- [ ] **Step 4: Verify.** Both builds and ctest; every fixture of `tools/td_saveload_smoke.sh`
      (none, `--factory --team --world --building --mobile --map --globals`), each run 3 times; the
      TD headless logic check (Verification) on SCG01EA, SCB01EA and one late mission (SCG15EA):
      `fields` lines identical to the pre-plan binary, `mapstate` differences within the old-vs-old
      noise.

- [ ] **Step 5: Commit** — one commit per group: teams and triggers; houses and base; the object
      classes; the map; `Read_Scenario_Ini` itself with `MISSION.INI` and `expand.cc`.

### Task 7: TD scenario writing on `IniFile`

**Files:** the `Write_INI` of the same 13 classes and `MapEditClass`, `Write_Scenario_Ini` in
`td/data/ini.cc` (`:730-783`, including `MASTER.INI`), `td/editor/mapeddlg.cc` (`:3157`, `:3470`),
`td/editor/mapedit.cc`.

**Interfaces:**

- Produces: `static void X::Write_INI(engine::file::IniFile& ini)` (`bool refresh` kept where it
  exists).

- [ ] **Step 1: Write a round-trip test first** (`src/td/data/scenario_ini_test.cc`, added to the
      `td_archive_roundtrip_test` sources in `src/td/CMakeLists.txt:63`, which links `td_engine`):
      load a small scenario text into an `IniFile`, `Save` it, load the result, and compare every
      section and entry. This pins that a scenario written by the editor is read back identically:

  ```cpp
  TEST(ScenarioIniTest, SavedScenarioReadsBackIdentically) {
    const auto first = Parse(kScenarioText);   // [Basic], [TeamTypes], [UNITS], [Waypoints]
    const std::string saved = SaveToString(*first);
    const auto second = Parse(saved);
    ASSERT_EQ(second->SectionCount(), first->SectionCount());
    for (int s = 0; s < first->SectionCount(); ++s) {
      const char* section = first->SectionName(s);
      ASSERT_EQ(second->EntryCount(section), first->EntryCount(section));
      for (int e = 0; e < first->EntryCount(section); ++e) {
        const char* entry = first->EntryName(section, e);
        EXPECT_EQ(second->GetString(section, entry, "?"),
                  first->GetString(section, entry, "!"));
      }
    }
  }
  ```

- [ ] **Step 2: Convert the writers.**

  ```cpp
  WWWritePrivateProfileString(INI_Name(), nullptr, nullptr, buffer);  // drop the section
  // becomes
  ini.Clear(INI_Name());

  WWWritePrivateProfileString(sec, key, value, buffer);   →  ini.PutString(sec, key, value);
  WWWritePrivateProfileInt(sec, key, n, buffer);          →  ini.PutInt(sec, key, n);
  WWWritePrivateProfileString(sec, key, nullptr, buffer); →  ini.Clear(sec, key);
  ```

  `Write_Scenario_Ini` reads the existing file into an `IniFile` (or starts from the same `[Basic]`
  defaults it formats today at `:730`), calls the writers, and saves through
  `OpenGameFile(fname, FileAccess::kWrite)` + `ini.Save(stream)` in place of `WriteProfileFile`.

- [ ] **Step 3: Verify.** Build and ctest; in the scenario editor (a `tdsdl` built with the scenario
      editor enabled), load SCG01EA, save it under a new name, and run `ini_compare` on the original
      and saved files plus a `diff` of their `sort`ed non-comment lines: only whitespace and comment
      differences. This step needs a display; if run headless, record it under Progress as an open
      manual check.

- [ ] **Step 4: Commit** — `Write Tiberian Dawn's scenarios through IniFile`.

### Task 8: TD configuration files on `IniFile`

**Files:** `td/data/options.cc` (40 calls, `CONQUER.INI`), `td/net/mplayer.cc` (48),
`td/net/internet.cc` (21), `td/net/stats.cc` (1), `td/startup/startup.cc` (8, including
`Read_Private_Config_Struct` from `td/data/profile.cc`), `td/map/display.cc` and
`td/objects/ house.cc` if any calls remain after Tasks 6-7.

- [ ] **Step 1: Write the failing test** for decision 6 in `src/td/data/options_test.cc` (create it,
      modelled on `src/ra/data/options_test.cc`, and add it to `td_archive_roundtrip_test`): a
      `CONQUER.INI` text with a comment, an unknown section `[Mine]` with `Foo=Bar`, and
      `[Options] GameSpeed=3` is loaded, the options are saved into the same `IniFile`, and
      afterwards `GetString("Mine", "Foo", "")` is `"Bar"` and `GetInt("Options", "GameSpeed", -1)`
      is the saved value.

- [ ] **Step 2: Convert each file.** A function that does `ReadProfileFile("CONQUER.INI")` …
      `WWWritePrivateProfile*` … `WriteProfileFile(...)` becomes:

  ```cpp
  engine::file::IniFile ini;
  if (const auto file = OpenGameFile("CONQUER.INI")) {
    ini.Load(*file);
  }
  ini.PutInt("Options", "GameSpeed", GameSpeed);
  ...
  // The same opener WriteProfileFile uses (engine/file/game_file.h).
  if (const auto out = OpenGameFile("CONQUER.INI", FileAccess::kWrite)) {
    ini.Save(*out);
  }
  ```

  `Read_Private_Config_Struct(char* profile, NewConfigType*)` takes `const IniFile&`. The one
  pointer-result use, `td/startup/startup.cc:538`
  (`if (WWGetPrivateProfileString("Options", "DestNet", nullptr, netbuf, ...))`), becomes
  `if (ini.HasEntry("Options", "DestNet"))` followed by `GetString`. `td/net/internet.cc` reads
  WChat's own INI file: same pattern, read-only.

- [ ] **Step 3: Verify.** Build, ctest, the smoke fixtures, and a headless start in a scratch
      directory with an existing `CONQUER.INI`: after `-QUITFRAME60`, `ini_compare` the file and
      check every pre-existing key is still present.

- [ ] **Step 4: Commit** — one per file group: options; startup; multiplayer and internet.

### Task 9: Delete the profile reader

**Files:**

- Delete: `src/td/data/profile.h`, `src/td/data/profile.cc`, `src/td/data/number_parse_test.cc`'s
  profile cases (move any that test number parsing to an `IniFile` test),
  `src/engine/file/ profile_buffer.{h,cc}`, `profile_buffer_test.cc`,
  `src/ra/data/profile_buffer_test.cc`, `src/tools/ini_compare.cc` and its CMake lines.
- Modify: `src/ra/data/profile.{h,cc}` (remove `WWGetPrivateProfile*`; `Read_Bin_*`/`Write_Bin_*`
  stay), `src/ra/screens/expand.cc` (2 calls → `IniFile`), `src/ra/wol/wol_gsup.cc` (delete the
  commented-out calls), `src/engine/file/CMakeLists.txt`, `src/ra/CMakeLists.txt`, `CLAUDE.md` (the
  `file/` line of the architecture tree: "the INI profile buffer" → "the INI database").

- [ ] **Step 1:**
      `grep -rnE "WW(Get|Write)PrivateProfile|ReadProfile|WriteProfile|profile_buffer"     src` →
      only the files above. Keep `ReadProfileText`/`ReadProfileFile` if TD's scenario CRC still uses
      them to fetch the raw text; in that case move the two into `ini_file.h` as
      `ReadIniText`/`ReadIniFile` instead of deleting them.
- [ ] **Step 2:** Delete, build both trees, ctest, both smoke scripts.
- [ ] **Step 3: Commit** — `Remove the profile text reader`.
- [ ] **Step 4:** Record the outcome at the top of this document (status line, commit range, open
      manual checks) and add the leftovers to `TODO.md` with `/add-todo`.

---

## Verification

Run after every task that changes a game; exact commands:

```bash
JOBS=22
cmake --build build --parallel $JOBS && ctest --test-dir build --output-on-failure
cmake --build cmake-build-strict-ra-clang --parallel 14
ctest --test-dir cmake-build-strict-ra-clang --output-on-failure
tools/ra_saveload_smoke.sh build/src/ra/rasdl
tools/td_saveload_smoke.sh build/src/td/tdsdl SCG01EA --team    # and the other fixtures
```

Always pass the binary to the smoke scripts; with no argument they test the CLion strict directory.

**Headless logic check.** Build the pre-plan commit (578e1d94) into a separate directory once
(`git worktree add ../ini-base 578e1d94 && cmake -S ../ini-base -B ../ini-base/build -G Ninja -DSTRICT_CHECKS=OFF && cmake --build ../ini-base/build --parallel 22`).
In a scratch working directory per binary, do one warm-up run, then:

```bash
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy <rasdl> -NOMOVIES -NEWGAMESCU05EA -QUITFRAME600 \
    -CD"$HOME/.local/share/Steam/steamapps/common/Command & Conquer Red Alert" > ra.log 2>&1
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy <tdsdl> -NOMOVIES -SEED1 -NEWGAMESCG01EA \
    -QUITFRAME600 -CD"$HOME/.local/share/Steam/steamapps/common/Command & Conquer" > td.log 2>&1
```

Run the old binary twice first to measure noise (RA SCG01EA is nondeterministic even old-vs-old; use
SCU05EA). Then `diff <(grep '^frame' old.log) <(grep '^frame' new.log)`: RA must be line-identical;
TD's `fields` lines identical and `mapstate` within the old-vs-old noise.

**Manual checks that need a display or a second machine** (record as open if not done): the TD
scenario editor save in Task 7; a two-player RA lobby to confirm matching `rules.ini` IDs after
Task 2.

## Out of scope

- **Rejecting a digest mismatch.** `LoadVerified` now reports it; making `Read_Scenario_INI` refuse
  tampered official scenarios (with the original exemption for multiplayer maps 1-24) changes what
  loads and is the user's call.
- **RA's `Read_Bin_*`/`Write_Bin_*`** in `ra/data/profile.*`.
- **Typed accessors for TD** or a shared `game/` accessor layer.
- **Preserving comments** in written INI files.
- **TD's dead `#ifdef FRENCH/GERMAN` branches**: they compile nowhere, so the compiler will not flag
  profile calls inside them; convert them by reading, or leave them for the language-ifdef cleanup,
  and say which in the Task 9 outcome.

## Outcome

### Commits

| Task                                    | Commits                                                              |
| --------------------------------------- | -------------------------------------------------------------------- |
| 1. `INIClass` → `engine::file::IniFile` | 583feb2b, 6c58a4f9                                                   |
| 2. Dissolve `CCINIClass`                | 9902cb8f, 57bffc8b, 511c8d17                                         |
| 3. Case, line length, duplicates        | 92c7bcdc                                                             |
| 4. `ini_compare`                        | 98b42753, 1d3f2ca7 (tool deleted again in 775b0a11)                  |
| 5. Google-style names                   | 2614bce5, e19e8618, db318487, 4508196d, 826c31fc                     |
| 6. TD scenario reading                  | 88cf160e, 209ecca8, 8751ec50, 15789994, 97560cf4, 877f528e, 98b36d13 |
| 7. TD scenario writing                  | 6d9a8d7e, 266de234                                                   |
| 8. TD configuration files               | f1e69e8e, 9a7517ad, 7611cced, dde94525, 2603318b, 9a8c35ff, e827238a |
| 9. Delete the profile reader            | 775b0a11                                                             |
| Whole-branch review fixes               | f9b7db72, 83c358c4                                                   |

### Measurements

- **`ini_compare` over the shipped Tiberian Dawn files** (Task 4): 82 files (26 SCG, 33 SCB, 16 SCM,
  5 SCJ, `MISSION.INI`, `CONQUER.INI`), 1,847 sections, 45,164 keys. Value differences 0, order
  differences 0. Two keys with an empty value (`CONQUER.INI` `[Options] Socket=` and `DestNet=`) and
  93 entry-less sections (`[SMUDGE]` 52, `[REINFORCEMENTS]` 8, `[CellTriggers]` 7, `[Teams]` 7,
  `[STRUCTURES]` 5, `[TEMPLATE]` 5, `[UNITS]` 4, `[OVERLAY]` 3, `[INFANTRY]` 2), which `IniFile`
  drops and the old reader found; both read the same either way. No file ends without a newline.
- **Line lengths**: no line over 127 characters in Red Alert's `RULES.INI` (longest 122) or
  `SCU05EA.INI`; Tiberian Dawn's longest value is 191 characters (a `[TeamTypes]` entry).
- **Name collisions under the old checksum** (review fix): 0 over 91 shipped files of both games
  (2,235 sections, 49,446 entries).
- **Headless frame logs against the pre-plan binary** (578e1d94 built with the same `.env`), at each
  of Tasks 6-9 and at the final commit: Tiberian Dawn SCG01EA 67,914 lines, SCB01EA 89,713, SCG15EA
  159,555, each with 0 differing lines and 0 noise between two old runs; Red Alert SCU05EA 24,600
  lines, 0 differing.
- **Settings**: with a `CONQUER.INI` that has an `[Options]` section, old and new Tiberian Dawn
  binaries produce identical frame logs (70,184 lines, different from the default-settings run) and
  leave the same 14 `section/key=value` triples behind. A hand-edited file with a comment, an
  unknown section, an unknown key and the two empty keys is byte-identical after a run.
- **Tests**: 1,153 before the plan, 1,215 after, in both the GCC and the strict clang build; the
  save/load smoke scripts of both games pass.

### Where execution departed from the plan

- **Task 1, duplicates.** The plan re-pointed the index at the next duplicate when an entry was
  removed, which made a `Put` not read back. Instead `PutString()`, `Clear(section)` and
  `Clear(section, entry)` remove every element of that name.
- **Decision 3, how names are compared.** The plan kept the lookup by a checksum of the name. That
  checksum (`CrcEngine`, a rotate-and-add) collides for names such as `gdi13`/`hdi11` and
  `TEAM3`/`UEAM1`; Red Alert always had that, but for Tiberian Dawn it would have been new. The
  indexes are keyed by the name itself, compared without ASCII case.
- **Task 2, `SaveWithDigest()`** writes the data and the digest section through one non-finishing
  helper and finishes the sink once. `ra/ai/scenario.cc` loads with `LoadVerified()` and accepts a
  mismatch, as before.
- **Task 5, names.** `LineCount()` and `SavedSize()` do not exist: the old `Line_Count()` and
  `Size()` were declared and never defined. `GetTextBlock()`, `GetBase64Block()` and
  `PutBase64Block()` take the span alone. `Clear()` is three overloads, and no name parameter can be
  null. `PutPublicKey()` was deleted as unused.
- **Task 6, an unterminated last line** is parsed (in `IniFile::Load()`, for both games); the
  original Red Alert reader dropped it and Tiberian Dawn's read it. Every shipped file of both games
  ends in a newline.
- **Tasks 6-8, loading.** Tiberian Dawn loads every `IniFile` from the text `ReadIniFile()` /
  `ReadIniText()` return, which ends at the first NUL as the old readers' text did, through
  `td/data/ini_io.{h,cc}`. Red Alert loads from the stream.
- **Task 6, buffers.** Where the old call reserved one byte (`first(sizeof(buf) - 1)`), the whole
  buffer is passed, so a value may be one character longer.
- **Task 7, order.** New entries are appended to their section (the old writer inserted at the top,
  so the editor saved lists in descending order); an updated key moves to the end of its section,
  and a cleared and rewritten section to the end of the file.
- **Task 8, startup.** `CONQUER.INI` is saved at startup only when `[Intro] PlayIntro` is not
  already `No`, so a normal launch leaves a hand-edited file untouched.
- **Task 8, init strings.** An empty or blank modem init string cannot be stored, so the Add button
  refuses one and the reader validates the stored indexes. A file the old binary wrote with an empty
  init string may select a different string after its first load.
- **Task 8.** `Read_Private_Config_Struct()` had no caller and was deleted with `NewConfigType`.
- **Task 9.** `ReadIniText()` and `ReadIniFile()` (the old `ReadProfileText()`/`ReadProfileFile()`)
  live in `engine/file/ini_file.h`; Tiberian Dawn's scenario CRC still hashes that text.
- **Tiberian Dawn's `#ifdef FRENCH`/`GERMAN` branches** held no profile calls in the converted
  functions; the final grep for the old names over `src/` is empty.

### Known reader differences for Tiberian Dawn

None of these occurs in a shipped file.

- A `;` anywhere in a value starts a comment and the value is cut there, on every load. The old
  reader kept it. A custom mission's briefing line or a stored handle containing `;` reads
  truncated. Left as is; see `TODO.md` for the decision.
- A line starting with `#` is an entry, not a comment; a section header is recognized after trimming
  blanks and needs its `]`; an `=value` line is skipped instead of ending an enumeration.
- `GetInt()` also accepts `$FF` and `0FFh` and no longer cuts a number at 15 characters; a number
  followed by `; comment` now parses.
- A file the game rewrites loses its comments, its blank-value keys, later duplicates of a key and
  any text before the first section. Unknown sections and keys are kept.

### Open checks

Recorded in `TODO.md`:

- Tiberian Dawn's scenario editor: load SCG01EA, save it under a new name and compare. No automated
  run reaches `Write_Scenario_Ini()`, and the unit, infantry, building, terrain, aircraft and house
  writers have no test.
- Tiberian Dawn's options dialog and the `DEMO`/Windows-only branches, `Check_From_WChat()` and
  `Read_Game_Options()`, which were converted by reading.
- A two-player Red Alert lobby, for matching `rules.ini` IDs.

### Found on the way, not fixed here

- `td/data/ini.cc` reads and writes `[Basic] "carry_over_cap"` where the key is `CarryOverCap`
  (b2135cdf renamed the variable and the string with it). The pre-plan binary has the same bug, and
  fixing it would have changed behaviour against the comparison baseline.
- `td/net/mplayer.cc`'s phone-book reader overwrites the buffer it is tokenizing, so the fields
  after `DialMethod` always read as defaults.
- Headless comparisons: the games read `CONQUER.INI` from the directory of the executable. A stray
  settings file beside one binary (with `GameSpeed=0`) made identical sources look different from
  frame 2. Compare copies of the binaries in clean directories with the same settings file.
