# Clang-Tidy Check Prioritization Analysis

## Overview

`.clang-tidy` enables all checks (`'*'`) and then disables **275** of them. This document prioritizes which of those 275
to re-enable, ordered by **measured bug yield per unit of fix effort**.

Unlike the previous revision of this document, the tiers below are not guesses. They come from an actual measurement run
(see [Methodology](#methodology)). Every count in the tables is a real diagnostic site in this repository.

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

Last measured: 2026-07-25, clang-tidy 21.1.8.

---

## Status Corrections

The previous revision of this document marked several checks "✅ DONE" that are still disabled, and listed several as
deferred that are in fact already enabled. Verified against the current `.clang-tidy`:

| Check                                | Previously claimed | Actual state        |
|--------------------------------------|--------------------|---------------------|
| `clang-diagnostic-uninitialized`     | Tier 1 ✅ DONE     | **still disabled**  |
| `clang-analyzer-cplusplus.NewDelete` | Tier 1 ✅ DONE     | **still disabled**  |
| `clang-diagnostic-self-assign`       | Tier 2.1 ✅ DONE   | **still disabled**  |
| `misc-redundant-expression`          | Tier 2.1 ✅ DONE   | **still disabled**  |
| `modernize-use-nullptr`              | Tier 3, "defer"    | **already enabled** |
| `modernize-use-override`             | Tier 3, "defer"    | **already enabled** |
| `readability-else-after-return`      | Tier 4, keep off   | **already enabled** |

Genuinely done and confirmed enabled: `clang-diagnostic-suggest-override`, `-return-stack-address`,
`-mismatched-new-delete`, `-delete-incomplete`, `-sometimes-uninitialized`, `-unused-variable/-function/-parameter`,
`clang-analyzer-core.NullDereference`, `clang-analyzer-core.uninitialized.*`, `clang-analyzer-deadcode.DeadStores`,
`clang-analyzer-unix.MismatchedDeallocator`, `bugprone-not-null-terminated-result`,
`readability-redundant-control-flow`, `readability-redundant-declaration`.

---

## TIER 1: Enable now — real bugs, negligible volume

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
caught by `clang-diagnostic-int-in-bool-context`, which is currently disabled and has exactly one hit in the entire
sample.

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
| `clang-diagnostic-writable-strings`                      | 9      | string literal → `char*`; mechanical `const` fix                                                                      |
| `bugprone-suspicious-string-compare`                     | 1      | `stricmp` result used without comparison (`scenario.cc:1923`)                                                         |

Total fix surface: **~41 sites in the sample**, realistically a few hundred tree-wide, most of them one-line changes.

### 1.1 Free guards — zero hits, zero fix cost

Clean across the entire 10% sample. Enable them to prevent the class of defect from *entering*:

`bugprone-unhandled-self-assignment`, `bugprone-sizeof-expression`, `bugprone-parent-virtual-call`,
`bugprone-redundant-branch-condition`, `bugprone-inc-dec-in-conditions`,
`bugprone-unchecked-string-to-number-conversion`, `clang-diagnostic-uninitialized`,
`clang-diagnostic-conditional-uninitialized`, `clang-diagnostic-self-assign`, `clang-diagnostic-implicit-fallthrough`,
`clang-diagnostic-char-subscripts`, `clang-diagnostic-null-arithmetic`, `clang-diagnostic-logical-not-parentheses`,
`clang-diagnostic-format-security`, `clang-diagnostic-varargs`, `clang-diagnostic-missing-field-initializers`,
`misc-redundant-expression`, `misc-definitions-in-headers`, `readability-misleading-indentation`, `cert-oop57-cpp`,
`clang-analyzer-core.CallAndMessage`, `clang-analyzer-cplusplus.NewDelete`,
`clang-analyzer-optin.cplusplus.VirtualCall`,
`clang-analyzer-optin.cplusplus.UninitializedObject`, `clang-analyzer-unix.Stream`, `clang-analyzer-security.VAList`.

Caveat: zero in 10% of the tree is not zero tree-wide. Expect a small tail, especially from `src/td`.

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

### Phase A — Tier 1 (do now)

1. Fix `src/ra/house.cc:2367` (`!` → `~`) — it is a bug regardless of tooling.
2. Un-disable the Tier 1 table + §1.1 free guards + §1.2 in `.clang-tidy`.
3. Build `rasdl` **and** `tdsdl`; fix fallout. Expect the `src/td` tail to exceed the `src/ra` count.
4. Re-run the sample command to confirm a clean pass.

### Phase B — Tier 1.5

Fix `src/ra/vector.h` and `src/tech/listnode.h` first, then enable the five checks and clear the remainder.

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

| Tier | Checks | Sample sites | Action                                        |
|------|--------|--------------|-----------------------------------------------|
| 1    | ~36    | ~41          | Enable now — confirmed bugs, cheap fixes      |
| 1.5  | 5      | 42           | Enable after fixing `vector.h` / `listnode.h` |
| 2    | 7      | 576          | Per-directory only, tied to type migration    |
| 3    | ~230   | —            | Keep disabled                                 |
