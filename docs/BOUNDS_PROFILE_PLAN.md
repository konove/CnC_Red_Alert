# Plan: enable the remaining index and pointer-arithmetic bounds checks

Written 2026-09-26 against `.clang-tidy` and clang-tidy 23.1.2. Companion to the
`cppcoreguidelines-pro-bounds-constant-array-index` and
`cppcoreguidelines-pro-bounds-pointer-arithmetic` rows in
[CLANG_TIDY_PRIORITIES.md](CLANG_TIDY_PRIORITIES.md).

**Status: complete (2026-09-26).** Both checks are enabled; the results are in the "Bounds index and
pointer arithmetic enablement" review in CLANG_TIDY_PRIORITIES.md.

## Context

Both checks were skipped on 2026-09-12 with 3,365 and 2,491 reports, under the "Buffer, union and
boolean conversion policy": the engine walked raw buffers, and the useful change was a span-based
buffer API after which the checks "can be revisited per module". That API now exists.
`clang-diagnostic-unsafe-buffer-usage` and `pro-bounds-avoid-unchecked-container-access` are
enforced tree-wide, and runtime indices go through `base::At`, spans and checked accessors.

No row in CLANG_TIDY_PRIORITIES.md is Pending any more, so the next work is re-measuring the skipped
ones. An isolated sweep of every skipped P2–P4 tidy check on 2026-09-26 (all 1,080 translation units
of `cmake-build-strict-ra-clang/compile_commands.json`, header-verify units included) found these
two nearly clean, while every other skip still has hundreds of reports:

| Check                                               | 2026-09-12 | 2026-09-26 |
| --------------------------------------------------- | ---------- | ---------- |
| `cppcoreguidelines-pro-bounds-constant-array-index` | 3,365      | 13         |
| `cppcoreguidelines-pro-bounds-pointer-arithmetic`   | 2,491      | 10         |

For comparison: `pro-bounds-array-to-pointer-decay` 2,256, `modernize-avoid-c-arrays` 1,985,
`pro-type-union-access` 1,507, `owning-memory` 843, `easily-swappable-parameters` 374.

## Findings

The unsafe-buffer diagnostic leaves two gaps these checks close. It accepts a constant index into a
raw pointer (`p[0]`), which `pointer-arithmetic` reports, and it cannot see into an array subscript
the compiler synthesizes, which `constant-array-index` reports.

**`constant-array-index` (13):**

- `engine/crypto/int.h` `RemainderTable::Increment(const RemainderTable&)` (10): indexes the C
  arrays directly, while the `Increment(uint16_t)` overload just above it already uses `base::At`.
- `engine/crypto/int.h` `Gcd` (3): rotates through a local `T g[3]` by `i % 3`.
- `engine/platform/win32/win32_com.h` `GUID::operator==` (1): the defaulted comparison's synthesized
  loop over `Data4[8]`. The index is the array's own extent, so the hazard cannot occur. `Data4`
  stays a C array, since it mirrors the Windows `GUID`, and code that fills it
  (`wolapi_iids_test.cc`) must compile against both.

**`pointer-arithmetic` (10), all `p[0]` on a `const char*` or C API pointer:**

- `ra/scenario.cc` (3): the first letters of the three message-box button labels, for hotkeys. The
  same function already tests the labels with `*b1txt == '\0'`.
- `td/expand.cc` (2): `list.Current_Item()[0] == 'G'` to pick the side in the two expansion dialogs.
- `td/mplayer.cc` (1): `Read_MultiPlayer_Settings` terminating a fresh `new char[INITSTRBUF_MAX]`
  through the raw pointer, one line after wrapping it in the span `entry_storage`.
- `engine/window/ww_mouse.cc` (1): clearing entry 0's alpha in the SDL cursor palette.
- `base/enum_array_test.cc` (1), `td/list_test.cc` (2): tests reading the first character of a
  returned C string.

## Plan

1. **Fix the sites.**
   - `int.h`: `base::At` in the second `Increment`, like the first; `Gcd` indexes its `T g[3]`
     through `base::At` too (it has no `std::array` overload).
   - `win32_com.h`: a reasoned `NOLINTNEXTLINE` on the defaulted `operator==`.
   - `scenario.cc`: `*b1txt` (and b2, b3), the function's own idiom.
   - `expand.cc`: `std::string_view(list.Current_Item()).starts_with('G')`.
   - `mplayer.cc`: write the terminator through `entry_storage`.
   - `ww_mouse.cc`: `palette->colors->a`, the first of the 256 entries SDL allocates for an 8-bit
     surface.
   - Tests: compare through `std::string_view` (`== "pear"`, `.front()`).
2. **Enable.** Remove both names from `.clang-tidy`, re-sweep to zero, run the full strict build,
   the GCC build and CTest, and record the result in CLANG_TIDY_PRIORITIES.md: both rows Enabled, a
   review section, and a note on the 2026-09-12 policy section that it is superseded for these two.

## Verification

- The isolated sweep of both checks reports nothing, with no `clang-diagnostic-error` in any TU.
- `cmake --build build-strict --parallel 14` and `cmake --build build --parallel 22` succeed;
  `ctest` passes in both (the bignum, WOL IID, enum array and TD list tests cover the touched code).
