# Strict Build Performance Plan

Measured 2026-09-19 with LLVM 23.1.2 on the flags of `build-strict` (RelWithDebInfo). Each pass was
run alone, outside ninja, ccache and ctcache, by invoking the compiler, IWYU and clang-tidy directly
with each TU's command from `compile_commands.json`. The tree-wide sums were taken on a loaded
machine and are good to about ±15%.

`build-strict` and CLion's "Strict RA Clang" dir run three passes per translation unit, one after
the other in the same ninja job: IWYU, clang-tidy (538 checks, 131 of them `clang-analyzer-*`), then
the clang compile. A one-file edit costs 15–60 s of wall time. A header edit re-runs all three
passes for hundreds of TUs at `-j14`, which is memory-bound at ~1 GB per clang-tidy job. ccache and
ctcache don't help the edit loop, because a changed TU always misses. Each state is also analyzed
twice, once in each strict dir, because their flags differ.

## Findings

Per TU, in seconds; the analyzer columns include the ~3 s parse:

| TU                   | compile | IWYU | tidy w/o analyzer | analyzer deep | analyzer shallow |
| -------------------- | ------: | ---: | ----------------: | ------------: | ---------------: |
| ra/ioobj.cc          |     4.3 |  3.0 |               8.2 |          61.8 |              4.1 |
| ra/techno.cc         |     4.2 |  3.2 |               5.6 |          15.6 |              6.9 |
| td/init.cc           |     3.6 |  2.4 |          **21.9** |           6.2 |                – |
| tech/fixed_test.cc   |     1.7 |  1.3 |               1.6 |          11.4 |              1.8 |
| ra/iomap.cc (median) |     3.4 |  2.7 |               4.5 |           8.9 |              4.1 |

Tree-wide totals over 549 project TUs, in CPU-s:

- compile: 1102
- IWYU: 948. 393 of 549 TUs print suggestions nobody acts on; CMake ignores IWYU's exit code.
- tidy without the analyzer: 2211. That includes ~1000 of parsing and 890 of matchers.
  `bugprone-unchecked-optional-access` accounts for 130 of the matcher time, almost all of it in 10
  TUs: any call on a `std::optional` makes it run its dataflow over the whole enclosing function,
  and the dialog/init functions are thousands of lines long.
- the analyzer's own cost: deep ≈ 1080, shallow ≈ 540.

A full pass costs ≈ 5340 CPU-s, and its long tail is ioobj.cc at ~63 s. After this plan it should
cost ≈ 3650 CPU-s (−32%), with the slowest TU at ~16 s. Expected per-edit wall time:

| TU            |  Now | After |
| ------------- | ---: | ----: |
| techno.cc     | 24 s | ~13 s |
| ioobj.cc      | 63 s | ~13 s |
| td/init.cc    | 31 s |  ~9 s |
| fixed_test.cc | 14 s |  ~4 s |

When the other strict dir has already analyzed the same state, the clang-tidy pass is skipped
entirely.

## Plan

Decisions (2026-09-19):

- Shallow analyzer locally, deep in CI.
- IWYU opt-in everywhere.
- Rewrite all `ParseInteger(…).value_or(…)` sites.
- Drop duplicate check aliases.
- Share ctcache across the strict dirs, with both dirs on RelWithDebInfo. `-O0` is dropped: it saves
  only 0.25–0.45 s/TU, and it changes preprocessing (`__OPTIMIZE__`), which would break the sharing.

One commit per step, RA/TD split where both change.

### 1. IWYU becomes opt-in

- `cmake/IWYU.cmake`: `ENABLE_IWYU` defaults to `OFF` instead of following `STRICT_CHECKS`.
  `misc-include-cleaner` already enforces `.cc` includes as an error, and IWYU re-parses every TU
  with its own clang for advice nobody acts on.
- CI lint job: drop `iwyu` from the apt line and from the step name and header comment.
- `build-strict`: reconfigure with `-DENABLE_IWYU=OFF`.
- Docs: `CLAUDE.md`, `.claude/commands/commit.md`, `docs/CLANG_TIDY_PRIORITIES.md`.

### 2. Fix the 15 findings only the shallow analyzer reports

Fixed, not suppressed, with patterns the tree already uses; must land before step 3:

- `dynamic_cast<T*>(x)->…` dereferenced unconditionally → `dynamic_cast<T&>`: `ra/techno.cc`,
  `td/techno.cc`, `ra/house.cc`.
