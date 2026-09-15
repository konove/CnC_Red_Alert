# Plan: enable `modernize-avoid-c-style-cast`, `google-readability-casting` and `clang-diagnostic-old-style-cast`

Written 2026-09-15 against `.clang-tidy` and clang-tidy 23.1.2. Companion to the three rows in
[CLANG_TIDY_PRIORITIES.md](CLANG_TIDY_PRIORITIES.md) and the follow-up to
[CSTYLE_CAST_PLAN.md](CSTYLE_CAST_PLAN.md).

**Status: complete (2026-09-15).** The three checks are enabled; the results, including the 58
macro-expansion casts that only `-Wold-style-cast` reported, are in the "Numeric cast enablement"
review in CLANG_TIDY_PRIORITIES.md.

## Context

The cast-qual and `pro-type-cstyle-cast` work removed every C-style cast that dropped `const`,
punned pointers or downcast. What remains is the numeric subset: `(int)enumerator`, `(HousesType)n`,
`(bool)n`, plus a few pointer casts the type-unsafe check does not classify as unsafe (upcasts,
`void*` conversions, casts to the operand's own type). `CLAUDE.md` already says "Do not use C-style
casts like `(int)value`"; these three names are the checks that enforce that sentence.

The rows stayed skipped because `docs/TYPE_MIGRATION.md` made cast replacement opportunistic and
ruled out a codebase-wide hunt. That rule was written against 1,393 sites. The cast series since
then removed most of them as a side effect, and a fresh isolated sweep on 2026-09-15 (486 unique
translation units from the strict compile database) reports **416 unique sites**:

| Message                                                                    | Sites |
| -------------------------------------------------------------------------- | ----- |
| C-style casts are discouraged; use static_cast                             | 346   |
| C-style casts are discouraged; use static_cast/const_cast/reinterpret_cast | 52    |
| C-style casts are discouraged; use reinterpret_cast                        | 6     |
| C-style casts are discouraged; use static_cast (if needed, redundant)      | 6     |
| redundant cast to the same type                                            | 6     |

| Module | Sites |
| ------ | ----- |
| ra     | 225   |
| td     | 185   |
| tech   | 6     |

Largest files: `td/mapedit.cc` 85, `ra/mapedit.cc` 76, `ra/jshell.h` 19, `td/debug.cc` 18,
`ra/wol_gsup.cc` 16, `ra/wolapiob.cc` 15, `ra/debug.cc` 13, `td/defines.h` 12.

`clang-diagnostic-old-style-cast` is the compiler's view of the same rule. It reports nothing today
only because the clang strict flag set passes `-Wno-old-style-cast`; enabling the name means
dropping that flag, after which `WarningsAsErrors: '*'` makes every old-style cast a build error.
The GCC strict set already has `-Wold-style-cast`.

## How the checks classify (read before fixing)

- The two tidy names are aliases: every site prints both. `WarningsAsErrors: '*'` applies to the
  isolated sweep, so findings are `error:` lines.
- Both report functional casts of the form `int(x)` as well as `(int)x`.
- Fix-its exist for the `static_cast` message only (`static_cast<T>(x)`); the
  `static_cast/const_cast/reinterpret_cast`, `reinterpret_cast` and redundant messages are by hand.
- `cppcoreguidelines-pro-type-static-cast-downcast` and `-reinterpret-cast` are enforced, so a
  downcast written with a C-style cast must become `dynamic_cast` or disappear, never `static_cast`.

## Fix strategy by group

Site counts are from the sweep; a line with several casts counts each.

| Group                                                                                                                                                                                                                                                        | Sites | Fix                                                                                                                                                                                                                                                                                                                                       |
| ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ----- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **A. Key-number case labels**: `case (int)KN_UP \| (int)KN_ALT_BIT:` in both map editors and debug key handlers, left by the switch-fallback commit                                                                                                          | 196   | Drop the casts: `case KN_UP \| KN_ALT_BIT:`. `jshell.h`'s `constexpr operator\|` combines the enumerators and the label converts to the `int` the switch already uses; `KN_LMOUSE \| KN_RLSE_BIT` in the two dialog compares likewise.                                                                                                    |
| **B. Enum arithmetic helpers**: `(T)((int)a + 1)` in `ra/jshell.h`, `(int)a` in `td/jshell.h`, facing and direction arithmetic in `ra/face.h`, `td/defines.h`, `ra/facing.h`                                                                                 | ~45   | `static_cast<T>` / `static_cast<int>`; the templates use `static_cast<int>` so they still accept the non-enum instantiations.                                                                                                                                                                                                             |
| **C. Numeric and enum conversions**: `(HousesType)(...)`, `(unsigned)`, `(uint32_t)`, `(DirType)`, `(int16_t)`, `(PlayerColorType)`, `int(MPH_LIGHT_SPEED)` and the rest                                                                                     | ~110  | The check's fix-its (`static_cast<T>`), applied with `clang-apply-replacements` and reviewed.                                                                                                                                                                                                                                             |
| **D. `(bool)ParseInteger(...)`** in the WOL setup code                                                                                                                                                                                                       | 8     | `!= 0`.                                                                                                                                                                                                                                                                                                                                   |
| **E. Redundant casts**: `(int)ttype.SightRange` on an `int`, `(ObjectClass*)Next` on an `ObjectClass*`, `(UnitClass*)this`, `(char*)FileDigest` on `char[32]`, `(unsigned char*)CurrentPalette` on `unsigned char[]`                                         | ~16   | Delete the cast.                                                                                                                                                                                                                                                                                                                          |
| **F. Pointer and reference conversions the unsafe check let through**: COM upcasts `(IChatEvent*)this`, `(void*)pUser` into `void*` list data, `(const NodeElement*)bsearch(...)`, `((ObjectClass&)*this).Mark(...)`, `(TDropListClass&)EditClass::Add(...)` | ~20   | `static_cast` for upcasts, `void*` and `void*`-returning calls; the `Mark` calls name the base explicitly, `ObjectClass::Mark`, only if that is what the cast did (it is not: `Mark` is virtual, so the cast changed nothing), else drop the cast; `TDropListClass::Add` and `Remove` use `dynamic_cast` as `DropListClass` already does. |

## Steps

Each step is one commit; each commit passes the isolated sweep on its files, and the series ends
with the strict build, CTest and both smoke scripts.

1. **Key labels (A).** Drop the `(int)` from the `KN_` case labels and compares.
2. **Hand fixes (B, D, E, F).** Templates and facing helpers, the `bool` parses, the redundant and
   pointer casts.
3. **Fix-its (C).** Re-run the sweep with `--export-fixes`, apply with `clang-apply-replacements`,
   `git clang-format`, review the diff, re-sweep to zero.
4. **Enable.** Remove the three names from `.clang-tidy` and `-Wno-old-style-cast` from
   `CMakeLists.txt`; full strict rebuild; set the rows to **Enabled**; add a "Numeric cast
   enablement (date)" review to the priorities file; update `docs/TYPE_MIGRATION.md` so the cast
   section describes the enforced rule instead of the opportunistic one.

## Verification

- **Isolated sweep** (~1.5 min), per the tree-wide fix-it recipe over the unique `src/` files in
  `cmake-build-strict-ra-clang/compile_commands.json`:

  ```sh
  xargs -P 10 -I{} sh -c 'clang-tidy -p cmake-build-strict-ra-clang --quiet \
      --extra-arg=-Wold-style-cast \
      --checks="-*,modernize-avoid-c-style-cast,clang-diagnostic-old-style-cast" "{}" 2>/dev/null' \
      < tus.txt > report.txt
  grep -c 'error:' report.txt   # target: 0
  ```

- **Probe**: a scratch file with `(int)x` must fail under the enabled configuration.
- **Full strict build** in the foreground
  (`timeout 590 cmake --build cmake-build-strict-ra-clang --parallel 10 -- -k 0`, repeated until no
  work), then CTest.
- **Smoke**: `tools/ra_saveload_smoke.sh` and `tools/td_saveload_smoke.sh` with the `--team`,
  `--building`, `--mobile`, `--map` and `--globals` fixtures. The map editors, debug key handlers
  and WOL dialogs are not on the smoke path; those stay a manual check.
