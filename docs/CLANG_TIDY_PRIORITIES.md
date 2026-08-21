# Clang-Tidy Check Prioritization Analysis

## Overview

`.clang-tidy` enables all checks (`'*'`) and then disables **237** of them. This document prioritizes which of those 237
to re-enable, ordered by **measured bug yield per unit of fix effort**.

Unlike the previous revision of this document, the tiers below are not guesses. They come from an actual measurement run
(see [Methodology](#methodology)). Every count in the tables is a real diagnostic site in this repository.

**Tier 1 and §1.1 are complete.** All ten rows of the Tier 1 table (13 checks) are enabled, and 24 of the 26 checks in
§1.1; see [Progress](#progress). The two §1.1 checks left out are deliberate — see
[1.1](#11-free-guards--the-name-was-wrong). The next work is §1.2 and Tier 1.5.

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

| Check                                  | Sample said | Tree-wide actual                     |
|----------------------------------------|-------------|--------------------------------------|
| `clang-diagnostic-writable-strings`    | 9           | **357** diagnostics over 244 lines   |
| `bugprone-suspicious-enum-usage`       | 1           | **617** call sites                   |
| `clang-diagnostic-format`              | 10          | ~160                                 |
| `bugprone-too-small-loop-variable`     | 5           | 36 loops                             |
| `bugprone-suspicious-string-compare`   | 1           | 13                                   |
| §1.1 as a whole                        | 0           | **336** sites across 26 checks        |

§1.1 is the sharpest case: the sample saw zero hits in all 26 checks and the section was written up as free, but three
of them alone (`conditional-uninitialized` 103, `optin.cplusplus.VirtualCall` 86, `implicit-fallthrough` 33) carry 222
sites.

Two effects the sample could not see. First, it contains no `src/td` files, and TD carries a near-duplicate copy of
most of this code — `writable-strings` landed 115 sites in RA against 241 in TD. Second, a single declaration can
carry an unbounded number of sites: `td/globals.cc` alone accounts for 126 of the 357 `writable-strings` diagnostics
because `SerialPacketNames[]` is a 100-entry table of literals. Use the sample to rank checks, not to budget them.

What did hold: every Tier 1 check was cheap *per site*, and each one paid for itself in real bugs.

Last measured: 2026-07-25, clang-tidy 21.1.8. Tier 1 and §1.1 outcomes measured as each check was enabled, through
2026-08-20; the §1.1 counts come from clang-tidy 22 over the full `cmake-build-strict-ra-clang-22` compile database
rather than from the sample.

---

## Status Corrections

An earlier revision of this document marked four checks "✅ DONE" while they were still disabled, and listed three as
deferred that were in fact already enabled. All seven now agree with `.clang-tidy` — the four that were falsely claimed
were enabled for real as part of §1.1:

| Check                                | Once claimed     | Now                                |
|--------------------------------------|------------------|------------------------------------|
| `clang-diagnostic-uninitialized`     | Tier 1 ✅ DONE   | enabled in `c9bb3fb0` (13 sites)   |
| `clang-analyzer-cplusplus.NewDelete` | Tier 1 ✅ DONE   | enabled in `12f738d9`              |
| `clang-diagnostic-self-assign`       | Tier 2.1 ✅ DONE | enabled in `12f738d9`              |
| `misc-redundant-expression`          | Tier 2.1 ✅ DONE | enabled in `12f738d9`              |
| `modernize-use-nullptr`              | Tier 3, "defer"  | was already enabled                |
| `modernize-use-override`             | Tier 3, "defer"  | was already enabled                |
| `readability-else-after-return`      | Tier 4, keep off | was already enabled                |

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

| Check                                                    | Commit     | Outcome                                                                          |
|----------------------------------------------------------|------------|----------------------------------------------------------------------------------|
| `clang-diagnostic-int-in-bool-context`                   | `f7552ee3` | The `house.cc` `Make_Enemy` bug below; that was its only site tree-wide           |
| `bugprone-suspicious-memory-comparison`                  | `aee456e6` | `EventClass::operator==` (padding + union, both games) deleted — it had no callers; `CellClass::Should_Save` never worked at all |
| `clang-analyzer-security.ArrayBound`                     | `2cc33f7a` | 4 real out-of-bounds accesses, incl. `buttons[-1]` in TD's multiplayer menu       |
| `bugprone-suspicious-enum-usage`                         | `6a792756` | 617 sites via a new `ButtonKey()` helper; exposed the Alt+W cheat comparing `KA_W` instead of `KN_W`, so it never fired |
| `bugprone-too-small-loop-variable`                       | `c4571d27` | 36 map loops; `MAP_CELL_W/H/TOTAL` retyped as `CELL` so they cannot regress       |
| `bugprone-signed-char-misuse`                            | `c4f61444` | 4 sign-extension bugs, incl. a truncated PCX RLE run and a read before `IsTranslucent[]` |
| `bugprone-multi-level-implicit-pointer-conversion`       | `f7c93bc9` | `XMP_Is_Small_Prime` bsearching a stack address; `KeyFrameSlots` clearing half its allocation on 64-bit; `Stop_Speaking` never matching |
| `clang-diagnostic-format` (+ `-nonliteral`, `-security`, `-signedness`) | `b1e57f20` | ~160 sites; runtime-only formats now funnel through `Format_Runtime_Text` in `jshell.h` |
| `clang-diagnostic-writable-strings`                      | `dd96637d` | 357 diagnostics; mostly mechanical `const`, but 3 were live writes into string literals |
| `bugprone-suspicious-string-compare`                     | `d58e3977` | 13 sites, all `!= 0`; no behaviour change                                        |

### §1.1

| Checks                                                                     | Commit     | Sites | Outcome                                                                    |
|----------------------------------------------------------------------------|------------|-------|----------------------------------------------------------------------------|
| 18 checks — the cheap end (7 genuinely clean, 11 with 51 sites)             | `12f738d9` | 51    | 4 real defects; `EventClass` now zeroes itself instead of 7 hand-written memsets; TD `TrackNumber`/`TrackIndex`/`Page` retyped `char` → `int` (bumps TD `SAVEGAME_VERSION`) |
| `missing-field-initializers`, `implicit-fallthrough`, `conditional-uninitialized` | `c2121fa4` | 156   | `td/mapsel.cc` frame 23 fell into the frame that blacks out what it just drew; `sdllib/iff.cc` memcpy arguments reversed; `ra/nullmgr.cc` returned an uninitialized `DialStatusType` on modem timeout |
| `clang-diagnostic-uninitialized`, `clang-analyzer-core.CallAndMessage`     | `c9bb3fb0` | 31    | 3 real null dereferences, each in both games (`sidebar.cc`, `display.cc`, `queue.cc`); TD's `NoInitClass` constructors no longer self-init a const member |

Two lessons worth carrying into the next tier:

- **A "mechanical" check is not automatically safe.** `writable-strings` looked like a pure `const` sweep, yet three
  sites were genuinely writing through a literal: `Format_Window_String` rewrites its buffer in place, `TextLabelClass`
  stores the pointer in a non-const member, and TD's serial dialog `strncpy`s into `CallWaitStrings[CALL_WAIT_CUSTOM]`.
  Adding `const` to those would have moved the error, not fixed it. Read what the callee does before const-ing the
  caller.
- **RA and TD diverge.** RA's copy of that same call-waiting loop had already been reworked to `std::string`, so the two
  ports needed opposite fixes. Never assume a fix ports across verbatim.
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
cost **336 sites**. Twenty-four are now enabled and cleared — see [Progress](#progress) for the three commits and what
they turned up.

Enabled, with their measured tree-wide counts:

| Check                                                | Sites |
|------------------------------------------------------|-------|
| `clang-diagnostic-conditional-uninitialized`         | 103   |
| `clang-diagnostic-implicit-fallthrough`              | 33    |
| `clang-diagnostic-missing-field-initializers`        | 20    |
| `clang-analyzer-core.CallAndMessage`                 | 18    |
| `clang-diagnostic-uninitialized`                     | 13    |
| the other 19, together                               | 51    |

The remaining 19 are `bugprone-unhandled-self-assignment`, `bugprone-sizeof-expression`,
`bugprone-redundant-branch-condition`, `bugprone-inc-dec-in-conditions`,
`bugprone-unchecked-string-to-number-conversion`, `clang-diagnostic-self-assign`,
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

### 1.2 Single most valuable check in the whole list

**`bugprone-raw-memory-call-on-non-trivial-type`** — 0 hits today, so it costs nothing to enable.

`TFixedIHeapClass::Save/Load` in `src/ra/heap.cc` serializes game objects by **raw memcpy**, followed by a placement-new
with `NoInitClass()`. That contract silently breaks the moment anyone adds a `std::string`,
`std::optional`, or any non-trivially-copyable member to a serialized type — and it breaks as save-file corruption or a
multiplayer desync, not as a compile error. This check is the tripwire for exactly that mistake.

---

## TIER 1.5: Small counts, real UB in a deep virtual hierarchy

The `AbstractClass → ObjectClass → TechnoClass → ...` hierarchy makes these more dangerous here than in typical code.

| Check                                        | Sample | Notes                                                  |
|----------------------------------------------|--------|--------------------------------------------------------|
| `clang-diagnostic-overloaded-virtual`        | 10     | silently shadowed virtuals — behaviour bugs, not style |
| `cppcoreguidelines-virtual-class-destructor` | 14     | polymorphic delete through base                        |
| `clang-diagnostic-non-virtual-dtor`          | 12     | overlaps the above                                     |
| `misc-unconventional-assign-operator`        | 4      | `ra/vector.h:43,84,85` — `virtual operator=`           |
| `cert-oop58-cpp`                             | 2      | `tech/listnode.h:56,59` — copy ctor mutates its source |
| `clang-analyzer-optin.cplusplus.VirtualCall` | **86** | moved down from §1.1; needs ctor/dtor restructuring    |

`vector.h` and `listnode.h` are foundational; fixing those two files clears most of this tier.

---

## TIER 2: Valuable, but these *are* the type-migration project

High counts, genuine latent bugs mixed with thousands of benign sites. Enabling any of these globally halts the build
until ~3,000 sites are fixed.

| Check                                                 | Sample | Tree-wide est. |
|-------------------------------------------------------|--------|----------------|
| `bugprone-narrowing-conversions`                      | 292    | ~2900          |
| `cppcoreguidelines-pro-type-member-init`              | 116    | ~1160          |
| `clang-diagnostic-shorten-64-to-32`                   | 69     | ~690           |
| `clang-diagnostic-sign-compare`                       | 40     | ~400           |
| `bugprone-implicit-widening-of-multiplication-result` | 29     | ~290           |
| `clang-diagnostic-switch`                             | 20     | ~200           |
| `bugprone-switch-missing-default-case`                | 10     | ~100           |

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

| Check                              | Sample | Why not                                                |
|------------------------------------|--------|--------------------------------------------------------|
| `cppcoreguidelines-init-variables` | 371    | Largest count in the list; almost entirely benign      |
| `performance-enum-size`            | 168    | Enum storage width is irrelevant at this scale         |
| `bugprone-macro-parentheses`       | 28     | 1990s macros; fixes risk changing behaviour            |
| `cert-err34-c`                     | 19     | Unchecked `atoi` in INI parsing; low real-world impact |
| `bugprone-branch-clone`            | 9      | Intentionally parallel branches throughout game logic  |
| `performance-no-int-to-ptr`        | 1      | Legacy handle/pointer punning                          |

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

### Phase A.2 — §1.1 ✅ done, §1.2 still open

1. ~~Enable the §1.1 guards in small batches~~ — done in `12f738d9`, `c2121fa4` and `c9bb3fb0`; 24 of 26 enabled, 336
   sites cleared. Batching mattered: `conditional-uninitialized` alone was 103 sites and would have blocked the rest.
2. `bugprone-parent-virtual-call` stays disabled permanently, and `clang-analyzer-optin.cplusplus.VirtualCall` moved to
   Tier 1.5. See [1.1](#11-free-guards--the-name-was-wrong).
3. **Still open:** `bugprone-raw-memory-call-on-non-trivial-type` (§1.2). Still 0 hits, still the highest-value item in
   this document — it is the only automated guard on the `TFixedIHeapClass::Save/Load` memcpy contract.

### Phase B — Tier 1.5

Fix `src/ra/vector.h` and `src/tech/listnode.h` first, then enable the five original checks and clear the remainder.
`clang-analyzer-optin.cplusplus.VirtualCall` (86 sites) belongs to this phase and should go last, since it is the one
that actually changes constructor and destructor behaviour.

### Phase C — Tier 2, per directory

1. Add a stricter `.clang-tidy` to `src/base` and `src/port` (already clean).
2. Extend it to each directory as type migration completes there.
3. Never enable Tier 2 at the repository root.

---

## Re-measuring

Before enabling anything not listed above, measure it — the counts here are the whole basis of the tiering:

```bash
# Cost of a single check, tree-wide:
clang-tidy -p cmake-build-strict-ra-clang --quiet \
    --checks='-*,the-check-name' --warnings-as-errors= src/ra/*.cc 2>/dev/null \
  | grep -c 'warning:\|error:'

# Cost of several at once, tallied per check (dedup by site, since headers repeat):
clang-tidy -p cmake-build-strict-ra-clang --quiet \
    --checks='-*,check-a,check-b' --warnings-as-errors= <files> 2>/dev/null \
  | grep -oE '^[^ ]+:[0-9]+:[0-9]+: (warning|error): .*\[[a-zA-Z0-9_.,-]+\]$' \
  | sed -E 's/^([^ ]+): (warning|error): .*\[([^]]+)\]$/\3\t\1/' | sort -u \
  | cut -f1 | sort | uniq -c | sort -rn
```

Note `--warnings-as-errors=` (empty) to override the config's `WarningsAsErrors: '*'` while measuring.

---

## Summary

| Tier      | Checks | Sample sites | Status                                                       |
|-----------|--------|--------------|--------------------------------------------------------------|
| 1 (table) | 13     | ~41          | ✅ **Done** — 206 files touched, 18 latent bugs surfaced      |
| 1.1       | 26     | 0 (336 real) | ✅ **Done** — 24 enabled, 7 more real bugs; 1 off for good, 1 moved to 1.5 |
| 1.2       | 1      | 0            | Next — cheapest and highest-value item left                  |
| 1.5       | 6      | 128          | Enable after fixing `vector.h` / `listnode.h`; `VirtualCall` last |
| 2         | 7      | 576          | Per-directory only, tied to type migration                   |
| 3         | ~230   | —            | Keep disabled                                                |

Disabled-check count: **275 → 237**.
