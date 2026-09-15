# Plan: enable the three remaining P1 checks

Written 2026-09-15 against `.clang-tidy` and clang-tidy 23.1.2. Companion to the
`bugprone-unhandled-code-paths`, `abseil-unchecked-statusor-access` and
`clang-analyzer-optin.core.EnumCastOutOfRange` rows in
[CLANG_TIDY_PRIORITIES.md](CLANG_TIDY_PRIORITIES.md).

**Status: complete (2026-09-15).** The three checks are enabled; the results are in the "Remaining
P1 enablement" review in CLANG_TIDY_PRIORITIES.md. `KeyNumType` turned out not to be a flag enum
(`-Wflag-enum` rejects its key codes) and got its own operators instead; the direction helper is
`AsDirection`, since TD already has `Direction(COORDINATE, COORDINATE)`; `ShapeFlags_Type`, the WSA
open flags and the modem status bits were unnamed enums the first count missed.

## Context

All three were skipped on 2026-09-11, and each reason has since changed. A fresh isolated sweep of
the 486 source translation units on 2026-09-15:

| Check                                          | Then                                                  | Now                                                                                                   |
| ---------------------------------------------- | ----------------------------------------------------- | ----------------------------------------------------------------------------------------------------- |
| `bugprone-unhandled-code-paths`                | 61 switches without a `default`                       | 0 reports and no crash: the switch-fallback work gave every switch a `default`                        |
| `abseil-unchecked-statusor-access`             | LLVM 23.1.2 crashes even on checked access            | still crashes, but only on the two PCX readers, which are the only `StatusOr` users left in the tree  |
| `clang-analyzer-optin.core.EnumCastOutOfRange` | 45 locations; flag combinations and continuous angles | 27 locations: 274 messages, 246 of them the `jshell.h` flag operators instantiated for six flag enums |

## Design

**Unhandled code paths.** Nothing to fix; remove the exclusion. `WarnOnMissingElse` stays at its
default of off.

**StatusOr.** The PCX byte reader's `absl::StatusOr<uint8_t> ReadByte()` is the construct the
dataflow model crashes on (`getSyntheticFields` in `runTypeErasedDataflowAnalysis`). Its callers
only ever test `.ok()`; the error string is never read. `std::optional<uint8_t>` says the same
thing, drops the Abseil status dependency from both games, and leaves no `StatusOr` in the tree, so
the check enforces the rule for whatever uses it next.

**Enum cast range.** The checker accepts any cast to an enum marked `[[clang::flag_enum]]`, probed
on 2026-09-15: zero, a combination, a mask and `~` all pass; GCC rejects the bare attribute under
`-Werror=attributes`, so `base/attributes.h` wraps it in `CNC_FLAG_ENUM` behind
`__has_cpp_attribute`. The six enums the flag operators are instantiated for get the mark:
`TextPrintType`, `ThreatType` (both games), the gadget `FlagEnum` (both games), RA's `AttachType`,
`KeyNumType` and `GBC_Enum`. That removes every report at the `jshell.h` operators, the `keyboard.h`
button-key builder, the two text point-size masks and the two zero gadget flags.

`DirType` is a 256-step direction with only the compass points named; every value is valid, which
the checker cannot express. A `constexpr DirType Direction(int angle)` next to each game's
definition wraps the angle to the circle and is the one place that converts a computed integer to
the type, carrying the one `NOLINT` and the explanation. The eleven flagged computations
(`Desired_Facing*`, the nuclear launch angles, the aircraft search, the debug random directions, the
harvester unloading adjustment) and both games' `DirType` arithmetic operators use it. The ~800
`static_cast<DirType>` in the data tables are constant initializers the analyzer never runs and stay
as they are.

`FacingType` `-2` is the path optimizer's removed-command marker, already named `kEmptyCommand` two
lines above the table that still spells it as a cast; the table uses the name.

## Steps

1. `base/attributes.h`, the flag marks, `Direction()`, the path tables, the PCX readers, the CMake
   link lists.
2. Remove the three names from `.clang-tidy`; isolated sweep of the three checks over every
   translation unit; full strict build; CTest; both smoke scripts; rows to **Enabled**; a "Remaining
   P1 enablement (date)" review.

## Verification

```sh
xargs -P 10 -I{} sh -c 'clang-tidy -p cmake-build-strict-ra-clang --quiet \
    --checks="-*,bugprone-unhandled-code-paths,abseil-unchecked-statusor-access,clang-analyzer-optin.core.EnumCastOutOfRange" "{}" 2>&1 \
    | grep -E "error:|warning:|Stack dump"' < tus.txt   # target: nothing
```

Then the strict build in the foreground, CTest, and `tools/ra_saveload_smoke.sh` plus
`tools/td_saveload_smoke.sh` with the `--team`, `--building`, `--mobile`, `--map` and `--globals`
fixtures. The PCX reader runs on the title screen, not the smoke path; the `port_test` and
`td_saveload_test` binaries cover the helpers.
