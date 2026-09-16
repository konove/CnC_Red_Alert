# Plan: enable `cppcoreguidelines-init-variables`

Written 2026-09-15 against `.clang-tidy` and clang-tidy 23.1.2. Companion to the row in
[CLANG_TIDY_PRIORITIES.md](CLANG_TIDY_PRIORITIES.md).

**Status: complete (2026-09-15).** The check is enabled; the results are in the "Local
initialization enablement" review in CLANG_TIDY_PRIORITIES.md.

## Context

The row was skipped on 2026-09-12 because the check's own fix-it (`int x = 0;`) satisfies it while
hiding the one thing the eleven enabled uninitialized-read checks catch, a path that forgot to
assign. That argument is against the fix-it, not against the rule. Google style asks to declare
variables as close to first use as possible and initialize them in the declaration, and Core
Guidelines ES.20 and ES.21 say the same, and the 2026-09-12 review itself named declaring each local
at its first use as "the valuable version of this cleanup". This plan does that version and uses the
fix-it only where the declaration genuinely has to precede the branches that assign it.

A fresh isolated sweep on 2026-09-15 reports **2,887 sites** in 236 files (the 2026-09-12 count of
2,998 included the generated header-check units). 1,433 are in Tiberian Dawn, 1,050 in Red Alert,
the rest in `winvq`, `sdllib` and `tech`. Every one is the same construct: a local declared at the
top of a block and assigned before it is read.

| Fix-it the check offers | Sites |
| ----------------------- | ----- |
| ` = 0`                  | 1,790 |
| ` = nullptr`            | 703   |
| ` = false`              | 82    |
| none (enum types)       | 312   |

## Design

Three tiers, tried in this order for every site.

**1. Sink the declaration to its first use (1,878 sites).** `tools/sink_declarations.py` reads the
sites from the check's `--export-fixes` YAML, which gives the exact byte offset of each declared
name, finds the declaration statement and the first later mention of the name (comments, strings and
member accesses of the same spelling do not count), and moves the declaration when:

- the first mention is an unconditional `name = expr;` statement in the same block, which becomes
  `Type name = expr;` (the previous token is `;`, `{` or `}`, so `if (c) x = 1;`, `else x = 1;`,
  `case 1: x = 1;` and `(x = f())` are all refused);
- or that statement sits in a nested block and no later mention appears after the block closes, so
  the declaration moves into the block (a loop body assigning before reading each iteration, a
  branch that both sets and consumes the value);
- or every later mention is the init of a `for (name = ...; ...)` header, in which case each header
  declares its own loop variable.

It refuses when the name appears in the assigned expression, when a preprocessor line, `goto` or
case label lies between declaration and sink, when a case label follows the sink in the same block
(the initialization would be jumped over), and when the `for` init has a comma. Multi-declarator
statements (`int x, y;`) are split one per line and the trailing comment follows the moved
declaration.

**2. The check's fix-it (881 sites).** Variables assigned in more than one branch or loop and read
afterwards, out-parameters passed by address (`Map.Input(input, x, y)`, `ReadObject(house)`), and
declarations separated from their first use by `#if` keep their position and get ` = 0`,
` = nullptr` or ` = false`. This is the honest form for those: all paths assign, the analyzer
confirms nothing reads the placeholder, and the placeholder documents that the value comes later.

**3. By hand (128 sites).** Enum-typed locals get no fix-it. Each takes its type's sentinel where
one exists (`KN_NONE`, `HOUSE_NONE`, `FACING_NONE`, `SMUDGE_NONE`, `VOC_NONE`, `MISSION_NONE`,
`THEME_NONE`, `SOURCE_NONE`, `OVERLAY_NONE`, `REMAP_NONE`, `SCEN_*_NONE`, `STRUCT_NONE` and the
other type-ID enums, `EV_NONE`, `RC_NORMAL`, `kTargetNone`; `DIR_N` for directions, which have no
sentinel; RA's `RejectType` gains `REJECT_NONE = -1` for the out-parameter of `Get_Join_Responses`).
Where an `if`/`else` only picks the value, it becomes a `const` ternary (`faceto`, `pref_house`, the
edit-box `flags`, the radar `style`); the nested pickers in each game's `textbtn.cc` become
immediately invoked lambdas returning the style. Sinks that land inside a `switch` case get braces
around the case body.

**Cascades.** Sunk declarations trip five more enabled checks, all fixed in the same pass:
`misc-const-correctness` (most sunk locals are never modified again), `modernize-use-auto` (a sunk
`T* p = static_cast<T*>(...)` or `= new T`), `modernize-loop-convert` (a `for` that now declares its
own index over an array becomes a range-for; its fix-it spells `short`/`long` for
`int16_t`/`int64_t` arrays, rewritten by hand), `readability-redundant-nested-if` (an outer `if`
whose only content was a declaration plus an inner `if`), `clang-diagnostic-shadow` (a range-for
element named like a field) and `cppcoreguidelines-prefer-member-initializer` (a constructor
assignment that no longer has a loop in front of it).

## Steps

1. Export the sites: an isolated `--export-fixes` sweep of the check over every translation unit,
   merged into one JSON list keyed by file and offset. **Key the per-unit output by path, not by
   basename**: `ra/unit.cc` and `td/unit.cc` overwrite each other otherwise, which silently dropped
   1,077 Red Alert sites from the first pass here.
2. `tools/sink_declarations.py --sites sites.json --fallback fallback.json --fixits`, then
   `git clang-format -f`.
3. Re-sweep the check alone: it doubles as the compile check. Fix the no-fix-it fallbacks by hand.
4. Full check set over the changed units, `--export-fixes`, `clang-apply-replacements`, repeat until
   quiet (three rounds: const, auto and loop-convert cascade into each other).
5. Remove the name from `.clang-tidy`; strict build; CTest; both smoke scripts with every fixture.

## Verification

```sh
xargs -P 10 -I{} sh -c 'n=$(echo "{}" | sed "s|.*/src/||; s|/|_|g"); \
    clang-tidy -p cmake-build-strict-ra-clang --quiet --checks="-*,cppcoreguidelines-init-variables" "{}" \
    > "$OUT/$n.log" 2>/dev/null' < tus.txt   # target: no error: lines in any log
```

Then the strict build in the foreground, CTest, `tools/ra_saveload_smoke.sh` (plain and
`--load-fixture`) and `tools/td_saveload_smoke.sh` with every fixture.