- `CHECK_NE(bigger, 0)` is opaque to the shallow analyzer → `CHECK(bigger != 0)`: `ra/face.cc`,
  `td/face.cc`.
- `int realval[5];` is only partly initialized → value-initialize it: `ra/msgbox.cc`.
- The random-lobby walk can step past the end of the list → bound the loop on `pChannel`:
  `ra/wolapiob.cc`.
- `static_cast<DirType>(0..255)` → `AsDirection()`: `td/coord_inline_test.cc`.

### 3. Shallow analyzer locally, deep in CI

- `CMakeLists.txt`: cache string `CLANG_ANALYZER_MODE` (`shallow` default, or `deep`), passed to
  clang-tidy as `-Xclang -analyzer-config -Xclang mode=…`. Shallow inlines only tiny callees and
  caps the path budget. The flag is part of the tidy command line, so ninja and ctcache invalidate
  correctly.
- CI lint job: `-DCLANG_ANALYZER_MODE=deep`, the backstop.

### 4. Drop duplicate check aliases

- Every enabled alias whose primary is also enabled and whose options match is disabled under one
  comment in `.clang-tidy` (e.g. `cert-dcl37-c` and `cert-dcl51-cpp` duplicate
  `bugprone-reserved-identifier`, which so far ran three times). Aliases with different options
  stay.
- Remove the duplicate `cert-err58-cpp.AllowedTypes` block.
- NOLINTs naming a disabled alias drop it or name the primary instead.

### 5. Take `std::optional` out of the giant functions

- `tech/number_parse.h/.cc`: `ParseIntegerOr<T>`, `ParseHexOr<T>` and `ParseIniIntegerOr` return the
  fallback on failure, with no optional involved. Tests in `tech/number_parse_test.cc`.
- Rewrite every `Parse{Integer,Hex,IniInteger}<…>(…).value_or(…)` call site (~238 in 51 files).
- Re-profile the 10 hotspot TUs with `--enable-check-profile`:
  `td/{mapeddlg,init,nulldlg,mapedtm, netdlg}` and `ra/{nulldlg,init,teamtype,trigtype,session}`. A
  function that is still slow holds another optional call; extract that code into its own function.

### 6. Share the clang-tidy cache between build dirs

`cmake/Speedup.cmake` wraps `clang-tidy-cache` in `cmake -E env` with:

- `CTCACHE_DIR` (default `~/.cache/ctcache`), so the cache survives reboots;
- `CTCACHE_STRIP=<binary dir>/` and `CTCACHE_STRIP_SRC=1`, so `_deps` include paths hash the same in
  every dir;
- `CTCACHE_EXCLUDE_HASH_REGEX` for `-O`, `-g` and the color-diagnostics flags CLion adds.

Sharing needs the same build type and the same `.env` in both dirs.

### 7. CLion "Strict RA Clang" profile (user action)

Build type **RelWithDebInfo**, and `-DENABLE_IWYU=OFF` (removing the flag alone keeps the cached
`ON`).

### 8. Memory notes

Full-pass time, the cost breakdown, and the sharing prerequisites.

## Verification

1. Step 2: shallow and deep analyzer runs on the fixed TUs are clean; `ctest --test-dir build`
   passes.
2. Steps 1, 3, 4, 6: `timeout 590 cmake --build build-strict --parallel 14 -- -k 0` in the
   foreground, repeated until `ninja: no work to do`, with zero `error:` lines.
3. Before/after numbers from the same direct-invocation harness, plus a real one-line code change
   timed in `ra/techno.cc` and `td/mapeddlg.cc` (a comment-only edit is a ctcache hit).
4. Sharing: build `rasdl` in `build-strict`, then in `cmake-build-strict-ra-clang`; the second shows
   tidy hits. A deliberate tidy violation is reported by both dirs.
5. `ctest --test-dir build` after the helper and call-site commits.
6. The CI lint job (deep) is green on the next push.

## Not in this plan

- Parsing is ~55% of what remains: each TU is parsed twice, once by tidy and once by the compile.
  Trimming the fan-out of `defines.h`/`externs.h` is the next big lever.
- ccache sharing across the strict dirs (`CCACHE_BASEDIR` + `hash_dir=false`) would make the second
  dir's compile free too; it needs a look at debug-info paths first.
- 31 RA/TD sources are compiled a second time into test targets, 34 extra TUs.

## Progress

- 2026-09-19: plan written.
