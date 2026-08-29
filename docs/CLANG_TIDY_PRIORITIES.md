# Clang-Tidy Check Prioritization Analysis

## Overview

`.clang-tidy` enables all checks (`'*'`) and then disables **230** of them. This document prioritizes which of those
230 to re-enable, ordered by **measured bug yield per unit of fix effort**.

Unlike the previous revision of this document, the tiers below are not guesses. They come from an actual measurement run
(see [Methodology](#methodology)). Every count in the tables is a real diagnostic site in this repository.

**Tier 1, §1.1, §1.2 and Tier 1.5 are complete, and Tier 2 is four checks of seven in.** All ten rows of the Tier 1
table (13 checks) are enabled, 24 of the 26 checks in §1.1, the single §1.2 check, all six of Tier 1.5, and three of
Tier 2 with a fourth reclassified; see [Progress](#progress). The two §1.1 checks left out are deliberate — see
[1.1](#11-free-guards--the-name-was-wrong). What is left in Tier 2 is `narrowing-conversions`,
`pro-type-member-init` and `switch-missing-default-case`.

The clang-tidy 22 move on 2026-08-20 turned the strict build red for reasons unrelated to any tier below; all four
causes are resolved and it is green again — see [clang-tidy 22 fallout](#clang-tidy-22-fallout).

### Two facts that shape everything below

1. **`WarningsAsErrors: '*'`.** Enabling a check does not "start reporting" it — it *breaks the build* until every site
   is fixed. Check selection is therefore a scheduling decision, not a reporting decision.
2. **The clang build already passes `-Weverything`** (`CMakeLists.txt:107`). Every `clang-diagnostic-*` entry in the
   disable list is *already being printed on every build*; disabling it in `.clang-tidy` only decides whether it is
   fatal. For these checks the discovery cost is zero — you are choosing what to enforce, not what to find.

---

## Methodology

57 disabled checks were re-enabled and run over a 16-file sample (~49,700 lines, **~10% of `src/` by line count**),
deduplicated by source location so shared headers count once.

```bash
# Sample: src/ra/{conquer,display,house,techno,infantry,building,cell,radar,init,saveload,scenario,ini,mission_id}.cc
#         src/sdllib/{gbuffer,wsa}.cc  src/tech/lcw.cc
clang-tidy -p cmake-build-strict-ra-clang --quiet \
    --checks="-*,<comma-separated-candidates>" --warnings-as-errors= <file>
```

**Reading the counts:** "Sample" is the observed number of distinct sites. Tree-wide is roughly **10×** that, but the
sample deliberately favours large, busy files, so treat 10× as an upper bound for `src/ra` — and note `src/td` (294
files) will contribute its own, largely parallel, set.

⚠️ **The 10× rule held for effort, not for counts.** Now that Tier 1 has actually been cleared, the sample turns out to
be a poor predictor of volume:

| Check                                | Sample said | Tree-wide actual                   |
|--------------------------------------|-------------|------------------------------------|
| `clang-diagnostic-writable-strings`  | 9           | **357** diagnostics over 244 lines |
| `bugprone-suspicious-enum-usage`     | 1           | **617** call sites                 |
| `clang-diagnostic-format`            | 10          | ~160                               |
| `bugprone-too-small-loop-variable`   | 5           | 36 loops                           |
| `bugprone-suspicious-string-compare` | 1           | 13                                 |
| §1.1 as a whole                      | 0           | **336** sites across 26 checks     |

§1.1 is the sharpest case: the sample saw zero hits in all 26 checks and the section was written up as free, but three
of them alone (`conditional-uninitialized` 103, `optin.cplusplus.VirtualCall` 86, `implicit-fallthrough` 33) carry 222
sites.

Two effects the sample could not see. First, it contains no `src/td` files, and TD carries a near-duplicate copy of most
of this code — `writable-strings` landed 115 sites in RA against 241 in TD. Second, a single declaration can carry an
unbounded number of sites: `td/globals.cc` alone accounts for 126 of the 357 `writable-strings` diagnostics because
`SerialPacketNames[]` is a 100-entry table of literals. Use the sample to rank checks, not to budget them.

What did hold: every Tier 1 check was cheap *per site*, and each one paid for itself in real bugs.

Last measured: 2026-07-25, clang-tidy 21.1.8. Tier 1 and §1.1 outcomes measured as each check was enabled, through
2026-08-20; the §1.1 counts come from clang-tidy 22 over the full `cmake-build-strict-ra-clang-22` compile database
rather than from the sample. The §1.2 and clang-tidy 22 counts are clang-tidy 22.1.8 over that same database, all 838
project translation units (423 sources plus the 415 header-verification units).

---

## Status Corrections

An earlier revision of this document marked four checks "✅ DONE" while they were still disabled, and listed three as
deferred that were in fact already enabled. All seven now agree with `.clang-tidy` — the four that were falsely claimed
were enabled for real as part of §1.1:

| Check                                | Once claimed     | Now                              |
|--------------------------------------|------------------|----------------------------------|
| `clang-diagnostic-uninitialized`     | Tier 1 ✅ DONE   | enabled in `c9bb3fb0` (13 sites) |
| `clang-analyzer-cplusplus.NewDelete` | Tier 1 ✅ DONE   | enabled in `12f738d9`            |
| `clang-diagnostic-self-assign`       | Tier 2.1 ✅ DONE | enabled in `12f738d9`            |
| `misc-redundant-expression`          | Tier 2.1 ✅ DONE | enabled in `12f738d9`            |
| `modernize-use-nullptr`              | Tier 3, "defer"  | was already enabled              |
| `modernize-use-override`             | Tier 3, "defer"  | was already enabled              |
| `readability-else-after-return`      | Tier 4, keep off | was already enabled              |

The lesson stands even though the discrepancy is gone: this document is not the source of truth for what is enabled.
`.clang-tidy` is. Check a claim against it before acting on it.

Also genuinely done and confirmed enabled: `clang-diagnostic-suggest-override`, `-return-stack-address`,
`-mismatched-new-delete`, `-delete-incomplete`, `-sometimes-uninitialized`, `-unused-variable/-function/-parameter`,
`clang-analyzer-core.NullDereference`, `clang-analyzer-core.uninitialized.*`, `clang-analyzer-deadcode.DeadStores`,
`clang-analyzer-unix.MismatchedDeallocator`, `bugprone-not-null-terminated-result`,
`readability-redundant-control-flow`, `readability-redundant-declaration`.

---

## Progress

Tier 1 is done. Each check was enabled in its own commit, with every site it flagged cleared first, so `main` has never
been red on an enabled check.

| Check                                                                   | Commit     | Outcome                                                                                                                                 |
|-------------------------------------------------------------------------|------------|-----------------------------------------------------------------------------------------------------------------------------------------|
| `clang-diagnostic-int-in-bool-context`                                  | `f7552ee3` | The `house.cc` `Make_Enemy` bug below; that was its only site tree-wide                                                                 |
| `bugprone-suspicious-memory-comparison`                                 | `aee456e6` | `EventClass::operator==` (padding + union, both games) deleted — it had no callers; `CellClass::Should_Save` never worked at all        |
| `clang-analyzer-security.ArrayBound`                                    | `2cc33f7a` | 4 real out-of-bounds accesses, incl. `buttons[-1]` in TD's multiplayer menu                                                             |
| `bugprone-suspicious-enum-usage`                                        | `6a792756` | 617 sites via a new `ButtonKey()` helper; exposed the Alt+W cheat comparing `KA_W` instead of `KN_W`, so it never fired                 |
| `bugprone-too-small-loop-variable`                                      | `c4571d27` | 36 map loops; `MAP_CELL_W/H/TOTAL` retyped as `CELL` so they cannot regress                                                             |
| `bugprone-signed-char-misuse`                                           | `c4f61444` | 4 sign-extension bugs, incl. a truncated PCX RLE run and a read before `IsTranslucent[]`                                                |
| `bugprone-multi-level-implicit-pointer-conversion`                      | `f7c93bc9` | `XMP_Is_Small_Prime` bsearching a stack address; `KeyFrameSlots` clearing half its allocation on 64-bit; `Stop_Speaking` never matching |
| `clang-diagnostic-format` (+ `-nonliteral`, `-security`, `-signedness`) | `b1e57f20` | ~160 sites; runtime-only formats now funnel through `Format_Runtime_Text` in `jshell.h`                                                 |
| `clang-diagnostic-writable-strings`                                     | `dd96637d` | 357 diagnostics; mostly mechanical `const`, but 3 were live writes into string literals                                                 |
| `bugprone-suspicious-string-compare`                                    | `d58e3977` | 13 sites, all `!= 0`; no behaviour change                                                                                               |

### §1.1

| Checks                                                                            | Commit     | Sites | Outcome                                                                                                                                                                                               |
|-----------------------------------------------------------------------------------|------------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 18 checks — the cheap end (7 genuinely clean, 11 with 51 sites)                   | `12f738d9` | 51    | 4 real defects; `EventClass` now zeroes itself instead of 7 hand-written memsets; TD `TrackNumber`/`TrackIndex`/`Page` retyped `char` → `int` (bumps TD `SAVEGAME_VERSION`)                           |
| `missing-field-initializers`, `implicit-fallthrough`, `conditional-uninitialized` | `c2121fa4` | 156   | `td/mapsel.cc` frame 23 fell into the frame that blacks out what it just drew; `sdllib/iff.cc` memcpy arguments reversed; `ra/nullmgr.cc` returned an uninitialized `DialStatusType` on modem timeout |
| `clang-diagnostic-uninitialized`, `clang-analyzer-core.CallAndMessage`            | `c9bb3fb0` | 31    | 3 real null dereferences, each in both games (`sidebar.cc`, `display.cc`, `queue.cc`); TD's `NoInitClass` constructors no longer self-init a const member                                             |

### §1.2

| Check                                           | Commit     | Sites | Outcome                                                                                                                            |
|-------------------------------------------------|------------|-------|------------------------------------------------------------------------------------------------------------------------------------|
| `bugprone-raw-memory-call-on-non-trivial-type`  | `c580283d` | 0     | Free, and it guards nothing this document thought it did — see [1.2](#12-the-check-that-was-not-what-this-document-thought-it-was) |
| — (no check; `src/{ra,td}/heap_layout_test.cc`) | `201cfb3c` | —     | `sizeof()` pinned for 44 RA and 35 TD byte-serialized types; the actual tripwire on the save format                                |

### 1.5

| Check                                   | Commit     | Sites | Outcome                                                                                                                                       |
|-----------------------------------------|------------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------|
| `misc-unconventional-assign-operator`   | `7a9e933b` | 9     | `virtual` dropped from `VectorClass::operator=` in both games; TD's `LinkClass` brought in line with RA                                       |
| `cert-oop58-cpp`                        | `30cb1c6e` | 2     | `GenericNode`'s copy operations deleted — they spliced the destination into the source's list rather than copying                             |
| `clang-diagnostic-overloaded-virtual`   | `bc3c370c` | 28    | 6 live bugs: vessel damage, TD turret-locked movement, anim/bullet refresh lists in both games, RA checklist items, 4 null-modem stubs        |
| `cppcoreguidelines-virtual-class-destructor` + `clang-diagnostic-non-virtual-dtor` | `fdb3a9e8` | 33 | 6 edits — a virtual destructor on each hierarchy root; 3 of them only restore RA/TD parity. No layout change              |
| `clang-analyzer-optin.cplusplus.VirtualCall` | `e3ad8275` | 81 | 17 classes and 8 methods marked `final`, 5 qualified calls, 4 restructures, 0 NOLINTs; found buffered writes lost in `~BufferIOFileClass` |

`src/ra/vector.h` and `src/tech/listnode.h`, which Phase B named as the prerequisite, were cleared by the first two.

### 2

| Check                                                 | Commit     | Sites | Outcome                                                                                                                     |
|-------------------------------------------------------|------------|-------|-------------------------------------------------------------------------------------------------------------------------------|
| `bugprone-implicit-widening-of-multiplication-result` | `c0045ac4` | 224   | Stride and buffer-size types, not casts; 3 timer macro definitions cleared 43 sites at once                                 |
| `clang-diagnostic-sign-compare`                       | `35c51bbf` | 305   | 8 type changes cleared ~240; the cause was RA being migrated to signed sizes and TD not                                     |
| `clang-diagnostic-shorten-64-to-32`                   | `cf44023c` | 439   | Much of it created by the two above — signedness work moves the mismatch from comparisons into assignments                  |
| — (`clang-diagnostic-switch`, not enabled)            | —          | 720   | Reclassified: 567 are `case ButtonKey(n):`, a deliberate idiom `-Wswitch` cannot model — see [Tier 2](#tier-2-valuable-but-these-are-the-type-migration-project) |

Two supporting commits: `7553a5ce` replaced 34 timer and object-limit macros with `inline constexpr` constants, which
is what made the timer durations typed enough to fix; `532aef0f` cleared the Tier 1.5 fallout.

Three lessons worth carrying into the next tier:

- **A "mechanical" check is not automatically safe.** `writable-strings` looked like a pure `const` sweep, yet three
  sites were genuinely writing through a literal: `Format_Window_String` rewrites its buffer in place, `TextLabelClass`
  stores the pointer in a non-const member, and TD's serial dialog `strncpy`s into `CallWaitStrings[CALL_WAIT_CUSTOM]`.
  Adding `const` to those would have moved the error, not fixed it. Read what the callee does before const-ing the
  caller.
- **RA and TD diverge.** RA's copy of that same call-waiting loop had already been reworked to `std::string`, so the two
  ports needed opposite fixes. Never assume a fix ports across verbatim.
- **A check name is not a check.** §1.2 was budgeted for two revisions of this document on the strength of its name.
  `bugprone-raw-memory-call-on-non-trivial-type` is not the trivially-copyable UB check it sounds like; it is
  `cert-oop57-cpp`, which was already enabled, and the check the section described
  (`bugprone-undefined-memory-manipulation`) was already enabled too. Check names are also added, renamed and aliased
  between releases. Confirm against `clang-tidy --list-checks` *and* the check's own documentation before writing a
  line of this document about it.
- **Adding a virtual destructor renumbers a vtable, so land it with a build nobody is racing.** `GScreenClass` had no
  declared destructor and therefore no destructor slot; adding one shifts the index of every virtual after it, right
  down the `GScreenClass → MapClass → DisplayClass → … → MapEditClass` chain. Ninja gets this right on its own, but a
  build already running when the header is saved keeps objects it had already stat'ed, and the link silently mixes two
  vtable layouts. The result is not a crash at the edit: it is one virtual call landing in a different virtual
  entirely — a `Help_Text(TXT_NONE)` in `sidebar.cc` arriving in `MapEditClass::Scroll_Map`, which then read its
  `int&` argument from `TXT_NONE`. A call that lands in the wrong function is a stale build, not a logic bug; rebuild
  the directory before debugging it. `sizeof()` is untouched either way when the class already had virtual functions,
  so the layout tests stay green and cannot catch this.
- **Sweep with the project's own warning flags.** `clang++ -Wno-everything -W<name>` is not a reliable way to isolate
  one diagnostic: `-Wconditional-uninitialized` needs `-Wuninitialized` enabled to fire at all, so that form reported
  zero sites where the real count was 103. Run the compile database with its own flags (which already include
  `-Weverything`) and filter the output by warning name. Check for hard `error:` lines while filtering, too — a
  translation unit that fails to compile reports no warnings, which reads exactly like a clean one.

---

## TIER 1: Enable now — real bugs, negligible volume

> **Status: complete.** Retained for the reasoning; the per-check outcomes are in [Progress](#progress).

The measurement found a **live bug in shipped logic** on its first pass, `src/ra/house.cc:2367`:

```cpp
void HouseClass::Make_Enemy(HousesType house) {
  ...
  Allies &= ~(1L << house);
  if (ScenarioInit) {
    Control.Allies &= !(1L << house);   // '!' should be '~'
  }
```

`1L << house` is always non-zero, so `!(...)` is `0` and the statement clears **every** ally bit instead of one. It is
caught by `clang-diagnostic-int-in-bool-context`, which had exactly one hit in the entire sample — and, as it turned
out, exactly one in the entire tree. Fixed in `f7552ee3`.

| Check                                                    | Sample | What it caught                                                                                                        |
|----------------------------------------------------------|--------|-----------------------------------------------------------------------------------------------------------------------|
| `clang-diagnostic-int-in-bool-context`                   | 1      | the `house.cc:2367` bug above                                                                                         |
| `bugprone-suspicious-memory-comparison` (`cert-flp37-c`) | 1      | `ra/event.h:265` — `EventClass::operator==` memcmps a type with padding; this is the **multiplayer event** comparison |
| `clang-analyzer-security.ArrayBound`                     | 1      | heap-underflow path in `ra/search.h:515`                                                                              |
| `bugprone-suspicious-enum-usage`                         | 1      | mixed enum families in `ra/conquer.cc:543`                                                                            |
| `bugprone-too-small-loop-variable`                       | 5      | `CELL` loop counters against `int` bounds in `ra/display.cc` — truncation                                             |
| `bugprone-signed-char-misuse` (`cert-str34-c`)           | 7      | `signed char`→`int` on file/INI data (`display.cc`, `infantry.cc`, `init.cc`)                                         |
| `bugprone-multi-level-implicit-pointer-conversion`       | 5      | `ObjectClass**`→`void*` in `saveload.cc`, `cell.cc` — the save/load memcpy paths                                      |
| `clang-diagnostic-format`                                | 10     | genuine `printf`/`scanf` type mismatches in `ini.cc`, `radar.cc`, `init.cc`                                           |
| `clang-diagnostic-writable-strings`                      | 9      | string literal → `char*`; mostly a mechanical `const` fix                                                             |
| `bugprone-suspicious-string-compare`                     | 1      | `stricmp` result used without comparison (`scenario.cc:1923`)                                                         |

The sample predicted **~41 sites**, "realistically a few hundred tree-wide". Clearing the tier actually touched **206
files under `src/`** and well over a thousand sites — see the volume caveat under [Methodology](#methodology). The
estimate of *cost per site* was right; the estimate of *site count* was not.

### 1.1 Free guards — the name was wrong

These 26 checks were clean across the entire 10% sample, so this section was written up as free. Measured against every
translation unit in `cmake-build-strict-ra-clang-22` (1001 entries: 586 regular plus 415 header-verification units) they
cost **336 sites**. Twenty-four were enabled and cleared — see [Progress](#progress) for the three commits and what
they turned up. Twenty-three are still on: clang-tidy 22 broadened
`bugprone-unchecked-string-to-number-conversion` from clean to 200 sites, and it was disabled rather than swept — see
[clang-tidy 22 fallout](#clang-tidy-22-fallout).

Enabled, with their measured tree-wide counts:

| Check                                         | Sites |
|-----------------------------------------------|-------|
| `clang-diagnostic-conditional-uninitialized`  | 103   |
| `clang-diagnostic-implicit-fallthrough`       | 33    |
| `clang-diagnostic-missing-field-initializers` | 20    |
| `clang-analyzer-core.CallAndMessage`          | 18    |
| `clang-diagnostic-uninitialized`              | 13    |
| the other 19, together                        | 51    |

The remaining 19 are `bugprone-unhandled-self-assignment`, `bugprone-sizeof-expression`,
`bugprone-redundant-branch-condition`, `bugprone-inc-dec-in-conditions`,
`bugprone-unchecked-string-to-number-conversion` (since disabled), `clang-diagnostic-self-assign`,
`clang-diagnostic-char-subscripts`, `clang-diagnostic-null-arithmetic`, `clang-diagnostic-logical-not-parentheses`,
`clang-diagnostic-format-security`, `clang-diagnostic-varargs`, `misc-redundant-expression`,
`misc-definitions-in-headers`, `readability-misleading-indentation`, `cert-oop57-cpp`,
`clang-analyzer-cplusplus.NewDelete`, `clang-analyzer-optin.cplusplus.UninitializedObject`,
`clang-analyzer-unix.Stream`, `clang-analyzer-security.VAList` — seven of which really are clean tree-wide.

**Two are deliberately left disabled.**

`bugprone-parent-virtual-call` — 12 sites, all of them intentional grandparent dispatch:
`TriColorGaugeClass::Draw_Me` reaches `ControlClass::Draw_Me` past `GaugeClass`, `ListClass::Draw_Me` reaches
`GadgetClass::Draw_Me` past `ControlClass`, `UnitClass::Assign_Destination` reaches `FootClass::` past `DriveClass`, and
`td/unit.cc` dumps `CargoClass`, `MissionClass` and `TarComClass` in a row on purpose. Enabling it would cost twelve
NOLINTs and prevent nothing. This one should stay off permanently.

`clang-analyzer-optin.cplusplus.VirtualCall` — 86 sites, virtual calls from constructors and destructors. Some are real
(`LinkClass::Add` from a constructor and `RawFileClass::Close` from a destructor both mean a derived override never
runs); most are benign because the class has no subclasses. Clearing it means restructuring constructors and destructors
across `AbstractClass → ObjectClass → TechnoClass → …`, which is exactly the hierarchy refactoring `CLAUDE.md` says not
to start unasked. It is listed under [Tier 1.5](#tier-15-small-counts-real-ub-in-a-deep-virtual-hierarchy) instead.

### 1.2 The check that was not what this document thought it was

**`bugprone-raw-memory-call-on-non-trivial-type`** — enabled in `c580283d`. 0 sites, measured with clang-tidy 22 over
all 838 translation units in `cmake-build-strict-ra-clang-22` (423 project sources plus the 415 header-verification
units).

An earlier revision of this section called it "the single most valuable check in the whole list", on the grounds that
it was the only automated guard on the `TFixedIHeapClass::Save/Load` memcpy contract in `src/ra/heap.cc`. Both halves
of that were wrong.

**It is a different check.** `bugprone-raw-memory-call-on-non-trivial-type` is clang-tidy 22's *primary* name for
`cert-oop57-cpp` — the CERT OOP57-CPP style rule, "prefer special member functions and overloaded operators to C
Standard Library functions", covering `memset`, `memcpy`, `memcmp`, `strcpy`, `strcmp` and friends. `cert-oop57-cpp`
was never in the disable list, so the alias already had that coverage in force; enabling the primary name only makes it
survive the alias being retired. The trivially-copyable UB check the section actually described is a **separate**
check, `bugprone-undefined-memory-manipulation`, which still exists in clang-tidy 22 and is also already enabled. It
too measures 0.

**Neither one can see the save path.** `src/ra/heap.cc:508` and `:568` are `file.Put(Ptr(i), sizeof(T))` and
`file.Get(ptr, sizeof(T))`, going through `Pipe::Put(const void*, int)` and `Straw::Get(void*, int)`. No `memcpy` is
textually present and the `T*` decays to `void*` at the call boundary, so no class-typed pointer ever reaches a call
the checks can match — and the matcher additionally carries `unless(isInTemplateInstantiation())`, which is exactly
what `heap.cc` is. The same holds for `src/td/heap.cc:485`, `src/ra/iomap.cc` (`CellClass`, `MouseClass`),
`src/td/ioobj.cc` (~40 `Read_Object`/`Write_Object` pairs), `src/ra/saveload.cc` (`ScenarioClass`, `ScoreClass`,
`CarryoverClass`, `SpecialClass`, `GameOptionsClass`) and `src/ra/vortex.cc`.

**What guards it instead.** No standard trait works: `AbstractClass` declares a virtual destructor
(`src/ra/abstract.h:70`), so every serialized type fails both `is_trivially_copyable_v` and
`is_trivially_destructible_v`, and there is no trait for "trivially copyable apart from the vptr that the
placement-new restores". `sizeof(T)` is the closest observable proxy — a `std::string`, `std::vector` or
`std::optional` member changes it. `src/ra/heap_layout_test.cc` and `src/td/heap_layout_test.cc` (added in `201cfb3c`)
pin it for every byte-serialized type, so the failure names the type.

#### `SAVEGAME_VERSION` already does half of this, with gaps

`SAVEGAME_VERSION` (`src/ra/saveload.cc:132-145`, `src/td/saveload.cc:99-112`) is a sum of `sizeof()` over the
serialized types, so a layout change invalidates existing saves rather than corrupting loads. Neither sum is complete:

| Game | Byte-serialized but absent from the sum                                                                                                                                     |
|------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| RA   | `VesselTypeClass`, `WeaponTypeClass`, `WarheadTypeClass`, `ScoreClass`, `CarryoverClass`, `SpecialClass`, `GameOptionsClass`, `NodeNameType`, `EventClass`, `BaseNodeClass` |
| TD   | `TriggerClass`, `ScoreClass`, `EventClass`, `BaseNodeClass`, `WeaponTypeClass`, `WarheadTypeClass`                                                                          |

For those types a layout change is caught by nothing but the new tests. Extending either sum would invalidate every
existing save, so it has not been done; the gap is recorded here rather than closed.

---

## clang-tidy 22 fallout

The toolchain moved from clang-tidy 21 to 22 on 2026-08-20, which turned the strict build red. None of it came from
§1.2 — the four checks below were all enabled through `'*'` and all four are either new in 22 or newly stronger in 22.
Measured over the same 838 translation units, deduplicated by site. **All four are now resolved and the strict build
is green again**: two swept, two disabled.

| Check                                            | Sites | Files | Status                                                                                                                              |
|--------------------------------------------------|-------|-------|-------------------------------------------------------------------------------------------------------------------------------------|
| `bugprone-unchecked-string-to-number-conversion` | 200   | 47    | ✅ Disabled → [Tier 3](#tier-3-keep-disabled--high-volume-near-zero-bug-yield). Was clean under 21 as part of §1.1; 22 broadened it |
| `readability-redundant-parentheses`              | 170   | 34    | ✅ Swept. New in 22. Almost all of it was the legacy `return (x);` idiom                                                            |
| `llvm-prefer-static-over-anonymous-namespace`    | 5     | 2     | ✅ Disabled — style-only, and Google style permits both spellings                                                                   |
| `readability-redundant-typename`                 | 1     | 1     | ✅ Fixed. New in 22 — `src/ra/search.h:626`                                                                                         |

The two mechanical ones are done, and both now measure 0 sites tree-wide.
`readability-redundant-parentheses` was swept with `clang-tidy --export-fixes` over all 840 units, applied with
`clang-apply-replacements --format`. The 170 sites were `return (x);` → `return x;`, `case (KN_ESC):` →
`case KN_ESC:`, and a handful of expression parens (`Stage * (12)`, `x + (WinX)`, `-(MAP_CELL_W)`). Formatting
fallout was one re-wrap (`src/ra/mapeddlg.cc:236`) and two trailing-comment realignments (`src/td/const.cc:200`,
`src/td/menus.cc:304`); nothing else moved. `readability-redundant-typename` was the single `src/ra/search.h:626`
line, valid to drop since C++20 P0634.

`llvm-prefer-static-over-anonymous-namespace` is **disabled** as style-only.
[Google style](https://google.github.io/styleguide/cppguide.html#Internal_Linkage) explicitly permits either spelling
("put it in an unnamed namespace or declare it `static`") and `CLAUDE.md` makes Google style primary, so the check
enforces an LLVM house preference this project has not adopted. All 5 sites are test helpers in
`src/sdllib/font_test.cc` and `src/winvq/vqa32/vqaplay_test.cc`, and both files need their anonymous namespace anyway
— it wraps the fixtures (`VqaPlayTest`) and the `TEST_F` bodies. Satisfying the check would move only the free
helpers out to file-scope `static`, separating them from the fixtures they exist to serve. It would also fire on
every future test helper written the idiomatic way.

`bugprone-unchecked-string-to-number-conversion` is now **disabled**, joining `cert-err34-c` in Tier 3 — it is the
same 200 sites seen tree-wide rather than in a 19-file sample. Three findings decided it:

- **Nothing remote reaches these calls.** All 200 read a local `.INI` (modem and sound-card settings, rules,
  scenario data) or a UI edit box. The 24 in each `teamtype.cc` and every `MPlayerCredits` site in `netdlg.cc` /
  `nulldlg.cc` are `Get_Text()` off a map-editor or lobby gadget, not packet data.
- **The 18 `sscanf` sites cannot be fixed by checking the return value.** `src/ra/session.cc:1007` already reads
  `if (sscanf(buf, "%x", &trap_target) == 1)` and is still flagged: the check's complaint is unreported *overflow*,
  not match failure. Only a rewrite to `strtoul` silences them.
- **Where a bad value would actually hurt, the check aims at the wrong thing.** The hazard is an out-of-range value
  indexing a type array, and `atoi` → `strtol` does not help — a well-formed `"999"` is exactly as dangerous. Range
  checking is the fix, and several sites already do it (`Bound(atoi(credbuf), 0, 9999)`,
  `src/td/nulldlg.cc:4124`).

Fixing all 200 was rejected on that basis, as was routing them through a behaviour-preserving `ParseInt` helper —
that silences the check without changing what the code does, which is worse than saying so here. If the parsing is
ever hardened for real, do it as range validation at the INI and gadget boundary, and re-enable this check afterwards
as the regression guard.

Reproduce with the sweep under [Re-measuring](#re-measuring), substituting these four check names.

---

## TIER 1.5: Small counts, real UB in a deep virtual hierarchy

The `AbstractClass → ObjectClass → TechnoClass → ...` hierarchy makes these more dangerous here than in typical code.

| Check                                        | Sample | Tree-wide | Status                                                                  |
|----------------------------------------------|--------|-----------|-------------------------------------------------------------------------|
| `misc-unconventional-assign-operator`        | 4      | 9         | ✅ Done, `7a9e933b`                                                     |
| `cert-oop58-cpp`                             | 2      | 2         | ✅ Done, `30cb1c6e`                                                     |
| `clang-diagnostic-overloaded-virtual`        | 10     | **28**    | ✅ Done, `bc3c370c` — silently shadowed virtuals, 6 of them live bugs   |
| `cppcoreguidelines-virtual-class-destructor` | 14     | **32**    | ✅ Done, `fdb3a9e8`, with the row below — 26 sites in common            |
| `clang-diagnostic-non-virtual-dtor`          | 12     | **27**    | ✅ Done, `fdb3a9e8`                                                     |
| `clang-analyzer-optin.cplusplus.VirtualCall` | **86** | **81**    | ✅ Done, `e3ad8275` — `final` did most of it, not the feared refactor   |

Tree-wide counts are over all 840 translation units in `cmake-build-strict-ra-clang-22`, deduplicated by site,
measured 2026-08-28. The sample under-predicted all of them except `cert-oop58-cpp`, as it did for Tier 1.

The two destructor checks were done as one commit, as planned: they overlap on 26 sites, and both fire on every class
in a hierarchy whose root lacks a virtual destructor, so 33 sites collapsed to six declarations — `GScreenClass`
covering 11 RA sites and TD's `AbstractTypeClass` covering 13.

**`VirtualCall` was not the refactor this document feared.** Two revisions predicted that clearing it meant
restructuring constructors and destructors across `AbstractClass → ObjectClass → TechnoClass → …`. It did not.
`final` cleared 55 of the 81 sites on its own — the checker stops warning once no override can exist — and the
compiler validates the claim, since `final` on a class someone inherits does not build. Eight more went to
method-level `final` where the class has subclasses but nothing overrides that particular slot. Five were calls
sitting directly in a constructor or destructor, where qualifying the base version only writes down what already
happened. Only four needed real restructuring, and each of those was a place where a derived override genuinely
existed and was being skipped: `GaugeClass`'s constructor, `~GadgetClass`, `DisplayClass`'s view setup, and TD's
`VectorClass` copy constructor. Reach for `final` before reaching for a refactor — and note that qualification is
*not* a general remedy, because the checker flags calls reachable **from** a constructor, not only calls textually
inside one. Qualifying inside `LinkClass::Remove` or `DisplayClass::Set_View_Dimensions` would have broken dispatch
for every ordinary caller.

---

## TIER 2: Valuable, but these *are* the type-migration project

Four of the seven are done. Measured tree-wide over all 840 translation units on 2026-08-28, the tier came to **3,568
sites, not the ~5,700 the estimates said** — and the estimates were wrong in shape as well as size, which is why the
order below is not the order of the counts.

| Check                                                 | Sample | Tree-wide | Status                                                                 |
|-------------------------------------------------------|--------|-----------|------------------------------------------------------------------------|
| `bugprone-implicit-widening-of-multiplication-result` | 29     | **224**   | ✅ Done, `c0045ac4`                                                    |
| `clang-diagnostic-sign-compare`                       | 40     | **305**   | ✅ Done, `35c51bbf`                                                    |
| `clang-diagnostic-shorten-64-to-32`                   | 69     | **439**   | ✅ Done, `cf44023c` — 428 when first measured, before the two above    |
| `clang-diagnostic-switch`                             | 20     | **720**   | ❌ Reclassified, see below — 567 of them are one deliberate idiom      |
| `bugprone-narrowing-conversions`                      | 292    | **1424**  | Next, but read the option note below: 839 after excluding one sub-class |
| `cppcoreguidelines-pro-type-member-init`              | 116    | **349**   | Collides with `NoInitClass`; decide the annotation first               |
| `bugprone-switch-missing-default-case`                | 10     | **118**   | Mechanical, low yield, last                                            |

`src/ra` and `src/td` held 3,243 of the 3,568; the support layers (`sdllib`, `tech`, `winvq`, `base`, `port`) held
325, and `src/port` was clean on all seven. The per-directory strategy below is still right, but the split is that
lopsided.

**`clang-diagnostic-switch` is off the list, at 720 sites.** 567 of them say *"case value not in enumerated type
`KeyNumType`"* and come from `case ButtonKey(200):` — the helper `6a792756` introduced. Clang's `-Wswitch` warns on
any case label that is not a **named enumerator**, and giving `KeyNumType` a fixed underlying type does not change
that; it was tried and measured no different. Clearing them means rewriting 105 switch statements across 43 files to
switch on `static_cast<unsigned>(input)`, which buys silence and nothing else, since those switches were never
enumerator-based. The remaining ~153 are genuine "enumeration value not handled" findings and could be had
per-directory.

**Configure `bugprone-narrowing-conversions` before enabling it.** At its defaults it measures 1424, but 585 of those
are same-width `unsigned` → `signed` — `LEPTON` (`unsigned short`), `COORDINATE` (`uint32_t`) and friends flowing into
`int`, which is exactly what `docs/TYPE_MIGRATION.md`'s do-not-touch list protects. `WarnOnEquivalentBitWidth: false`
drops it to **839** real narrowing sites. Setting that option is better than accepting 585 findings already decided
against.

### What the three finished checks actually cost

**The fixes were type changes, not casts.** `sign-compare` cleared roughly 240 of its 305 sites through eight
declarations, and the recurring cause was not the comparisons: **RA had already been migrated to signed sizes and TD
had not.** `DynamicVectorClass::Count()`, `VectorClass`, `TechnoTypeClass::Level`, the radar geometry — signed in one
port, `size_t`/`unsigned` in the other, which is why TD carried 170 sites against RA's 103. The same held for
`implicit-widening`: six `int` → `base::ssize` stride declarations in `drawbuff.cc` took that file from 23 sites to 8,
and three timer macro definitions cleared 43 more.

**Order matters, and it is sign-compare before truncation.** Making `Count()` return `base::ssize` moved the mismatch
out of comparisons and into assignments, which is precisely what `shorten-64-to-32` catches — so a large share of its
439 sites were created by the commit before it. That is the correct sequence, not an accident, but budget for it: the
truncation count grows as the signedness work lands.

**The layout tests stopped two member retypes.** Widening `td/vector.h`'s `VectorMax` to `base::ssize` changed
`sizeof()` for three serialized types (56→64, 40→48, 1952→1960); narrowing `HouseClass::Tiberium` and `Capacity` to
`int` shrank TD's `HouseClass` from 3368 to 3360. Both are the raw-byte save format. Both members keep their width
with a comment saying why, and their call sites got casts instead. Full RA/TD type parity in those two classes needs a
`SAVEGAME_VERSION` bump, which is a separate decision.

**Do not bulk-wrap expressions using a diagnostic's column.** The last 151 `shorten-64-to-32` sites were wrapped by a
script that read each diagnostic's line and column and inserted a cast around what it judged the expression to be. The
column often points into the middle of an expression rather than at its start, so it produced about 25 malformed edits
— `Array.static_cast<int>(ID(ptr))`, `x static_cast<int>(- y)`, `std::max<static_cast<int>(int>(...)` — all caught by
the compiler, and **one that compiled and changed behaviour**:

```cpp
int graph = kRecordHeight * fixed(kTimerSecond - SpareTicks, kTimerSecond);
int graph = kRecordHeight * static_cast<int>(fixed(kTimerSecond - SpareTicks), kTimerSecond);
```

The inserted paren landed on `fixed()`'s argument separator, turning a two-argument ratio into a one-argument
construction followed by a comma operator, so `graph` became `kTimerSecond`. It compiled because `fixed` has a
one-argument constructor, and only `clang-diagnostic-comma` caught it. Two audits over the other 150 wraps found no
second instance — none has a top-level comma inside a cast, and every changed line reduces to its committed form once
the inserted casts are stripped — but the technique is not worth repeating. Fix the type, or edit the site by hand.

**Strategy — per-directory, not global.** Drop a stricter `.clang-tidy` into directories that are already clean
(`src/base`, `src/port`) and into each directory as it completes type migration. clang-tidy applies the nearest config
file, so new and modernized code is held to Tier 2 while legacy code is not. This is strictly better than
`// NOLINTBEGIN` blocks, which rot silently.

These checks are the enforcement mechanism for `docs/TYPE_MIGRATION.md` — turning one on for a directory is the natural
way to *finish* a migration and keep it finished.

⚠️ **`cppcoreguidelines-pro-type-member-init` conflicts with the `NoInitClass` pattern.** Constructors that deliberately
leave members uninitialized so memcpy'd save data survives will need `// NOLINT` with a comment pointing at `heap.cc`.
Budget for that before enabling it anywhere that serializes.

---

## TIER 3: Keep disabled — high volume, near-zero bug yield

| Check                                            | Sample | Why not                                                  |
|--------------------------------------------------|--------|----------------------------------------------------------|
| `cppcoreguidelines-init-variables`               | 371    | Largest count in the list; almost entirely benign        |
| `performance-enum-size`                          | 168    | Enum storage width is irrelevant at this scale           |
| `bugprone-macro-parentheses`                     | 28     | 1990s macros; fixes risk changing behaviour              |
| `cert-err34-c`                                   | 19     | Unchecked `atoi` in INI parsing; low real-world impact   |
| `bugprone-unchecked-string-to-number-conversion` | 200    | Same sites as `cert-err34-c`, seen tree-wide — see below |
| `llvm-prefer-static-over-anonymous-namespace`    | 5      | Style-only; Google style permits unnamed namespaces      |
| `bugprone-branch-clone`                          | 9      | Intentionally parallel branches throughout game logic    |
| `performance-no-int-to-ptr`                      | 1      | Legacy handle/pointer punning                            |

Plus the standing architectural exclusions, unchanged and still correct:

- **Not applicable to this project:** `altera-*` (3), `android-*` (4), `fuchsia-*` (6), `llvmlibc-*` (4)
- **Would require a rewrite:** `cppcoreguidelines-avoid-non-const-global-variables` (the game *is* globals — see
  `ra/externs.h`), `-no-malloc`, `-owning-memory`, `-avoid-c-arrays`, `-pro-bounds-pointer-arithmetic`,
  `readability-magic-numbers`
- **Style-only:** `readability-braces-around-statements`, `readability-identifier-length`, `llvm-else-after-return`
- **False-positive prone:** `bugprone-easily-swappable-parameters`, `cert-err58-cpp`
- **Cast checks:** `clang-diagnostic-old-style-cast`, `cppcoreguidelines-pro-type-cstyle-cast`,
  `google-readability-casting` — thousands of sites; revisit only per-directory alongside Tier 2

---

## C++ Core Guidelines Type Safety Profile

The [Type Safety Profile](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ss-type) maps directly onto
`cppcoreguidelines-pro-type-*`. Current state:

| Rule   | Description                         | Check                                             | State    |
|--------|-------------------------------------|---------------------------------------------------|----------|
| Type.1 | Don't use `reinterpret_cast`        | `cppcoreguidelines-pro-type-reinterpret-cast`     | Enabled  |
| Type.2 | Don't use `static_cast` to downcast | `cppcoreguidelines-pro-type-static-cast-downcast` | Enabled  |
| Type.3 | Don't use `const_cast`              | `cppcoreguidelines-pro-type-const-cast`           | Enabled  |
| Type.4 | Don't use C-style casts             | `cppcoreguidelines-pro-type-cstyle-cast`          | Disabled |
| Type.5 | Always initialize variables         | `cppcoreguidelines-init-variables`                | Disabled |
| Type.6 | Always initialize member variables  | `cppcoreguidelines-pro-type-member-init`          | Disabled |
| Type.7 | Avoid naked unions                  | `cppcoreguidelines-pro-type-union-access`         | Disabled |
| Type.8 | Avoid varargs                       | `cppcoreguidelines-pro-type-vararg`               | Disabled |

Rules 1–3 are already satisfied tree-wide. Rules 4–8 belong to the Tier 2 per-directory strategy; do not attempt them
globally.

---

## Action Plan

### Phase A — Tier 1 ✅ done

1. ~~Fix `src/ra/house.cc:2367` (`!` → `~`)~~ — done in `f7552ee3`.
2. ~~Un-disable the Tier 1 table~~ — done, one check per commit; see [Progress](#progress).
3. ~~Build `rasdl` **and** `tdsdl`; fix fallout.~~ The `src/td` tail did exceed `src/ra`, by roughly 2:1.

### Phase A.2 — §1.1 ✅ done, §1.2 ✅ done

1. ~~Enable the §1.1 guards in small batches~~ — done in `12f738d9`, `c2121fa4` and `c9bb3fb0`; 24 of 26 enabled, 336
   sites cleared. Batching mattered: `conditional-uninitialized` alone was 103 sites and would have blocked the rest.
2. `bugprone-parent-virtual-call` stays disabled permanently, and `clang-analyzer-optin.cplusplus.VirtualCall` moved to
   Tier 1.5. See [1.1](#11-free-guards--the-name-was-wrong).
3. ~~`bugprone-raw-memory-call-on-non-trivial-type` (§1.2)~~ — enabled in `c580283d` at 0 sites. It turned out to be
   `cert-oop57-cpp` under a new name, and to have no visibility into the save path at all; the real guard is the pair
   of layout tests added in `201cfb3c`. See [1.2](#12-the-check-that-was-not-what-this-document-thought-it-was).

### Phase B — Tier 1.5, in progress

1. ~~Fix `src/ra/vector.h` and `src/tech/listnode.h` first~~ — done in `7a9e933b` and `30cb1c6e`, which is what
   enabling `misc-unconventional-assign-operator` and `cert-oop58-cpp` amounted to.
2. ~~`clang-diagnostic-overloaded-virtual`~~ — done in `bc3c370c`, 28 sites.
3. ~~`cppcoreguidelines-virtual-class-destructor` and `clang-diagnostic-non-virtual-dtor` together~~ — done in
   `fdb3a9e8`, 33 sites in six edits.
4. ~~`clang-analyzer-optin.cplusplus.VirtualCall` (86 sites) last~~ — done in `e3ad8275`, 81 sites, no suppressions.
   The predicted hierarchy refactor did not materialise: see the note under [Tier 1.5](#tier-15-small-counts-real-ub-in-a-deep-virtual-hierarchy).

### Phase C — Tier 2, in progress

Three of the seven went in tree-wide rather than per-directory, because their counts turned out to be in the hundreds
rather than the thousands the estimates predicted:

1. ~~`bugprone-implicit-widening-of-multiplication-result`~~ — done in `c0045ac4`, 224 sites.
2. ~~`clang-diagnostic-sign-compare`~~ — done in `35c51bbf`, 305 sites.
3. ~~`clang-diagnostic-shorten-64-to-32`~~ — done in `cf44023c`, 439 sites.
4. ~~`clang-diagnostic-switch`~~ — reclassified, not enabled; 567 of its 720 sites are a deliberate idiom.
5. `bugprone-narrowing-conversions` next, with `WarnOnEquivalentBitWidth: false` — 839 sites rather than 1424.
6. `cppcoreguidelines-pro-type-member-init` after deciding how the `NoInitClass` constructors get annotated.
7. `bugprone-switch-missing-default-case` last, or never.

The per-directory idea still stands for what remains, but note `src/port` is already clean on all seven checks and
`src/base` had four sites, so the two directories Phase C named as the starting point are worth almost nothing on
their own. `src/ra` and `src/td` hold 91% of the tier.

---

## Re-measuring

Before enabling anything not listed above, measure it — the counts here are the whole basis of the tiering:

```bash
JOBS=$(($(getconf _NPROCESSORS_ONLN) / 2))

# The file list. Take every entry the compile database has that is not a
# dependency, which keeps the 415 header-verification units - they live under
# the build directory, not under src/, so filtering on 'src/' silently drops
# all header coverage (and picks up abseil, whose paths contain '/src/' too).
python3 -c "
import json
d = json.load(open('cmake-build-strict-ra-clang-22/compile_commands.json'))
fs = sorted({e['file'] for e in d
             if '_deps' not in e['file'] and '/third_party/' not in e['file']})
open('/tmp/tidy_files.txt', 'w').write('\n'.join(fs) + '\n')"

# Cost of one or more checks, tallied per check and deduplicated by site,
# since a diagnostic in a shared header repeats once per including TU.
xargs -a /tmp/tidy_files.txt -P $JOBS -I{} clang-tidy \
    -p cmake-build-strict-ra-clang-22 --quiet \
    --checks='-*,check-a,check-b' --warnings-as-errors= {} 2>&1 \
  | grep -oE '^/home[^ ]+ (warning|error): .*\[[a-z0-9-]+' \
  | sed -E 's/^([^ ]+) .*\[([a-z0-9-]+)$/\2\t\1/' | sort -u \
  | cut -f1 | sort | uniq -c | sort -rn
```

Note `--warnings-as-errors=` (empty) to override the config's `WarningsAsErrors: '*'` while measuring.

**A check's own fix can be the next check's finding, and that is fine.** Enabling `sign-compare` created a large
share of `shorten-64-to-32`'s sites, because making a count signed moves the mismatch from the comparison to the
assignment. Sequence the signedness work before the truncation work and expect the second count to grow while you
clear the first; do not read it as a regression.

**Measure the check you are enabling, then sweep the whole config before committing.** Tier 1.5 was verified twice
by sweeping only the check being enabled, and twice the strict build went red afterwards on a check that had never
been measured: marking classes `final` for `VirtualCall` tripped
`clang-diagnostic-unnecessary-virtual-specifier` in 46 declarations, and giving TD's `AbstractTypeClass` a virtual
destructor made `~TeamTypeClass` an override, tripping `clang-diagnostic-suggest-destructor-override` and
`modernize-use-override`. A single-check sweep cannot see fallout that lands on a different check. Run the sweep
below once with the check under study, and once with no `--checks` at all — the full config, all 840 units — before
committing. Both were fixed in `532aef0f`.

Four traps, every one of which reports a clean tree that is not clean.

1. **Confirm the check name exists in the *installed* clang-tidy.** `clang-tidy --checks='-*,name' --list-checks`
   prints `No checks enabled.` for a name it does not know, so a typo or a renamed check measures as zero.
2. **A `clang-diagnostic-*` name on its own enables nothing.** `--checks='-*,clang-diagnostic-overloaded-virtual'`
   prints `Error: no checks enabled.` once per file and reports zero sites — those entries only filter warnings the
   compiler already emits, they do not select work. Always pair them with at least one real check; the sweep above
   works because `cppcoreguidelines-virtual-class-destructor` is in the same list.
3. **Check that `-p` names a directory that still has a `compile_commands.json` with `-Weverything` in it.**
   Without the database clang-tidy falls back to default flags, real checks keep firing, and every
   `clang-diagnostic-*` silently drops to zero — which looks exactly like success. `grep -c -- -Weverything
   <dir>/compile_commands.json` before trusting a zero.
4. **Watch for hard `error:` lines.** A translation unit that fails to compile reports no warnings, which reads
   exactly like a clean one.

---

## Summary

| Tier      | Checks | Sample sites | Status                                                                                   |
|-----------|--------|--------------|------------------------------------------------------------------------------------------|
| 1 (table) | 13     | ~41          | ✅ **Done** — 206 files touched, 18 latent bugs surfaced                                 |
| 1.1       | 26     | 0 (336 real) | ✅ **Done** — 24 enabled (23 after 22), 7 more real bugs; 1 off for good, 1 moved to 1.5 |
| 1.2       | 1      | 0            | ✅ **Done** — free, but it guards nothing; save layout pinned in tests                   |
| 1.5       | 6      | 128 (265 real) | ✅ **Done** — all six; `VirtualCall` alone found lost buffered writes                  |
| 2         | 7      | 576 (3568 real) | 3 done, 1 reclassified; `narrowing-conversions` next                                  |
| 3         | ~230   | —            | Keep disabled                                                                            |

Disabled-check count: **275 → 230**.
