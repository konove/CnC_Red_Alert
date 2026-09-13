# Clang-tidy priorities

Updated: 2026-09-12, against [`.clang-tidy`](../.clang-tidy) and clang-tidy 23.1.2.

This tracks **all 162 currently excluded check names** and completed entries, in recommended work
order. Priorities reflect likely defect prevention, relevance to this engine, and the cost of useful
fixes; they are judgments, not fresh finding counts. Start at P1 and work downward. Aliases stay
beside their related check so a single cleanup can handle them together. Previously deferred checks
are back on the list for review.

Keep completed rows in place: change **Status** to **Enabled** and record the commit in **Reason /
result**. Use **Skipped** with a short reason only after deciding against a check. P5 entries are
recommendations to skip, not completed decisions. Add newly excluded checks when the configuration
changes.

- **Pending:** excluded and awaiting work.
- **Enabled:** enforced by the configuration; the result records the enabling commit.
- **Skipped:** reviewed but excluded for the recorded reason; revisit toolchain limitations after
  upgrades.
- **Covered:** this name is excluded, but an enabled equivalent already supplies the check.
- **Legacy:** excluded name unavailable in the installed LLVM 23 toolchain; review older-toolchain
  compatibility alongside the related check. Do not mistake zero findings for successful
  enforcement.
- **Skip proposed:** low expected value or an unsuitable platform/style policy.

Alias relationships can be checked in the
[LLVM check index](https://clang.llvm.org/extra/clang-tidy/checks/list.html). Availability above
comes from the installed tool, since the online documentation follows LLVM development.

## P1 — Direct correctness and memory safety

| Check                                                         | Status  | Reason / result                                                                                                                                                                                                                                   |
| ------------------------------------------------------------- | ------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `bugprone-suspicious-stringview-data-usage`                   | Enabled | Commit `Enable string-view data usage checking`: copy the RA title-screen filename to a terminated string before calling the PCX reader.                                                                                                          |
| `abseil-unchecked-statusor-access`                            | Skipped | Commit `Handle failed PCX byte reads`: guard both games' byte reads, but retain the exclusion because LLVM 23.1.2 crashes even on checked access with Abseil 20260107.0. Revisit after a toolchain fix; see reproduction below.                   |
| `clang-analyzer-unix.cstring.UninitializedRead`               | Enabled | Commit `Enable uninitialized C-string read checking`: initialize the public-key generation self-test buffer while preserving the random-fill loop.                                                                                                |
| `clang-analyzer-cplusplus.InnerPointer`                       | Enabled | Commit `Enable string inner-pointer checking`: detect string-buffer pointers used after invalidation.                                                                                                                                             |
| `bugprone-copy-constructor-init`                              | Enabled | Commit `Enable copy-constructor base initialization checking`: both games and shared code already initialize copied base state correctly; no source fixes needed.                                                                                 |
| `bugprone-unchecked-string-to-number-conversion`              | Enabled | Commit `Enable checked string-to-number conversions`: replace unchecked decimal/hex conversions with range-checked parsing and explicit defaults; preserve legacy INI, coordinate, and protocol formats.                                          |
| `cert-err34-c`                                                | Enabled | Alias enabled with `bugprone-unchecked-string-to-number-conversion` in commit `Enable checked string-to-number conversions`.                                                                                                                      |
| `clang-analyzer-unix.StdCLibraryFunctions`                    | Enabled | Commit `Fix TCP socket option size and enable C library checking`: pass an integer TCP_NODELAY flag with its actual size, avoiding a read past a one-byte bool.                                                                                   |
| `clang-diagnostic-cast-align`                                 | Enabled | Commit `Fix buffer alignment and enable cast alignment checking`: copy unaligned packet/media values, check typed buffer access, align cached shape headers, and bound legacy byte fills.                                                         |
| `clang-diagnostic-uninitialized-const-pointer`                | Enabled | Commit `Enable uninitialized const-pointer argument checking`: both games and shared code pass without source fixes.                                                                                                                              |
| `clang-diagnostic-reorder-ctor`                               | Enabled | Commit `Match constructor initialization order to declarations`: reorder 16 initializer lists while preserving expressions, member layouts, and actual initialization order.                                                                      |
| `bugprone-unhandled-code-paths`                               | Skipped | Commit `Document missing-default check policy`: default mode flags switches with valid post-switch fallbacks and bounded inputs; retain exclusion rather than require redundant defaults. See review below.                                       |
| `bugprone-non-zero-enum-to-bool-conversion`                   | Enabled | Commit `Enable nonzero enum-to-bool conversion checking`: both games and shared code pass without source fixes.                                                                                                                                   |
| `clang-analyzer-optin.core.EnumCastOutOfRange`                | Skipped | Commit `Document enum cast range check policy`: LLVM 23.1.2 rejects intentional intermediate directions, flag combinations, and path-command sentinels; retain exclusion. See review below.                                                       |
| `clang-diagnostic-tautological-constant-out-of-range-compare` | Enabled | Commit `Preserve shutdown states and enable constant range comparison checking`: store RA shutdown states 0 through 3 in an integer instead of collapsing them to bool.                                                                           |
| `clang-diagnostic-tautological-unsigned-enum-zero-compare`    | Enabled | Commit `Simplify unsigned enum bounds and enable zero comparison checking`: use an unsigned event range check and remove an impossible negative template-ID check while preserving the no-template sentinel.                                      |
| `clang-diagnostic-tautological-unsigned-zero-compare`         | Enabled | Commit `Remove impossible icon checks and enable unsigned zero comparison checking`: drop the always-false negative test on the unsigned template icon index in both map validators while keeping the upper bound and icon-map checks.            |
| `clang-diagnostic-implicit-int-conversion`                    | Enabled | Commit `Make integer narrowing explicit and enable implicit conversion checking`: cast intentional narrowing at 518 sites, return full uncompressed image sizes, assign the RA foot speed directly, and widen the TD ownable-house mask accessor. |
| `clang-diagnostic-implicit-int-conversion-on-negation`        | Enabled | Enabled with `clang-diagnostic-implicit-int-conversion` in commit `Make integer narrowing explicit and enable implicit conversion checking`: three negated heights packed into coordinates use explicit `LEPTON` casts.                           |
| `clang-diagnostic-int-to-pointer-cast`                        | Enabled | Commit `Enable integer-to-pointer cast checking`: both games and shared code pass without source fixes.                                                                                                                                           |
| `bugprone-derived-method-shadowing-base-method`               | Enabled | Commit `Turn hidden base methods into overrides and enable shadowing checking`: make the UI reset and INI writing chains virtual, drop redundant redeclarations, rename the buffer lock, and annotate the typed interface classes.                |

## P2 — Further correctness and targeted safety

| Check                                                      | Status  | Reason / result                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| ---------------------------------------------------------- | ------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------- | -------------------------------------------------------------------- |
| `clang-diagnostic-lifetime-safety-use-after-free`          | Enabled | Commit `Stop returning freed projectiles from the firing code`: clear the bullet pointer when unlimbo fails and read the recoil flag before the free.                                                                                                                                                                                                                                                                                                                                                   |
| `clang-diagnostic-lifetime-safety-invalidation`            | Skipped | Commit `Clear the screen buffer globals their owners delete`: fix the eight dangling-global findings, but retain the exclusion because LLVM 23.1.2 flags two consecutive `push_back` calls. See review below.                                                                                                                                                                                                                                                                                           |
| `clang-diagnostic-lifetime-safety-use-after-scope-moved`   | Enabled | Commit `Take the unit shape pointer from its owner and enable moved-storage checking`: store the shape data first, then read the pointer back, instead of holding one into a moved-from local.                                                                                                                                                                                                                                                                                                          |
| `bugprone-parent-virtual-call`                             | Enabled | Commit `Document the deliberate grandparent dispatches and enable parent virtual call checking`: drop TD's duplicate mission dump; the other twelve skips are intentional and now say why.                                                                                                                                                                                                                                                                                                              |
| `cppcoreguidelines-interfaces-global-init`                 | Enabled | Commit `Enable cross-unit global initialization checking`: TD's house table is the only report and is safe on two counts; it is annotated, and the check now guards the rest of the tree.                                                                                                                                                                                                                                                                                                               |
| `bugprone-throwing-static-initialization`                  | Enabled | Commit `Make the game data tables nothrow constructible and enable throwing static initialization checking`: mark the type-class, value-type, timer, heap and gadget constructors `noexcept`, and allow the engine's allocating singletons by type. See review below.                                                                                                                                                                                                                                   |
| `cert-err58-cpp`                                           | Enabled | Alias enabled with `bugprone-throwing-static-initialization` in the same commit; the alias needs its own `AllowedTypes` copy.                                                                                                                                                                                                                                                                                                                                                                           |
| `cppcoreguidelines-init-variables`                         | Skipped | Commit `Document local initialization check policy`: the check's own fix hides findings the eleven enabled uninitialized-use checks already report, and those report nothing across the tree. See review below.                                                                                                                                                                                                                                                                                         |
| `cppcoreguidelines-special-member-functions`               | Enabled | Commit `Declare copy and move intent on the classes that own a destructor`: annotate 170 class declarations; the compiler found the five classes that are genuinely copied. See review below.                                                                                                                                                                                                                                                                                                           |
| `hicpp-special-member-functions`                           | Enabled | Exclusion removed with `cppcoreguidelines-special-member-functions` in the same commit. The name does nothing on LLVM 23, so the alias enforces the same rule wherever it exists.                                                                                                                                                                                                                                                                                                                       |
| `clang-diagnostic-deprecated-copy-with-user-provided-copy` | Enabled | Commit `Enable deprecated implicit copy checking`: no reports; the declarations added for `cppcoreguidelines-special-member-functions` removed every implicit copy definition. See review below.                                                                                                                                                                                                                                                                                                        |
| `clang-diagnostic-deprecated-copy-with-user-provided-dtor` | Enabled | Commit `Enable deprecated implicit copy checking`: the three reports were `CellClass` in both games and TD's `TCountDownTimerClass`, already resolved by the preceding commit.                                                                                                                                                                                                                                                                                                                          |
| `clang-diagnostic-deprecated-copy-with-dtor`               | Enabled | Commit `Enable deprecated implicit copy checking`: the one report was `AbstractTypeClass`, already resolved by the preceding commit.                                                                                                                                                                                                                                                                                                                                                                    |
| `bugprone-macro-parentheses`                               | Enabled | Commit `Parenthesize macro bodies and arguments`: 85 sites, applied with the check's own fix-its; no current expansion changes meaning. See review below.                                                                                                                                                                                                                                                                                                                                               |
| `clang-diagnostic-logical-op-parentheses`                  | Enabled | Commit `Fix the shifted window origin and make operator precedence explicit`: six `&&` inside `                                                                                                                                                                                                                                                                                                                                                                                                         |                                                             | `, all parenthesized to keep the current grouping. See review below. |
| `clang-diagnostic-bitwise-op-parentheses`                  | Enabled | Commit `Fix the shifted window origin and make operator precedence explicit`: seventeen `&` inside `                                                                                                                                                                                                                                                                                                                                                                                                    | `, including the SHA-1 round functions; grouping preserved. |
| `clang-diagnostic-shift-op-parentheses`                    | Enabled | Commit `Fix the shifted window origin and make operator precedence explicit`: two real bugs -- TD shifted the window origin by `3 + Get_XPos()` instead of adding it.                                                                                                                                                                                                                                                                                                                                   |
| `readability-math-missing-parentheses`                     | Enabled | Commit `Parenthesize mixed-precedence arithmetic`: 2,513 sites, applied with the check's own fix-its plus 16 manual edits the fix-its could not reach; no behavior change (see the review below).                                                                                                                                                                                                                                                                                                       |
| `bugprone-branch-clone`                                    | Enabled | Commit `Merge duplicate branches and enable branch clone checking`: 43 sites, none a copy/paste bug; stack identical case labels, join repeated condition bodies, and collapse four identical if/else pairs. See review below.                                                                                                                                                                                                                                                                          |
| `clang-diagnostic-sign-conversion`                         | Skipped | Commit `Document sign and parameter check policy`: 2,432 reports; about 650 are `int` sizes and counts passed to `size_t` library parameters, 876 `int`/`unsigned` mixes on legacy fields the type-migration plan keeps, and 219 `long`/`unsigned long`. The project's signed-by-default policy makes each a cast rather than a fix. See review below.                                                                                                                                                  |
| `bugprone-signed-bitwise`                                  | Skipped | Commit `Document sign and parameter check policy`: 3,715 reports even with `IgnorePositiveIntegerLiterals`; about 1,800 are enum flag ORs in the unit and building data tables, the rest deliberate bit manipulation in the VQA loader, blitters and crypto. See review below.                                                                                                                                                                                                                          |
| `hicpp-signed-bitwise`                                     | Legacy  | Unavailable in LLVM 23; review with `bugprone-signed-bitwise` on older tools.                                                                                                                                                                                                                                                                                                                                                                                                                           |
| `clang-diagnostic-switch-enum`                             | Skipped | Commit `Document variadic and thread-safety check policy`: all 293 reports are switches that already have a deliberate default over large type enums (up to 102 values); `-Wswitch` and `-Wswitch-default` already require an explicit fallback.                                                                                                                                                                                                                                                        |
| `clang-diagnostic-switch`                                  | Enabled | Commit `Give every switch a fallback and switch on key numbers as integers`: 792 reports; 639 were gadget-ID `ButtonKey()` cases in 98 `KeyNumType` switches, which now switch on the integer key number; the 153 unhandled-enumerator switches get the default below.                                                                                                                                                                                                                                  |
| `clang-diagnostic-switch-bool`                             | Enabled | Commit `Enable ten checks the tree already satisfies`: no findings across 460 translation units; a probe confirms it reports.                                                                                                                                                                                                                                                                                                                                                                           |
| `clang-diagnostic-duplicate-enum`                          | Enabled | Commit `Drop the implicit FIRST enum aliases and fix mixed enum operations`: 25 reports, all an `X_FIRST = 0` alias duplicating the first real enumerator (22 TD enums, three RA trigger/team enums); uses now name that enumerator, per the magic_enum no-alias rule.                                                                                                                                                                                                                                  |
| `clang-diagnostic-missing-braces`                          | Enabled | Commit `Brace the infantry animation control tables`: all 680 reports were rows of TD's `[DO_COUNT][3]` tables in `idata.cc`, now one brace pair per row; layout unchanged.                                                                                                                                                                                                                                                                                                                             |
| `clang-diagnostic-cast-qual`                               | Skipped | Commit `Fix four writes through const and document cast-qual policy`: 422 reports. Four were writes through `const` and are fixed by design. The other ~260 write through a `const` object, so the only mechanical fix is `const_cast`, which the enabled `cppcoreguidelines-pro-type-const-cast` rejects; they need API const-correctness work instead. See review below.                                                                                                                              |
| `misc-explicit-constructor`                                | Enabled | Commit `Make single-argument constructors explicit where conversion is unintended`: 125 reports; the check's fix-its made 69 constructors explicit (file, pipe, straw, heap, vector, dialog and game-object constructors) with no call site relying on the conversion. The 56 deliberate conversions stay implicit under a reasoned `NOLINTNEXTLINE`: object-to-type-ID operators, `CCPtr`, `TargetClass`, `FacingClass`, countdown timers, choice tables, palettes and big integers. See review below. |
| `cppcoreguidelines-explicit-constructor`                   | Enabled | Alias enabled with `misc-explicit-constructor` in commit `Make single-argument constructors explicit where conversion is unintended`.                                                                                                                                                                                                                                                                                                                                                                   |
| `google-explicit-constructor`                              | Enabled | Alias enabled with `misc-explicit-constructor` in commit `Make single-argument constructors explicit where conversion is unintended`.                                                                                                                                                                                                                                                                                                                                                                   |
| `hicpp-explicit-conversions`                               | Legacy  | Unavailable in LLVM 23; review with `misc-explicit-constructor` on older tools.                                                                                                                                                                                                                                                                                                                                                                                                                         |
| `cppcoreguidelines-pro-type-vararg`                        | Skipped | Skipped in commit `Document variadic interface check policy`: 1,586 reports, every call into those printers and the C formatting library; follows the variadic-definition decision.                                                                                                                                                                                                                                                                                                                     |
| `hicpp-vararg`                                             | Legacy  | Unavailable in LLVM 23; review with `cppcoreguidelines-pro-type-vararg` on older tools.                                                                                                                                                                                                                                                                                                                                                                                                                 |
| `modernize-avoid-variadic-functions`                       | Skipped | Skipped in commit `Document variadic interface check policy`: the 19 reports are the printf-style text and debug printers (`Fancy_Text_Print`, `Plain_Text_Print`, `Mono_Printf`, `Smart_Printf`, `Fatal`, `Format_Runtime_Text`) with about 1,500 call sites; most format translated strings looked up at run time, so a parameter-pack replacement still needs a runtime-typed formatter. See review below.                                                                                           |
| `cert-dcl50-cpp`                                           | Skipped | Alias of `modernize-avoid-variadic-functions`; skipped with it.                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| `clang-diagnostic-missing-format-attribute`                | Enabled | Commit `Enable ten checks the tree already satisfies`: no findings across 460 translation units; a probe confirms it reports.                                                                                                                                                                                                                                                                                                                                                                           |
| `clang-diagnostic-undef`                                   | Enabled | Commit `Test language and debug macros with defined()`: 105 reports; TD's `FRENCH`/`GERMAN`/`JAPANESE` builds and two WOL `SHOW_MONO` blocks tested undefined macros with `#if`, now `defined()` with the same result.                                                                                                                                                                                                                                                                                  |
| `clang-diagnostic-undefined-func-template`                 | Enabled | Commit `Declare the explicitly instantiated templates`: 121 reports; `extern template` declarations now sit beside `CCPtr`, TD's vectors, `ObjectPtr` and the out-of-line `Serialize` members whose definitions live in one `.cc` file.                                                                                                                                                                                                                                                                 |
| `clang-diagnostic-undefined-var-template`                  | Enabled | Commit `Declare the CCPtr heap specializations`: the 27 `CCPtr<T>::Heap` explicit specializations defined in `globals.cc` are now declared in `ccptr.h`, which also removes an ill-formed use-before-declaration.                                                                                                                                                                                                                                                                                       |
| `clang-diagnostic-shadow-field`                            | Enabled | Commit `Name each map layer's redraw flag after its layer`: the eight reports were one `IsToRedraw` bit-field redeclared at every step of both games' `GScreenClass` → `TabClass` chain; each layer's flag now has its own name, and every use was rebound by the compiler to the layer it already meant. See review below.                                                                                                                                                                             |
| `clang-diagnostic-shadow`                                  | Enabled | Commit `Give shadowing locals and parameters their own names`: 65 reports, none a use of the wrong variable; the 57 inner locals and eight member-named parameters are renamed within their scope. See review below.                                                                                                                                                                                                                                                                                    |
| `concurrency-mt-unsafe`                                    | Skipped | Commit `Document variadic and thread-safety check policy`: 323 reports, 222 `strtok` in INI and text parsing plus `exit`, `rand`, `inet_ntoa`, `gethostbyname`, `getenv` and `glob`; every call runs on the main game thread, and the SDL audio callback and VQA timer paths call none of them. See review below.                                                                                                                                                                                       |
| `clang-analyzer-optin.core.FixedAddressDereference`        | Enabled | Commit `Keep mono pages in memory and bound the box drawing`: the port addressed the DOS mono card at 0xB0000; the pages now live in memory, which exposed and fixed an off-by-one box clamp, an unclamped view size and `Fill_Attrib` testing `h` for `y` without the enable check.                                                                                                                                                                                                                    |
| `clang-analyzer-core.FixedAddressDereference`              | Legacy  | Unavailable in LLVM 23; review with `clang-analyzer-optin.core.FixedAddressDereference` on older tools.                                                                                                                                                                                                                                                                                                                                                                                                 |
| `bugprone-easily-swappable-parameters`                     | Skipped | Commit `Document sign and parameter check policy`: 426 reports, almost all adjacent same-typed coordinates, sizes and IDs in the legacy drawing, gadget and type APIs (`(int x, int y, int w, int h)`); renaming or wrapping them in strong types would touch most call sites for little defect value.                                                                                                                                                                                                  |
| `bugprone-random-generator-seed`                           | Enabled | Commit `Seed the C library generator from random_device`: five `srand(time(nullptr))` seeds use `std::random_device`; TD's dead `srand(0)` before `randomize()` is removed. `tech/rawfile.h` takes `UINT_MAX` from `std::numeric_limits`.                                                                                                                                                                                                                                                               |
| `cert-msc32-c`                                             | Enabled | Alias enabled with `bugprone-random-generator-seed` in the same commit.                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| `cert-msc51-cpp`                                           | Enabled | Alias enabled with `bugprone-random-generator-seed` in the same commit.                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| `modernize-use-integer-sign-comparison`                    | Enabled | Commit `Compare mixed-sign integers with std::cmp functions`: 238 reports, applied with the check's fix-its (`std::cmp_less` and friends, with `<utility>`); the values compared are unchanged, and the comparisons are now correct for negative operands.                                                                                                                                                                                                                                              |
| `modernize-use-nodiscard`                                  | Enabled | Commit `Mark value-returning functions nodiscard`: 927 reports; the check's fix-its applied 921 attributes; 47 declarations whose results the engine deliberately ignores (`Validate`, `Create_And_Place`, `Create_One_Of`, the `AI_*` helpers) keep no attribute under a reasoned suppression, and no call site discards a result. See review below.                                                                                                                                                   |

## P3 — Broader safety and maintainability

| Check                                                           | Status  | Reason / result                                                                                                                                                                                                                                                                                                                                                                                                              |
| --------------------------------------------------------------- | ------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `clang-diagnostic-unsafe-buffer-usage`                          | Skipped | Commit `Document buffer, union and boolean conversion check policy`: 5,924 reports of raw pointer and array indexing across the blitters, codecs, packet and save code; enforcing it needs a span-based buffer API first. See review below.                                                                                                                                                                                  |
| `cppcoreguidelines-pro-bounds-avoid-unchecked-container-access` | Skipped | Commit `Document buffer, union and boolean conversion check policy`: 1,840 reports of `operator[]` on the engine's vectors and heaps. See review below.                                                                                                                                                                                                                                                                      |
| `cppcoreguidelines-pro-bounds-constant-array-index`             | Skipped | Commit `Document buffer, union and boolean conversion check policy`: 3,365 reports of runtime indices into fixed game tables. See review below.                                                                                                                                                                                                                                                                              |
| `cppcoreguidelines-owning-memory`                               | Skipped | Commit `Document P3 checks the legacy-code policy rules out`: 1,240 reports, raw `new`/`delete` ownership across the object heaps, dialogs and buffers; `gsl::owner` or smart pointers everywhere is exactly what CLAUDE.md's legacy-code rules list it under changes to avoid unless requested. See review below.                                                                                                           |
| `cppcoreguidelines-no-malloc`                                   | Pending | Move suitable allocations to typed lifetime management.                                                                                                                                                                                                                                                                                                                                                                      |
| `hicpp-no-malloc`                                               | Legacy  | Unavailable in LLVM 23; review with `cppcoreguidelines-no-malloc` on older tools.                                                                                                                                                                                                                                                                                                                                            |
| `cppcoreguidelines-pro-type-union-access`                       | Skipped | Commit `Document buffer, union and boolean conversion check policy`: 1,438 reports on the event, target and packet unions, whose layouts are part of the network and save formats. See review below.                                                                                                                                                                                                                         |
| `cppcoreguidelines-pro-bounds-array-to-pointer-decay`           | Skipped | Commit `Document buffer, union and boolean conversion check policy`: 4,423 reports, mostly C strings and fixed arrays passed to C-style APIs. See review below.                                                                                                                                                                                                                                                              |
| `hicpp-no-array-decay`                                          | Legacy  | Unavailable in LLVM 23; review with `cppcoreguidelines-pro-bounds-array-to-pointer-decay` on older tools.                                                                                                                                                                                                                                                                                                                    |
| `cppcoreguidelines-pro-bounds-pointer-arithmetic`               | Skipped | Commit `Document buffer, union and boolean conversion check policy`: 2,491 reports in the pixel, audio, compression and crypto loops that walk raw buffers. See review below.                                                                                                                                                                                                                                                |
| `modernize-avoid-c-arrays`                                      | Skipped | Commit `Document P3 checks the legacy-code policy rules out`: 2,252 reports; many arrays are fixed-layout game data, packet and save structures, and STL containers everywhere is what CLAUDE.md's legacy-code rules list it under changes to avoid unless requested. See review below.                                                                                                                                      |
| `cppcoreguidelines-avoid-c-arrays`                              | Skipped | Alias of `modernize-avoid-c-arrays`; skipped with it.                                                                                                                                                                                                                                                                                                                                                                        |
| `hicpp-avoid-c-arrays`                                          | Legacy  | Unavailable in LLVM 23; review with `modernize-avoid-c-arrays` on older tools.                                                                                                                                                                                                                                                                                                                                               |
| `cppcoreguidelines-use-enum-class`                              | Pending | Strengthen enum boundaries; account for flags and serialized values.                                                                                                                                                                                                                                                                                                                                                         |
| `modernize-avoid-c-style-cast`                                  | Pending | Make conversion intent visible across remaining casts.                                                                                                                                                                                                                                                                                                                                                                       |
| `google-readability-casting`                                    | Pending | Alias of `modernize-avoid-c-style-cast`; handle together.                                                                                                                                                                                                                                                                                                                                                                    |
| `cppcoreguidelines-pro-type-cstyle-cast`                        | Pending | Review C-style casts that bypass type safety.                                                                                                                                                                                                                                                                                                                                                                                |
| `clang-diagnostic-old-style-cast`                               | Pending | Finish compiler enforcement after the cast migration.                                                                                                                                                                                                                                                                                                                                                                        |
| `clang-diagnostic-deprecated-enum-enum-conversion`              | Enabled | Commit `Drop the implicit FIRST enum aliases and fix mixed enum operations`: 76 reports; the `WWKEY_*` modifier bits are flags, so they became integer constants, and five facing-to-animation offsets cast the facing to `int`.                                                                                                                                                                                             |
| `clang-diagnostic-deprecated-anon-enum-enum-conversion`         | Enabled | Commit `Drop the implicit FIRST enum aliases and fix mixed enum operations`: three TD editor house-button offsets now subtract from an integer key number.                                                                                                                                                                                                                                                                   |
| `clang-diagnostic-deprecated-enum-compare`                      | Enabled | Commit `Drop the implicit FIRST enum aliases and fix mixed enum operations`: six comparisons against the wrong enum's zero or 1002 constant (`RESULT_NONE` for `IMPACT_NONE`, `ACTION_NONE` for `TACTION_NONE`, `NET_FILE_CHUNK` for `SERIAL_FILE_CHUNK`); values were equal, so behavior is unchanged.                                                                                                                      |
| `google-runtime-int`                                            | Pending | Bring remaining integer spellings into the project's fixed-width policy.                                                                                                                                                                                                                                                                                                                                                     |
| `modernize-use-default-member-init`                             | Pending | Centralize common defaults and reduce constructor drift.                                                                                                                                                                                                                                                                                                                                                                     |
| `cppcoreguidelines-use-default-member-init`                     | Pending | Alias of `modernize-use-default-member-init`; handle together.                                                                                                                                                                                                                                                                                                                                                               |
| `cppcoreguidelines-prefer-member-initializer`                   | Pending | Initialize members directly; preserve construction-order semantics.                                                                                                                                                                                                                                                                                                                                                          |
| `modernize-use-equals-delete`                                   | Enabled | Commit `Use defaulted and deleted special members and range loops`: 54 reports. The fix-its turn 11 undefined private copy operations into `= delete`, and the deleted members of 16 classes (the pipe and straw family, `BufferClass`, `IconsetClass`, both games' `FixedHeapClass` and `CCFileClass`, `IndexClass`, `GenericList`) move to `public:`, where misuse reports a deleted function rather than an access error. |
| `hicpp-use-equals-delete`                                       | Legacy  | Unavailable in LLVM 23; review with `modernize-use-equals-delete` on older tools.                                                                                                                                                                                                                                                                                                                                            |
| `modernize-use-equals-default`                                  | Enabled | Commit `Use defaulted and deleted special members and range loops`: 62 reports (35 trivial destructors, 27 trivial default constructors), now `= default` from the check's fix-its.                                                                                                                                                                                                                                          |
| `hicpp-use-equals-default`                                      | Legacy  | Unavailable in LLVM 23; review with `modernize-use-equals-default` on older tools.                                                                                                                                                                                                                                                                                                                                           |
| `performance-noexcept-swap`                                     | Enabled | Commit `Enable ten checks the tree already satisfies`: no findings across 460 translation units; a probe confirms it reports.                                                                                                                                                                                                                                                                                                |
| `cppcoreguidelines-noexcept-swap`                               | Enabled | Commit `Enable ten checks the tree already satisfies`: no findings across 460 translation units; a probe confirms it reports.                                                                                                                                                                                                                                                                                                |
| `bugprone-switch-missing-default-case`                          | Enabled | Commit `Give every switch a fallback and switch on key numbers as integers`: all 136 reports were among the 311 switches given a default.                                                                                                                                                                                                                                                                                    |
| `hicpp-multiway-paths-covered`                                  | Legacy  | Unavailable in LLVM 23; review with `bugprone-switch-missing-default-case` on older tools.                                                                                                                                                                                                                                                                                                                                   |
| `clang-diagnostic-switch-default`                               | Enabled | Commit `Give every switch a fallback and switch on key numbers as integers`: 311 switches without a default, now `default: break;`, matching GCC's `-Wswitch-default` policy in `CMakeLists.txt`.                                                                                                                                                                                                                            |
| `clang-diagnostic-covered-switch-default`                       | Skipped | Commit `Document variadic and thread-safety check policy`: conflicts with GCC's `-Wswitch-default`, which the build already requires (see `CMakeLists.txt`); 16 reports are defaults on fully covered switches.                                                                                                                                                                                                              |
| `readability-implicit-bool-conversion`                          | Skipped | Commit `Document buffer, union and boolean conversion check policy`: 6,938 reports, overwhelmingly `if (ptr)` and flag tests, which the Google C++ style guide explicitly allows. See review below.                                                                                                                                                                                                                          |
| `readability-inconsistent-declaration-parameter-name`           | Enabled | Commit `Name declaration parameters after their definitions`: 164 reports, applied with the check's fix-its. TD's `WWGetPrivateProfileString` definition took RA's parameter names instead, because the header-side rename made 66 correct calls read as swapped arguments to `readability-suspicious-call-argument`.                                                                                                        |
| `misc-const-correctness`                                        | Skipped | Commit `Document P3 checks the legacy-code policy rules out`: 4,090 reports of locals that could be `const`; const everywhere is what CLAUDE.md's legacy-code rules list it under changes to avoid unless requested, and the fix-its would churn most dialog and game-logic functions. See review below.                                                                                                                     |
| `readability-make-member-function-const`                        | Pending | Expose read-only operations for safer interfaces.                                                                                                                                                                                                                                                                                                                                                                            |
| `misc-override-with-different-visibility`                       | Enabled | Commit `Match override access to the base declarations`: 19 overrides moved to their base's access level in both games' gadget, turret, drive, building and list classes.                                                                                                                                                                                                                                                    |
| `misc-header-include-cycle`                                     | Enabled | Commit `Enable ten checks the tree already satisfies`: no findings across 460 translation units; a probe confirms it reports.                                                                                                                                                                                                                                                                                                |
| `misc-include-cleaner`                                          | Pending | Review missing/redundant includes alongside existing IWYU checks.                                                                                                                                                                                                                                                                                                                                                            |
| `clang-diagnostic-missing-prototypes`                           | Enabled | Commit `Declare shared symbols in headers and give file-local ones internal linkage`: header drift fixed (`Expansion_Dialog`, `Smart_Printf`, `Write_Bin_Init`, `LCW_Uncompress`, dead `output` stub); local `extern` declarations and prototypes moved into the defining file's header. See review below.                                                                                                                   |
| `clang-diagnostic-missing-variable-declarations`                | Enabled | Commit `Declare shared symbols in headers and give file-local ones internal linkage`: shared globals declared in their headers; unused DOS globals `MaxDevice`, `DefaultDrive` and `CallingDOSInt` deleted. See review below.                                                                                                                                                                                                |
| `misc-use-internal-linkage`                                     | Enabled | Commit `Declare shared symbols in headers and give file-local ones internal linkage`: 240 file-local functions and variables made `static`; `AnalyzeTypes` off because types only get internal linkage from anonymous namespaces. See review below.                                                                                                                                                                          |
| `misc-use-anonymous-namespace`                                  | Skipped | Commit `Declare shared symbols in headers and give file-local ones internal linkage`: 766 reports would move working `static` definitions into unnamed namespaces with no linkage change; Google style accepts `static` and the tree uses no namespaces. See review below.                                                                                                                                                   |
| `clang-diagnostic-unneeded-internal-declaration`                | Enabled | Commit `Enable ten checks the tree already satisfies`: no findings across 460 translation units; a probe confirms it reports.                                                                                                                                                                                                                                                                                                |
| `clang-diagnostic-unused-but-set-global`                        | Enabled | Commit `Remove the unused path-finding start location`: `StartLocation` was only read inside `#ifdef NEVER` in both games.                                                                                                                                                                                                                                                                                                   |
| `misc-static-assert`                                            | Enabled | Commit `Enable ten checks the tree already satisfies`: no findings across 460 translation units; a probe confirms it reports.                                                                                                                                                                                                                                                                                                |
| `cert-dcl03-c`                                                  | Enabled | Commit `Enable ten checks the tree already satisfies`: no findings across 460 translation units; a probe confirms it reports.                                                                                                                                                                                                                                                                                                |
| `modernize-use-std-format`                                      | Skipped | Commit `Document formatting check policy`: the 11 reports are `absl::StrFormat` calls; CLAUDE.md prefers Abseil over the standard library equivalents, and `absl::StrFormat` is the project's formatter.                                                                                                                                                                                                                     |
| `modernize-use-std-print`                                       | Skipped | Commit `Document formatting check policy`: 177 reports on legacy `printf`-family debug output (mostly the queue and serial diagnostics); new diagnostics go through Abseil `LOG`/`DLOG`, so the replacement is logging, not `std::print`.                                                                                                                                                                                    |
| `performance-string-view-conversions`                           | Enabled | Commit `Apply range algorithms and trim string conversions`: one test literal passed directly; `MixFileClass::Open` keeps its terminated copy under a named local.                                                                                                                                                                                                                                                           |
| `modernize-loop-convert`                                        | Enabled | Commit `Use defaulted and deleted special members and range loops`: 52 index loops become range-based `for` from the fix-its; two nested score-reset loops the fix-its skipped are converted by hand, and one loop that sells the first qualifying building keeps its form under a reasoned suppression of `readability-use-anyofallof`.                                                                                     |
| `modernize-use-ranges`                                          | Enabled | Commit `Apply range algorithms and trim string conversions`: nine `std::fill`/`std::find_if` calls use their ranges forms, from the check's fix-its.                                                                                                                                                                                                                                                                         |
| `cppcoreguidelines-macro-usage`                                 | Pending | Replace avoidable macros with typed language constructs.                                                                                                                                                                                                                                                                                                                                                                     |
| `modernize-macro-to-enum`                                       | Pending | Replace suitable integral macro groups with typed constants/enums.                                                                                                                                                                                                                                                                                                                                                           |
| `cppcoreguidelines-macro-to-enum`                               | Pending | Alias of `modernize-macro-to-enum`; handle together.                                                                                                                                                                                                                                                                                                                                                                         |
| `bugprone-reserved-identifier`                                  | Pending | Avoid collisions with implementation-reserved identifiers.                                                                                                                                                                                                                                                                                                                                                                   |
| `cert-dcl37-c`                                                  | Pending | Alias of `bugprone-reserved-identifier`; handle together.                                                                                                                                                                                                                                                                                                                                                                    |
| `cert-dcl51-cpp`                                                | Pending | Alias of `bugprone-reserved-identifier`; handle together.                                                                                                                                                                                                                                                                                                                                                                    |
| `clang-diagnostic-reserved-identifier`                          | Pending | Enforce compiler-detected reserved names.                                                                                                                                                                                                                                                                                                                                                                                    |
| `clang-diagnostic-reserved-macro-identifier`                    | Pending | Fix reserved macros; also reduce preprocessing warnings/cache misses.                                                                                                                                                                                                                                                                                                                                                        |
| `clang-diagnostic-invalid-source-encoding`                      | Enabled | Commit `Enable ten checks the tree already satisfies`: no findings across 460 translation units; a probe confirms it reports.                                                                                                                                                                                                                                                                                                |
| `portability-template-virtual-member-function`                  | Pending | Review compiler-dependent template/virtual behavior.                                                                                                                                                                                                                                                                                                                                                                         |
| `portability-avoid-pragma-once`                                 | Enabled | Commit `Enable ten checks the tree already satisfies`: no findings across 460 translation units; a probe confirms it reports.                                                                                                                                                                                                                                                                                                |

## P4 — Cleanup and design consistency

| Check                                                               | Status  | Reason / result                                                                                |
| ------------------------------------------------------------------- | ------- | ---------------------------------------------------------------------------------------------- |
| `readability-braces-around-statements`                              | Pending | Reduce ambiguity in future edits; mostly mechanical churn.                                     |
| `hicpp-braces-around-statements`                                    | Legacy  | Unavailable in LLVM 23; review with `readability-braces-around-statements` on older tools.     |
| `readability-inconsistent-ifelse-braces`                            | Pending | Keep related branches consistently braced.                                                     |
| `readability-avoid-nested-conditional-operator`                     | Pending | Simplify conditional expressions that impede review.                                           |
| `readability-function-cognitive-complexity`                         | Pending | Identify difficult control flow; refactor with behavior coverage.                              |
| `readability-function-size`                                         | Pending | Identify oversized functions; size alone does not establish a defect.                          |
| `google-readability-function-size`                                  | Pending | Alias of `readability-function-size`; handle together.                                         |
| `hicpp-function-size`                                               | Legacy  | Unavailable in LLVM 23; review with `readability-function-size` on older tools.                |
| `readability-magic-numbers`                                         | Pending | Name meaningful constants without naming every literal.                                        |
| `cppcoreguidelines-avoid-magic-numbers`                             | Pending | Alias of `readability-magic-numbers`; handle together.                                         |
| `readability-enum-initial-value`                                    | Pending | Make enum numbering policy explicit; preserve stored/wire values.                              |
| `cert-int09-c`                                                      | Pending | Alias of `readability-enum-initial-value`; handle together.                                    |
| `readability-named-parameter`                                       | Pending | Improve interface documentation through useful parameter names.                                |
| `hicpp-named-parameter`                                             | Legacy  | Unavailable in LLVM 23; review with `readability-named-parameter` on older tools.              |
| `readability-isolate-declaration`                                   | Pending | Separate declarations for clearer initialization and scope.                                    |
| `readability-avoid-unconditional-preprocessor-if`                   | Pending | Remove obsolete scaffolding after platform review.                                             |
| `readability-redundant-nested-if`                                   | Pending | Simplify equivalent conditions without obscuring intent.                                       |
| `readability-trivial-switch`                                        | Pending | Simplify switches where the alternative reads better.                                          |
| `modernize-use-bool-literals`                                       | Pending | Express boolean values directly.                                                               |
| `readability-const-return-type`                                     | Pending | Remove ineffective top-level const on returned values.                                         |
| `clang-diagnostic-ignored-qualifiers`                               | Pending | Remove qualifiers with no effect.                                                              |
| `readability-convert-member-functions-to-static`                    | Pending | Mark operations independent of object state.                                                   |
| `modernize-use-using`                                               | Pending | Modernize type aliases consistently.                                                           |
| `modernize-use-auto`                                                | Pending | Reduce redundant type spelling where deduction stays clear.                                    |
| `hicpp-use-auto`                                                    | Legacy  | Unavailable in LLVM 23; review with `modernize-use-auto` on older tools.                       |
| `modernize-return-braced-init-list`                                 | Pending | Remove redundant return type spelling.                                                         |
| `modernize-use-designated-initializers`                             | Pending | Make aggregate field selection explicit where useful.                                          |
| `readability-uppercase-literal-suffix`                              | Pending | Make literal suffixes less ambiguous.                                                          |
| `cert-dcl16-c`                                                      | Pending | Alias of `readability-uppercase-literal-suffix`; handle together.                              |
| `hicpp-uppercase-literal-suffix`                                    | Legacy  | Unavailable in LLVM 23; review with `readability-uppercase-literal-suffix` on older tools.     |
| `readability-trailing-comma`                                        | Pending | Reduce diff noise in lists; formatting preference.                                             |
| `clang-diagnostic-missing-noreturn`                                 | Pending | Document functions that cannot return.                                                         |
| `clang-diagnostic-nrvo`                                             | Pending | Review missed copy elision; optimize only where worthwhile.                                    |
| `performance-no-int-to-ptr`                                         | Pending | Avoid integer/pointer round trips that hinder optimization.                                    |
| `performance-enum-size`                                             | Pending | Consider storage savings only after packet/recording layout review.                            |
| `clang-diagnostic-padded-bitfield`                                  | Pending | Review wasted bitfield space without changing wire layouts accidentally.                       |
| `clang-diagnostic-ms-bitfield-padding`                              | Pending | Review ABI-dependent bitfield padding before layout changes.                                   |
| `clang-diagnostic-weak-vtables`                                     | Pending | Consider vtable emission/build cost; little gameplay impact.                                   |
| `cppcoreguidelines-avoid-const-or-ref-data-members`                 | Pending | Review assignment restrictions when redesigning affected types.                                |
| `cppcoreguidelines-avoid-non-const-global-variables`                | Pending | Reduce global coupling gradually; substantial architecture work.                               |
| `cppcoreguidelines-non-private-member-variables-in-classes`         | Pending | Improve encapsulation where it adds invariants, not boilerplate.                               |
| `misc-non-private-member-variables-in-classes`                      | Pending | Review exposed mutable state alongside the Core Guidelines rule.                               |
| `misc-multiple-inheritance`                                         | Pending | Review hierarchy complexity; inheritance alone is not a defect.                                |
| `misc-no-recursion`                                                 | Pending | Review recursion depth where inputs can drive it.                                              |
| `cppcoreguidelines-avoid-do-while`                                  | Pending | Consider clearer loops; do-while itself is valid.                                              |
| `clang-diagnostic-global-constructors`                              | Pending | Reduce startup work where worthwhile; blanket removal is expensive.                            |
| `clang-diagnostic-exit-time-destructors`                            | Pending | Review shutdown ordering after initialization dependencies.                                    |
| `clang-diagnostic-lifetime-safety-intra-tu-suggestions`             | Pending | Add useful lifetime annotations after concrete findings are resolved.                          |
| `clang-diagnostic-lifetime-safety-intra-tu-constructor-suggestions` | Pending | Annotate constructor lifetime relationships when useful.                                       |
| `clang-diagnostic-lifetime-safety-cross-tu-suggestions`             | Pending | Extend annotations across translation-unit interfaces.                                         |
| `clang-diagnostic-lifetime-safety-cross-tu-constructor-suggestions` | Pending | Extend constructor annotations across translation units.                                       |
| `clang-diagnostic-date-time`                                        | Pending | Remove timestamp macros if reproducible builds require it.                                     |
| `clang-diagnostic-documentation-unknown-command`                    | Pending | Fix unsupported documentation commands.                                                        |
| `clang-diagnostic-pedantic`                                         | Pending | Review extension use individually against supported compilers.                                 |
| `clang-diagnostic-c99-extensions`                                   | Pending | Review C99 constructs in C++ for portability.                                                  |
| `clang-diagnostic-nested-anon-types`                                | Pending | Review anonymous nested types for portability.                                                 |
| `clang-diagnostic-nullability-extension`                            | Pending | Review compiler-specific nullability syntax.                                                   |
| `misc-confusable-identifiers`                                       | Pending | Avoid visually confusable names; low expected exposure here.                                   |
| `bugprone-copy-constructor-mutates-argument`                        | Covered | Already enforced through `cert-oop58-cpp`; reconcile the excluded name.                        |
| `cert-arr39-c`                                                      | Covered | Already enforced through `bugprone-sizeof-expression`; reconcile the excluded name.            |
| `cert-err33-c`                                                      | Covered | Already enforced through `bugprone-unused-return-value`; reconcile the excluded name.          |
| `cert-exp42-c`                                                      | Covered | Already enforced through `bugprone-suspicious-memory-comparison`; reconcile the excluded name. |
| `cert-oop54-cpp`                                                    | Covered | Already enforced through `bugprone-unhandled-self-assignment`; reconcile the excluded name.    |
| `cppcoreguidelines-c-copy-assignment-signature`                     | Covered | Already enforced through `misc-unconventional-assign-operator`; reconcile the excluded name.   |
| `cppcoreguidelines-explicit-virtual-functions`                      | Covered | Already enforced through `modernize-use-override`; reconcile the excluded name.                |
| `cppcoreguidelines-narrowing-conversions`                           | Covered | Already enforced through `bugprone-narrowing-conversions`; reconcile the excluded name.        |
| `llvm-else-after-return`                                            | Covered | Already enforced through `readability-else-after-return`; reconcile the excluded name.         |
| `llvm-qualified-auto`                                               | Covered | Already enforced through `readability-qualified-auto`; reconcile the excluded name.            |

## P5 — Proposed skips

| Check                                         | Status        | Reason / result                                                                    |
| --------------------------------------------- | ------------- | ---------------------------------------------------------------------------------- |
| `altera-id-dependent-backward-branch`         | Skip proposed | FPGA kernel execution rule; not a game-engine target.                              |
| `altera-struct-pack-align`                    | Skip proposed | FPGA layout tuning conflicts with general-purpose layout needs.                    |
| `altera-unroll-loops`                         | Skip proposed | FPGA loop-unrolling policy; no relevant target.                                    |
| `android-cloexec-accept`                      | Skip proposed | Low current priority; revisit descriptor inheritance if process launching expands. |
| `android-cloexec-fopen`                       | Skip proposed | Low current priority; close-on-exec can matter on desktop Unix too.                |
| `android-cloexec-open`                        | Skip proposed | Low current priority; revisit file descriptor leakage across exec.                 |
| `android-cloexec-socket`                      | Skip proposed | Low current priority; revisit socket inheritance across exec.                      |
| `llvm-header-guard`                           | Skip proposed | LLVM naming convention differs from this project's guards.                         |
| `llvm-include-order`                          | Skip proposed | LLVM-specific include ordering adds little to existing tooling.                    |
| `llvm-prefer-static-over-anonymous-namespace` | Skip proposed | LLVM-specific linkage preference; no clear project benefit.                        |
| `llvm-use-ranges`                             | Skip proposed | Prefer the general ranges check over LLVM-library transformations.                 |
| `llvmlibc-callee-namespace`                   | Skip proposed | Applies to LLVM libc implementation, not this project.                             |
| `llvmlibc-implementation-in-namespace`        | Skip proposed | Applies to LLVM libc implementation, not this project.                             |
| `llvmlibc-inline-function-decl`               | Skip proposed | LLVM libc declaration policy is unrelated to this codebase.                        |
| `llvmlibc-restrict-system-libc-headers`       | Skip proposed | This project legitimately uses system C library headers.                           |
| `boost-use-ranges`                            | Skip proposed | No reason to introduce Boost ranges when standard/Abseil facilities suffice.       |
| `fuchsia-default-arguments-calls`             | Skip proposed | Blanket default-argument ban offers little project value.                          |
| `fuchsia-default-arguments-declarations`      | Skip proposed | Blanket default-argument ban offers little project value.                          |
| `fuchsia-multiple-inheritance`                | Skip proposed | Duplicates the general inheritance design review.                                  |
| `fuchsia-overloaded-operator`                 | Skip proposed | Blanket operator-overload ban is unsuitable for game utility types.                |
| `fuchsia-statically-constructed-objects`      | Skip proposed | Fuchsia static-object policy is too broad for this engine.                         |
| `fuchsia-trailing-return`                     | Skip proposed | Fuchsia syntax policy adds no correctness protection.                              |
| `google-default-arguments`                    | Skip proposed | Blanket default-argument restrictions have low expected value here.                |
| `google-readability-todo`                     | Skip proposed | TODO ownership syntax is administrative style.                                     |
| `misc-predictable-rand`                       | Skip proposed | Simulation randomness must remain deterministic; audit security uses separately.   |
| `cert-msc30-c`                                | Skip proposed | Alias of `misc-predictable-rand`; handle together.                                 |
| `cert-msc50-cpp`                              | Skip proposed | Alias of `misc-predictable-rand`; handle together.                                 |
| `clang-analyzer-security.insecureAPI.rand`    | Skip proposed | Cryptographic RNG advice has low value for deterministic gameplay.                 |
| `modernize-use-trailing-return-type`          | Skip proposed | Large syntax-only rewrite with no clear readability gain.                          |
| `readability-identifier-length`               | Skip proposed | Short coordinates and loop indices are often appropriate.                          |
| `clang-diagnostic-padded`                     | Skip proposed | Padding is normal; blanket packing risks performance and layout compatibility.     |
| `clang-diagnostic-pre-c++17-compat`           | Skip proposed | The project requires C++23, so older-standard compatibility is unnecessary.        |
| `clang-diagnostic-pre-c++20-compat`           | Skip proposed | The project requires C++23, so older-standard compatibility is unnecessary.        |
| `clang-diagnostic-pre-c++23-compat`           | Skip proposed | The project requires C++23, so older-standard compatibility is unnecessary.        |

## Completing a row

### StatusOr check toolchain limitation (2026-09-11)

`abseil-unchecked-statusor-access` is available, but crashes with SIGSEGV in
`clang::dataflow::statusor_model::getSyntheticFields` on both games' `Read_PCX_File`, before and
after guarding the byte reads. The following checked access also reproduces it with LLVM 23.1.2 and
the project's Abseil 20260107.0 headers:

```cpp
#include "absl/status/statusor.h"
int ReadChecked(const absl::StatusOr<int>& value) {
  if (!value.ok()) return 0;
  return *value;
}
```

Save this as `/tmp/statusor-tidy-probe.cc` and run from the repository root:

```sh
clang-tidy /tmp/statusor-tidy-probe.cc --checks='-*,abseil-unchecked-statusor-access' -- \
  -std=c++23 -isystem cmake-build-strict-ra-clang/_deps/abseil-cpp-src
```

Both PCX readers now reject failed pixel and trailing-data reads with `nullptr`, freeing the image
buffer before returning. Regression tests exercise literal/RLE success and truncation, including
trailing scanline bytes, through the real TD reader. The check remains excluded because enabling it
would crash strict builds; a crash is not a clean scan.

The complete candidate sweep covered 879 project translation units (including 428 generated header
checks): only the two PCX readers crashed, with no other findings. The full-config sweep passed with
the check still excluded. Strict builds of both games and all 205 CTest tests passed. The new
missing-pixel regression test aborts against the original TD reader with an unchecked `OUT_OF_RANGE`
access and passes with the fix.

### Missing-default check policy (2026-09-11)

`bugprone-unhandled-code-paths` remains excluded after review. The isolated sweep of 889 project
translation units, including 431 generated header checks, produced 61 distinct missing-default
diagnostics across 54 translation units. This is a policy decision, not a clean candidate scan or a
claim that every reported switch has been proven correct.

Representative findings show why requiring this check would add little value here:

- TD `Text_String` handles special strings in a switch and then returns the normal string-table
  lookup. TD `RadarClass::Click_Cell_Calc` similarly delegates other results to the base class after
  its switch. Both already have explicit fallbacks.
- `Base64_Decode` switches on a counter constrained by its four-character packet loop; TD
  `Shake_Screen` switches on a random value constrained to -1, 0, or 1. Both list their reachable
  cases.
- RA menu navigation handles wrapping and unavailable expansion buttons in a switch, then uses other
  button indices unchanged. Its missing `default` is not a missing navigation action.

A minimal switch with two returning cases followed by an unconditional fallback return reproduces
the warning in LLVM 23.1.2. The installed default `WarnOnMissingElse=false` also means this scan
does not assess missing final `else` branches; see the
[LLVM check documentation](https://releases.llvm.org/23.1.0/tools/clang/tools/extra/docs/clang-tidy/checks/bugprone/unhandled-code-paths.html).
Keep this excluded rather than enforce empty defaults or move valid fallbacks solely to satisfy the
syntax rule. No source or configuration changes were made. The excluded-name count remains 230.
Review actual enum coverage separately with the pending compiler switch diagnostics.

### Enum-cast range check policy (2026-09-11)

`clang-analyzer-optin.core.EnumCastOutOfRange` remains excluded after review. The isolated sweep of
889 project translation units, including 431 generated header checks, produced findings in 141
translation units: 300 distinct diagnostic messages at 45 source locations. Different enum types and
values at shared operator templates account for many repeated locations. This is a policy decision,
not a clean candidate scan or proof that every reported cast is correct.

The checker compares cast inputs against named enumerator values, which does not match several
intentional representations in this code:

- Both games use `DirType : uint8_t` for 256-step directions with only selected angles named.
  Nuclear missile placement casts angle 28; TD aircraft searches use angles in steps of 16.
  `Desired_Facing256` also returns intermediate angles. These values need not be named enumerators.
  `Desired_Facing8` can return 256, which converts through the fixed unsigned-byte underlying type
  to north (0).
- `TextPrintType`, `ThreatType`, gadget flags, keyboard modifiers, and graphics-buffer flags combine
  values through enum operators. For example, combining `GBC_VIDEOMEM` (1) and `GBC_VISIBLE` (2)
  yields the intentional unnamed value 3.
- Both path optimizers use the representable `FacingType : int8_t` value -2 to mark removed commands
  before compacting the list. Negative values also encode relative turns, which `Next_Direction`
  wraps into the eight directions. Named sentinel and integer turn-delta types could clarify this
  code independently, but these reports alone do not establish invalid movement.

A minimal reproduction under the installed LLVM 23.1.2 reports the legal intermediate angle below:

```cpp
enum Direction : unsigned char { North = 0, East = 64, South = 128, West = 192 };
Direction intermediate() { return static_cast<Direction>(28); }
```

Separate probes also report unsigned-byte wrapping from 256 to 0, a flag combination of 1 and 2, and
a signed-byte sentinel of -2. Annotating the flag enum with `[[clang::flag_enum]]` removes its
warning, consistent with the
[LLVM checker implementation](https://clang.llvm.org/doxygen/EnumCastOutOfRangeChecker_8cpp_source.html).
That annotation does not describe continuous angles or signed path commands. Enabling this check
would require additional suppressions or type changes to accommodate valid representations. Retain
the exclusion and revisit if the checker gains suitable range semantics or those types are
redesigned. No source or configuration changes were made; the excluded-name count remains 229.
Markdown formatting and whitespace checks passed; game builds and tests were not rerun for this
documentation-only decision.

### Container invalidation check policy (2026-09-11)

`clang-diagnostic-lifetime-safety-invalidation` remains excluded after review. The isolated sweep of
890 project translation units, including 431 generated header checks, produced 13 findings in 9
files. Eight were real and are fixed; the remaining five are a blanket false positive that makes the
check unusable as it stands.

In LLVM 23.1.2 the diagnostic treats any mutating member call as invalidating the container object
itself, rather than the references and iterators into it. Two consecutive `push_back` calls are
enough:

```cpp
#include <vector>
void f(std::vector<int>& v) {
  v.push_back(1);
  v.push_back(2);
}
```

```sh
clang++ -std=c++23 -Wlifetime-safety-invalidation -fsyntax-only /tmp/inval-probe.cc
```

Assigning to a `std::string&` and then reading it, and `resize` followed by `std::ssize`, reproduce
it the same way. That accounts for all five remaining findings: the byte-appending helpers in
`ra/dib_test.cc` and `winvq/vqa32/vqaplay_test.cc`, `TriggerTypeClass::Build_INI_Entry` assigning
its output string before appending to it, and the cached credits frames in `ra/conquer.cc`. Enabling
the check would require rewriting ordinary container code. Retain the exclusion and revisit when the
diagnostic distinguishes a container from references into it.

The eight genuine findings were a separate pattern the checker reports well: a global left holding a
freed pointer. Both games' full-screen sequences allocate `PseudoSeenBuff`, `TextPrintBuffer`,
`BackgroundPage`, and TD's `Palette` into globals and delete them on the way out without clearing
the pointer. Two of the five screens already cleared `TextPrintBuffer`, so the convention existed
and the other sites had drifted from it; every site now follows it. No current reader sees a
dangling value, because each screen assigns before use.

TD's `Map_Selection` also allocated `PseudoSeenBuff` and never released it, leaking a 320x200 buffer
per visit. The checker cannot see a leak, so this was found while tracing the same four globals; it
is fixed alongside them because it is the missing half of the same cleanup.

The final isolated sweep reports only the five container false positives, and the full-config sweep
passed all 890 translation units with the check still excluded. Both strict game builds and all 237
CTest tests passed. The RA save/load smoke check matched 240 object positions; the TD checks matched
5,742 to 6,371 game states across all eight fixtures. The excluded-name count remains 221.

### Throwing static initialization review (2026-09-12)

`bugprone-throwing-static-initialization` and its alias `cert-err58-cpp` are now enforced. The
isolated sweep of 890 project translation units, including 431 generated header checks, started at
1,424 findings: every static object in the engine, because the check reports a static whose
constructor, or any function in its initializer, is not `noexcept`.

The engine has no `throw` statement, but exceptions are not unused either: `MixFileClass::Cache`
catches `std::bad_alloc` and falls back to reading from disk. So `std::bad_alloc` is a real path,
and during dynamic initialization there is nothing above `main` to catch it.

Most of the 1,424 were not allocating at all. 1,279 were the game-data type tables — the
`TemplateTypeClass`, `AnimTypeClass`, `BuildingTypeClass` and sibling objects in `cdata.cc`,
`adata.cc`, `bdata.cc` and the rest — whose constructors only assign members, `strncpy` an INI name,
or write a fixed-size array. The remainder of the reports came from the same tables through their
argument expressions, most of them `fixed` (1,589 notes on its constructors alone) and the
coordinate helpers in `inline.h`. Marking those `noexcept` is accurate, not a suppression, and it
holds the invariant that building the type tables allocates nothing:

- Both games' `AbstractTypeClass` → `ObjectTypeClass` → `TechnoTypeClass` chain and all twelve leaf
  type classes (31 constructors).
- The value types used to build the tables: `fixed`, `RGBClass`, `HSVClass`, `RandomClass`, `PKey`,
  `Int`, `GenericList`, `XY_Coord`/`XYP_COORD` and the `operator|` that combines flag enums.
- `FixedHeapClass`/`FixedIHeapClass`/`TFixedIHeapClass`, `QueueClass`, `IPXAddressClass`,
  `PaletteClass`, and the timers (`TimerClass`, `CountDownTimerClass`, `Timer`, `Stopwatch`,
  `TCountDownTimerClass`).
- The gadget chain behind the sidebar, radar, power bar and tactical map buttons: `LinkClass`,
  `GadgetClass`, `ControlClass`, `ToggleClass`, `ShapeButtonClass` and the nested button classes.

That left 66 findings, all of them the engine's singleton globals: the screen buffers
(`GraphicBufferClass`, `GraphicViewPortClass`), the dynamic vectors behind the trigger, player and
phone lists, the INI parsers, the network managers, the mono debug pages, the keyboard, the file
handle table, and the map/display/logic chain that owns vectors of its own. These do allocate while
constructing. They are defined once per game in `globals.cc`, and their construction order is load
bearing, so they cannot move into function-local statics without an initialization-order rework that
is out of scope here. The check's own `AllowedTypes` option names those 25 types in
[`.clang-tidy`](../.clang-tidy), which keeps the check live for a static of any other type —
including a new one added to `globals.cc`. The alias does not share the option, so
`cert-err58-cpp.AllowedTypes` carries the same list.

Both isolated sweeps now report nothing for either name, and the full-config sweep passes all 890
translation units. Both strict game builds are clean and all 237 CTest tests pass. The excluded-name
count drops from 218 to 216.

### Local initialization check policy (2026-09-12)

`cppcoreguidelines-init-variables` remains excluded after review. The isolated sweep of 890 project
translation units, including 431 generated header checks, produced 2,998 findings across both games
and the shared libraries — 738 of them pointer declarations. Every sampled finding is the same 1990s
construct: locals declared at the top of a block and assigned before they are read (`int x, y;` in
`ra/scenario.cc`, `int num;` in `td/team.cc`, `const TechnoTypeClass* otype;` in `td/teamtype.cc`).

The problem is not the volume, it is that the check's fix works against the checks already in place.
Eleven checks that do report a genuine uninitialized read are enforced today:
`clang-analyzer-core.uninitialized.ArraySubscript`, `.Assign`, `.Branch`, `.NewArraySize`,
`.UndefReturn`, `clang-analyzer-core.UndefinedBinaryOperatorResult`,
`clang-analyzer-core.CallAndMessage`, `clang-analyzer-optin.cplusplus.UninitializedObject`,
`clang-analyzer-unix.cstring.UninitializedRead`, and the `uninitialized` and
`conditional-uninitialized` compiler diagnostics. An isolated sweep of all eleven over the same 890
translation units reports nothing, so the declare-then-assign style is not currently hiding a real
uninitialized read anywhere in the tree.

Adding an initializer silences those checks without fixing anything. Both halves of this probe
reproduce in LLVM 23.1.2:

```cpp
int Missing_Else(bool c) {
  int x;
  if (c) {
    x = 1;
  }
  return x;  // reported by clang-analyzer-core.uninitialized.UndefReturn
}
int Zero_Initialized(bool c) {
  int x = 0;  // satisfies cppcoreguidelines-init-variables; nothing is reported
  if (c) {
    x = 1;
  }
  return x;  // still the wrong value when c is false
}
```

```sh
clang-tidy --checks='-*,cppcoreguidelines-init-variables,clang-analyzer-core.uninitialized.UndefReturn' \
  /tmp/probe.cc -- -std=c++23
```

The first function draws both warnings; the second draws none, and the missing `else` survives. So
applying this check across 2,998 sites would trade a working defect check for a declaration style
rule, and would have to be audited site by site to avoid writing a zero over a path that should have
assigned something else.

The valuable version of this cleanup is declaring each local at its point of first use, which
shortens the live range and removes the reports honestly. That is a restructuring of 2,998 sites in
long legacy functions, not a mechanical annotation, and it belongs with whatever modernization
touches those functions rather than with a tidy sweep. The check has no option to narrow its scope —
`IncludeStyle` and `MathHeader` only configure its NaN fix-it — so there is no smaller enforceable
subset, not even the 738 pointers.

Retain the exclusion. No source or configuration changes were made; the excluded-name count
remains 216.

### Special member function review (2026-09-12)

`cppcoreguidelines-special-member-functions` is now enforced, and the
`hicpp-special-member-functions` exclusion is removed alongside it. The isolated sweep of 890
project translation units, including 431 generated header checks, produced 249 findings covering 111
distinct classes at 170 declaration sites (91 RA, 71 TD, 8 shared). Every finding is a class that
declares a destructor -- usually a virtual one -- and leaves copying and moving to the implicit
rules.

The engine's classes fall into groups where copying is meaningless: the battlefield object hierarchy
from `AbstractClass` down through `TechnoClass`, `FootClass` and the leaf unit classes, whose
identity is their slot in a fixed heap; the gadget hierarchy behind the sidebar, radar and dialog
controls, which is threaded onto linked lists; the file, INI and network classes, which own handles
and buffers; and the Win32 COM interface shims. Those sites now declare the copy and move operations
deleted next to the destructor, which is the spelling already used in `Straw`, `RandomStraw` and
`GadgetClass`.

Deleting a copy that is actually performed is a compile error, so the build located every exception
rather than leaving it to inspection. There were five:

- `HouseTypeClass::Init_Heap` copy-constructs its heap entries from the static house prototypes, so
  `AbstractTypeClass` and `HouseTypeClass` keep a defaulted copy constructor. `ObjectTypeClass`
  already declared defaulted moves, so the base keeps defaulted moves too; deleting them made the
  derived declarations ill-formed, which `clang-diagnostic-defaulted-function-deleted` reported on
  the first strict build.
- Both games' `CellClass` is copy-assigned by `VectorClass` when the map array grows, so assignment
  is defaulted while construction stays deleted. Each also carried a private undeclared copy
  constructor, the pre-C++11 spelling of the same intent; that is now a plain `= delete` beside the
  other four.
- TD's `TCountDownTimerClass` is assigned from a tick count (`SightTimer = kTicksPerSecond`), which
  needs the implicit move assignment its empty destructor was suppressing. Deleting that empty
  destructor restores the value semantics the callers rely on and takes the class off the check's
  list entirely.

Two smaller traps came out of the same builds. A deleted copy constructor suppresses the implicit
default constructor, which broke `VqaIo` and the three COM interface shims until each regained an
explicit `= default` default constructor. And seven classes declare their destructor through a macro
or in a `.cc` file, so they were annotated by hand: the three WOL event sinks behind
`COM_SINK_DESTRUCTOR`, both games' `BufferedFileReader`, `VQAHandle`, and `IconsetClass` -- a
reinterpret-cast overlay on shape data that is never constructed, now saying so with deleted members
rather than private undeclared ones.

The isolated sweep now reports nothing, the full-config strict build of both games is clean, and all
237 CTest tests pass. The excluded-name count drops from 216 to 214.

### Deprecated implicit copy review (2026-09-12)

All three `clang-diagnostic-deprecated-copy-*` names are now enforced. The isolated sweep of 890
project translation units reports nothing, and the preceding commit is why.

Measured against the commit before it, the sweep found four reports, and they are exactly the four
classes that commit had to resolve by hand rather than by deleting copies:

| Report                                              | Class                                   |
| --------------------------------------------------- | --------------------------------------- |
| implicit copy assignment, user-provided destructor  | `ra/cell.h` and `td/cell.h` `CellClass` |
| implicit copy assignment, user-provided destructor  | `td/ftimer.h` `TCountDownTimerClass`    |
| implicit copy constructor, user-declared destructor | `ra/type.h` `AbstractTypeClass`         |

These diagnostics fire only where an implicit copy is actually defined, so declaring copy and move
explicitly on every class that owns a destructor retired all four at once: `CellClass` now defaults
its copy assignment and deletes construction, `AbstractTypeClass` defaults its copy constructor for
`HouseTypeClass::Init_Heap`, and `TCountDownTimerClass` lost the empty destructor that was
suppressing its implicit operations.

Zero findings here is a real result rather than a silent no-op. All three diagnostics reproduce on a
probe under the same invocation, one per flag:

```cpp
struct UserDtor { ~UserDtor() {} };
struct UserCopy { UserCopy() = default; UserCopy(const UserCopy&) {} };
struct DefaultedDtor { ~DefaultedDtor() = default; };
void Use() {
  UserDtor a; UserDtor b = a;              // deprecated-copy-with-user-provided-dtor
  UserCopy c, d; d = c;                    // deprecated-copy-with-user-provided-copy
  DefaultedDtor e; DefaultedDtor f = e;    // deprecated-copy-with-dtor
  (void)b; (void)f;
}
```

```sh
clang-tidy --checks='-*,misc-unused-using-decls,clang-diagnostic-deprecated-copy-with-user-provided-copy,clang-diagnostic-deprecated-copy-with-user-provided-dtor,clang-diagnostic-deprecated-copy-with-dtor' \
  /tmp/dep.cc -- -std=c++23 -Weverything
```

A `clang-diagnostic-*` filter needs its warning flag and at least one real clang-tidy check in the
same run, which is what `misc-unused-using-decls` supplies above; `-Weverything` is already in the
strict compile database.

The full-config sweep passes all 890 translation units, both strict game builds are clean, and all
237 CTest tests pass. No source changes were needed. The excluded-name count drops from 214 to 211.

### Macro parentheses review (2026-09-12)

`bugprone-macro-parentheses` is now enforced. The isolated sweep of 890 project translation units
produced 85 findings at macro definition sites: 60 replacement lists and 25 macro arguments. They
split into two shapes.

Sixty are object-like macros whose body is an unparenthesized expression, and they are dominated by
two patterns: negative sentinels (`INVALID_SOCKET -1`, `WWERROR -1`, the thirteen `VQAERR_*` codes,
`IFFERR_*`, `NO_CD_DRIVE`, `VSS_ID`, `CHAT_CHANNEL_LIST_ALL`) and arithmetic constants
(`OPTION_WIDTH 236 * 2`, `NUKE_GONE_TIME 14 * kTicksPerMinute`, `SOCKET_BUFFER_SIZE 1024 * 128`, the
nine `TXT_WINSOCK_* 4567 + n` string numbers, `MODEM_NAME_MAX PORTBUF_MAX - 1`).

Twenty-five are function-like macros that do not parenthesize their parameters. These are the ones
with real teeth: both games' `XYCELL(x, y)` expanding to `y * MAP_CELL_W + x`, the six date field
macros in `tech/wwfile.h`, `BLOCK_DIM`, `VQAFRAME_OFFSET`, `Reverse_LONG` in `tech/sha.cc`, and
`RP_SET`/`RP_INCR`/`MK_PTR` in the VESA real-mode helpers.

The fixes are the check's own fix-its, exported per translation unit and applied with
`clang-apply-replacements`, so the parenthesization is the tool's rather than hand-written:

```sh
clang-tidy -p "$BUILD" --checks='-*,bugprone-macro-parentheses' --export-fixes=fixes/$n.yaml <tu>
clang-apply-replacements-23 --format=false --style=none fixes
```

No current expansion changes meaning, which was checked rather than assumed. For the object-like
macros, every use in a larger expression still groups the same way: `10 + MODEM_NAME_MAX` and
`OPTION_Y + OPTION_HEIGHT - 15` are unaffected because `-` and `/` already bound as the parentheses
now say, and the rest are used as plain array bounds, comparisons or single arguments. For the
function-like macros, no call site passes a compound argument: `XYCELL` is called only with integer
literals, where unary minus already binds tighter than `*`; `BLOCK_DIM(header->BlockWidth, ...)` and
`VQAFRAME_OFFSET(vqabuf->Foff[i])` pass member and subscript expressions, which bind tighter than
the `&` and `<<` inside; `Reverse_LONG((length * 8))` was already parenthesized by its caller; and
the `wwfile.h` date macros and `RP_INCR` have no call sites at all. So this commit removes latent
hazards rather than fixing live bugs.

Applying the fix-its lengthened some definitions past the column limit, which pulled their
neighbours' trailing-comment alignment out of true. Only the disturbed ranges were reformatted
(`clang-format --lines`), and both games' `MODEM_NAME_MAX` moved its comment above the definition
rather than let clang-format split the replacement list across a line continuation. The touched
files gain no formatting violations they did not already have.

The isolated and full-config sweeps now report nothing, both strict game builds are clean, and all
237 CTest tests pass. The excluded-name count drops from 211 to 210.

### Operator precedence parentheses review (2026-09-12)

The three `clang-diagnostic-*-op-parentheses` names are now enforced. The isolated sweep of 890
project translation units produced 25 findings: 17 for `&` inside `|`, 6 for `&&` inside `||`, and 2
for `+` inside `<<`. (`readability-math-missing-parentheses` was measured in the same run and
produced 2,532; it was handled separately in the review below.)

The two shift findings are real bugs, in TD only:

```cpp
WindowList[window][WINDOWX] << 3 + LogicPage->Get_XPos(),
WindowList[window][WINDOWY] + LogicPage->Get_YPos(),
```

`+` binds tighter than `<<`, so this shifted the window's X origin left by `3 + Get_XPos()` places
instead of converting the 8-pixel units and then adding the viewport offset. The `WINDOWY` line
immediately below shows the intent, `cdata.cc`, `sdata.cc` and `dialog.cc` all write
`WindowList[window][WINDOWX] << 3` and add separately, and RA's counterpart stores `WINDOWX` in
pixels and simply adds. Both sites — `td/conquer.cc` in the shape-drawing helper and `td/techno.cc`
in `Draw_It` — are now `(WindowList[window][WINDOWX] << 3) + LogicPage->Get_XPos()`. The bug is
invisible whenever `Get_XPos()` is zero, which is the usual case for a full-screen logic page.

The other 23 are parenthesized to preserve the grouping they already had, applied from the check's
own fix-its. Most are plainly intentional once written out: the SHA-1 round functions in
`tech/sha.h` (`Z ^ (X & (Y ^ Z))` and `(X & Y) | (Z & (X | Y))` are the standard forms), the cell
icon and coordinate packing in both games' `cell.cc` and TD's `Coord_Snap`, TD's "all units
destroyed" trigger masks, and the `||`-of-`&&` conditions in `Init_Random`, `MapClass::Logic`,
`TeamClass::AI` and `InfantryClass::Assign_Target`.

Three groupings looked like they may not match intent, and were left exactly as they were in that
commit rather than changed alongside a parentheses cleanup. They were then investigated separately
and resolved in commit `Fix the shape header cache key and two mis-grouped conditions`:

- `ra/house.cc` `Update_Spied_Power_Plants` read
  `(!IsOwnedByPlayer && *bldg == STRUCT_POWER) || *bldg == STRUCT_ADVANCED_POWER`. The ownership
  filter is meant to apply to both plant types, and the regrouping is safe: the only case the two
  forms disagree on is a player-owned advanced power plant, and the loop body is
  `bldg->Mark(MARK_CHANGE)`, which `ObjectClass::Mark` resolves to `Mark_For_Redraw()` alone. No
  simulation state is touched, so nothing can desync, and the divergence needs the player's own spy
  bit on their own building besides. Regrouped to `!IsOwnedByPlayer && (POWER || ADVANCED)`.
- `ra/vessel.cc` `What_Action` tested
  `(In_Radar(cellnum) && Cost[SPEED_FOOT] == 0) || Occupy.Building || ...` for an unsuitable unload
  cell. `MapClass::In_Radar` is a pure playfield-bounds test — out-of-range cell number, then left
  or right of `MapCellX`/`MapCellWidth`, then top or bottom — with no shroud involved, so it is the
  validity guard for the whole test rather than one term of it. The same occupancy list appears
  without an `In_Radar` term at `vessel.cc:1580` and `unit.cc:3706`, where the cell is already known
  good. As written a cell outside the playfield counted as suitable whenever its occupancy flags
  happened to be clear, which offered the unload action to a loaded transport sitting against the
  map border. Now `!In_Radar(cellnum) || impassable || occupied`.
- `tech/2keyfbuf.cc` compared `draw_flags` against
  `(flags & SHAPE_TRANS) | SHAPE_FADING | SHAPE_PREDATOR | SHAPE_GHOST`. This one is a definite bug,
  and the proof is nine lines up in the same file: `Setup_Shape_Header` _stores_
  `flags & (SHAPE_TRANS | SHAPE_FADING | SHAPE_PREDATOR | SHAPE_GHOST)`. The reading expression adds
  the other three bits unconditionally, so the stored key could only equal the computed one when a
  draw requested all four effects at once. Every other draw saw a mismatch, re-ran
  `Setup_Shape_Header` — a full per-pixel pass over the shape — and took the all-flags blit path.
  Both sides now call one `ShapeEffectFlags()` helper in `2keyfbuf.h`, and `tech/2keyfbuf_test.cc`
  pins it: all five of its tests fail against the old expression and pass against the helper.

Applying the fix-its pushed a few conditions past the column limit; only those ranges were
reformatted, and the touched files gain no formatting violations they did not already have.

Both isolated and full-config sweeps now report nothing, both strict game builds are clean, and all
237 CTest tests pass. The excluded-name count drops from 210 to 207.

### Arithmetic precedence parentheses review (2026-09-12)

`readability-math-missing-parentheses` is now enforced. The isolated sweep of the 460 project
translation units produced 2,513 unique sites. The check has no options, so it is all-or-nothing:

| Grouping                                         | Sites |
| ------------------------------------------------ | ----- |
| `*` `/` `%` inside `+` `-` (ordinary precedence) | 2,458 |
| `+` `-` inside `&` `^` (arithmetic and bitwise)  | 55    |

All 55 mixed arithmetic/bitwise sites were read individually and none is a bug. They are deliberate
engine idioms: `(Tail + 1) & (size - 1)` ring wrap in both games' `queue.h` and the keyboard buffer,
`(n + 16) & 0xFFFFFFF0` rounding in the radar and the VQA loader, the Blowfish F-function
`((S0[a] + S1[b]) ^ S2[c]) + S3[d]`, and `celljammed & (0xFFFF - housebit)`, which clears one house
bit by subtracting it from an all-ones mask. Adding the parentheses makes each of those readable
without changing any of them. This is the Core Guidelines ES.41 subset, and the reason the check
earns its place despite the bulk of the findings being ordinary precedence.

The check's fix-its were applied with `run-clang-tidy -fix`, then only the changed lines were
reformatted with `git clang-format`. Sixteen sites needed hand edits because the fix-it landed
inside a macro expansion and was discarded: thirteen inside `EXPECT_EQ` arguments in five test
files, and three `sizeof(a) / sizeof(a[0]) - 1` and `(BuildLevel - 1) / 3 + 1` expressions in TD. A
further 128 findings across seven TD files came from one macro body, `XYP_COORD` in
[td/inline.h](../src/td/inline.h); parenthesizing its division retired all of them at once. RA has
no counterpart because its `XYP_COORD` is already an inline function.

Because the diff is large, the absence of behavior change was verified rather than assumed. Every
changed file is textually identical to its `HEAD` version once parentheses and whitespace are
removed, and all 181 changed translation units were compiled to `-O2` LLVM IR from both trees: 172
are bit-identical, and the nine that differ do so only in `__assert_fail` and GoogleTest line-number
constants, which moved when lines were reflowed.

The full-config strict build of both games is clean, the isolated sweep now reports nothing, and all
242 CTest tests pass. The excluded-name count drops from 207 to 206.

### Branch clone review (2026-09-12)

`bugprone-branch-clone` is now enforced. The check has no options. The isolated sweep of the 460
project translation units produced 43 findings, in both games and in `sdllib` and `winvq`:

| Finding                                  | Sites |
| ---------------------------------------- | ----- |
| Consecutive identical `switch` branches  | 26    |
| Repeated body in an `if`/`else if` chain | 13    |
| `if` with identical then and else        | 4     |

Every site was read against its surroundings, and where the duplication looked like it might hide a
slip, against EA's original source in the initial commit. None is a copy/paste bug. The ones worth
recording:

- `ra/display.cc` `Mouse_Left_Up` shows `MOUSE_NORMAL` for both `ACTION_DAMAGE` and `ACTION_GREPAIR`
  in its shadow branch, while the unshadowed switch below uses `MOUSE_DAMAGE` and `MOUSE_GREPAIR`.
  That reads like a missed edit, but it is EA's code verbatim: over shroud the cursor does not
  reveal what is underneath, the same reason `ACTION_NONE` is normal there too.
- RA's message dialogs (`netdlg.cc` and `nulldlg.cc`, four sites) handle `MessageListClass::Input`
  results 1 (refresh) and 2 (redraw) identically. TD distinguishes them, drawing for 1 and setting
  `REDRAW_MESSAGE` for 2; RA's own comment explains it only needs to redraw the edit box for either.
- `winvq/vqa32/audio.cc` `VQA_StopTimerInt` clears the shared `AudioFlags` timer bits in both
  branches. EA's DOS player cleared the caller's `audio->Flags` in the else branch, but the Windows
  player this port descends from already cleared the shared flag both ways. `VQA_StartTimerInt` has
  no callers, so the use count is always zero; the branch is collapsed and the DOS difference noted.
- `ra/wolapiob.cc` sends "back" from both `WOL_LEVEL_GAMESOFTYPE` and `WOL_LEVEL_LOBBIES` to the
  games list, consistent with `WOL_LEVEL_INLOBBY` going back to the lobbies list.
- `td/cell.cc` `Incoming` scattered infantry and other objects identically; a commented-out
  `Scatter(threat, false)` shows the non-infantry case once differed. Collapsed to one call.

The switch findings became stacked case labels, including `default:` where an explicit case only
broke out. Two needed more than stacking. TD's cheat-key parameters in `init.cc` were each wrapped
in an `#ifdef` of their own `PARM_*` name, all of which `td/defines.h` defines unconditionally, so
the guards were dropped. TD's main menu gave four buttons an extra `retval += 1` under `DEMO`; the
merged body applies it to every button except Start, which is exact for every configuration that
compiles (a `DEMO` build already cannot, because `BONUS_MISSIONS` is unconditional and
`BUTTON_BONUS` exists only under `NEWMENU`).

The chain findings were joined with `||`, keeping each condition's order so short-circuiting still
skips the same calls (`NullModem.Detect_Port`, `Init_Null_Modem`, `strcmp`). The `NET_PING` branch
in both games' `Get_Join_Responses` set the same `EV_NONE` as the default and was removed, with its
explanation moved into the default comment. `sdllib/wsa.cc` chose between the minimum and maximum
animation buffer in four branches; it is now one condition, from disk or a nonzero size below the
maximum, with the original rules restated in its comment.

A probe with identical then and else branches confirms the check reports an error under the
repository configuration. The isolated sweep now reports nothing, both strict game builds are clean,
and all 242 CTest tests pass. The excluded-name count drops from 206 to 205.

### Explicit instantiation declarations (2026-09-12)

`clang-diagnostic-undefined-func-template` is now enforced, completing the `-undefined-var-template`
fix above. The field-wise savegame code, `CCPtr`, TD's vectors and `ObjectPtr` define their
templates in one `.cc` file and explicitly instantiate them there, but no header said so, and every
other translation unit that used them instantiated a declaration with no definition. The sweep's 121
locations understate the work: one line in `ra/heap.h` or `tech/archive.h` instantiates `Serialize`
for dozens of classes.

Each explicit instantiation definition now has an `extern template` declaration in the header of the
class it instantiates, generated from the instantiation lines themselves so the two lists cannot
drift: 128 member-template declarations across 71 headers, plus the `CCPtr`, vector and `ObjectPtr`
class instantiations and both games' `SerializeObjectList`. The `CCPtr<T>::Heap` specializations
defined in `globals.cc` are declared in `ccptr.h`, which also removes a use of an explicit
specialization before its declaration. TD's `DynamicVectorClass<int>::Delete(const int&)`
specialization moved ahead of the declarations, since a specialization must precede the
instantiation it replaces.

### Explicit constructor review (2026-09-12)

`misc-explicit-constructor` and its two aliases are now enforced. The isolated sweep reported 125
single-argument constructors and conversion operators. The check's fix-its made all 125 explicit; a
compile of both games then failed at 3,738 call sites, and every failure traced to a type whose
implicit conversion is the point of the type:

| Kept implicit                                       | Why                                                  |
| --------------------------------------------------- | ---------------------------------------------------- |
| `operator StructType()` and the other type IDs      | Objects compare directly against their type ID.      |
| `CCPtr(T*)`, `CCPtr::operator T*`                   | A `CCPtr` stands in for the raw object pointer.      |
| `TargetClass` constructors, `operator RTTIType`     | Targets convert from every addressable thing.        |
| `FacingClass(DirType)`, `operator DirType`          | A facing reads and assigns as its direction.         |
| `TCountDownTimerClass(int64_t)`, `operator int64_t` | A countdown assigns and reads as its tick count.     |
| `ActionChoiceClass`, `EventChoiceClass`             | Choice tables are brace lists of action/event types. |
| Palette, buffer, file-name and `Int` pointers       | Legacy C interfaces take the object as a pointer.    |

Those 56 declarations keep their conversion under a `NOLINTNEXTLINE(*-explicit-constructor)` that
states the reason. The other 69 stay explicit with no call site changes: file, pipe and straw
constructors, heaps, vectors, dialogs, and the game-object constructors that take a type or house.

### Variadic, thread-safety and switch check policy (2026-09-12)

Measured in the same combined sweep, and retained as exclusions:

- `modernize-avoid-variadic-functions` (19) and `cppcoreguidelines-pro-type-vararg` (1,586) report
  the printf-style text and debug printers and every call into them. Most format a translated string
  looked up at run time (`Text_String(id)`), so a parameter-pack replacement still needs a
  runtime-typed formatter behind it; the declarations that take a literal format already carry
  `ABSL_PRINTF_ATTRIBUTE`, which checks what can be checked.
- `concurrency-mt-unsafe` (323) is 222 `strtok` calls in INI and text parsing plus `exit`, `rand`,
  `inet_ntoa`, `gethostbyname`, `getenv` and `glob`. Every call runs on the main game thread; the
  SDL audio callback and the VQA timer path call none of them.
- `clang-diagnostic-switch-enum` (293) reports only switches that already have a default, over type
  enums of up to 102 values. `-Wswitch` and `-Wswitch-default` already require the fallback.
- `clang-diagnostic-covered-switch-default` (16) contradicts GCC's `-Wswitch-default`, which
  `CMakeLists.txt` already chose.

### Sign and parameter check policy (2026-09-12)

Measured in the combined sweep, and retained as exclusions:

- `clang-diagnostic-sign-conversion` reported 2,432 implicit signedness changes: `int` to
  `unsigned int` (593), `int` to `size_t` under its various spellings (657), `unsigned int` to `int`
  (283), `long` to `unsigned long` (161) and smaller mixes. They are overwhelmingly signed sizes and
  counts handed to library parameters (`memcpy`, `strncpy`, container indexing) and the legacy
  unsigned fields that [TYPE_MIGRATION.md](TYPE_MIGRATION.md) deliberately keeps. Under the
  project's signed-by-default rule each would become a `static_cast`, which records nothing a reader
  does not already know. The warning also stays off in `CMakeLists.txt`.
- `bugprone-signed-bitwise` reported 4,191 uses of a signed operand with a bitwise operator, 3,715
  with `IgnorePositiveIntegerLiterals`. About 1,800 are OR'd flag enumerators in the unit, building,
  infantry and terrain data tables (`td/bdata.cc` alone has 643); the rest are the VQA loader,
  blitters, big-integer code and random straw, all manipulating bit patterns on purpose. Making the
  flag enums unsigned is the useful version of this change and belongs with a flag-type redesign.
- `bugprone-easily-swappable-parameters` reported 426 functions with adjacent same-typed parameters,
  almost all coordinates, sizes and IDs in the drawing, gadget and type APIs. Strong types for those
  would touch most call sites for little defect value.

### Switch fallback review (2026-09-12)

`clang-diagnostic-switch`, `clang-diagnostic-switch-default` and
`bugprone-switch-missing-default-case` are now enforced together. The isolated sweep reported 792,
311 and 136 findings, and they overlap almost completely:

| Finding                                                     | Sites | Switches |
| ----------------------------------------------------------- | ----- | -------- |
| Gadget-ID `ButtonKey()` and similar cases on a `KeyNumType` | 639   | 98       |
| Enum switch without a default missing enumerators           | 153   | 153      |
| Integer switch without a default                            | 136   | 136      |
| Fully covered enum switch without a default                 | 22    | 22       |

The 98 key-input switches mix keyboard enumerators with gadget IDs, which are integers built by
`ButtonKey()` and are not values of `KeyNumType`. They now switch on `static_cast<int>(input)`,
which states what the cases already assumed.

The 311 switches without a default each gain `default: break;` before their closing brace. That is
the fallback GCC's `-Wswitch-default` already requires in `CMakeLists.txt`, and it makes an
unhandled value's path explicit rather than implied. No case body changed. One report comes from the
switch inside GoogleTest's `EXPECT_DEATH` expansion in `port/unaligned_test.cc` and is suppressed at
that line.

### Map layer redraw flag review (2026-09-12)

`clang-diagnostic-shadow-field` is now enforced. All eight reports were the same construct: both
games' map display chain, `GScreenClass` → `DisplayClass` → `RadarClass` → `PowerClass` →
`SidebarClass` → `TabClass`, declares a one-bit `IsToRedraw` at every level. Each layer's methods
set and test the nearest one, so the six bits are independent flags that only look like one.

Renaming a single layer's member would have been silently wrong: its methods would still compile,
now reading the base layer's bit. All six were renamed at once (`IsScreenToRedraw`,
`IsDisplayToRedraw`, `IsRadarToRedraw`, `IsPowerToRedraw`, `IsSidebarToRedraw`, `IsTabToRedraw`), so
no fallback remained and the compiler reported every use. Each use was then given the name of the
nearest chain class at or above its member function's class, or of the object's static type, exactly
the member the old lookup found. The unrelated `IsToRedraw` flags in `DoorClass`, `CreditClass` and
the sidebar's `StripClass` are unchanged.

### Local shadowing review (2026-09-12)

`clang-diagnostic-shadow` is now enforced. It reported 65 declarations: 57 locals that reuse the
name of an enclosing local or parameter, and eight parameters named like a member of their class.

The locals were read for the defect the check exists to catch, a use of the inner name that meant
the outer object, and none was found. The riskiest-looking are deliberate: `Take_Damage` passes a
fresh `damage` of 500 to a building's occupant while its own `damage` parameter carries on,
`Goodie_Check` walks the ground layer with an inner `object` and returns to its `FootClass* object`
parameter after the loop, and the nested `for (int i ...)` loops in the dialogs never read the
function-scope counter they hide. Each inner declaration now has a distinct name, so the next reader
does not have to establish that.

The eight member-named parameters (`ToolTipClass::Move`, `WolapiObject::LinkToChatDlg`,
`RAChatEventSink::OnGameStart`) are renamed in both the declaration and the definition.

### Legacy-code policy exclusions (2026-09-12)

Four P3 names are retained because the change they ask for is one the project's own rules steer away
from. CLAUDE.md's legacy-code section lists smart pointers everywhere, const everywhere and STL
containers everywhere under changes to avoid unless requested. Measured in the combined sweep:

| Check                                                               | Reports | What enabling would require                                     |
| ------------------------------------------------------------------- | ------- | --------------------------------------------------------------- |
| `cppcoreguidelines-owning-memory`                                   | 1,240   | `gsl::owner` or smart pointers on every raw `new`/`delete`      |
| `misc-const-correctness`                                            | 4,090   | `const` on every local that is never modified                   |
| `modernize-avoid-c-arrays` (and `cppcoreguidelines-avoid-c-arrays`) | 2,252   | `std::array` or containers for every C array, including layouts |

Many of the C arrays are fixed-layout data tables, network packets and save structures, where a
container type would change more than spelling. These stay open to targeted work — a specific
ownership bug, a specific array overrun — rather than tree-wide enforcement.

### Buffer, union and boolean conversion policy (2026-09-12)

Measured in the combined sweep, and retained as exclusions:

| Check                                                           | Reports |
| --------------------------------------------------------------- | ------- |
| `clang-diagnostic-unsafe-buffer-usage`                          | 5,924   |
| `cppcoreguidelines-pro-bounds-array-to-pointer-decay`           | 4,423   |
| `cppcoreguidelines-pro-bounds-constant-array-index`             | 3,365   |
| `cppcoreguidelines-pro-bounds-pointer-arithmetic`               | 2,491   |
| `cppcoreguidelines-pro-bounds-avoid-unchecked-container-access` | 1,840   |
| `cppcoreguidelines-pro-type-union-access`                       | 1,438   |
| `readability-implicit-bool-conversion`                          | 6,938   |

The five bounds checks describe one property from different angles: the engine walks raw buffers.
The blitters, the LCW/LZO/LZW codecs, audio mixing, the crypto code and the packet and save readers
all index and advance pointers into memory whose size lives elsewhere. Enforcing them one site at a
time would add casts and suppressions without adding a bound; the useful change is a span-based
buffer API that carries the size, after which these checks can be revisited per module. The
`port::AlignedObject` and typed buffer helpers already added for cast alignment are the start of
that API.

The union reports are the event, target and packet unions, whose layouts are part of the network and
savegame formats; replacing them with variants changes those formats.

`readability-implicit-bool-conversion` is overwhelmingly `if (ptr)` and flag tests. The Google C++
style guide, which this project follows, explicitly allows pointers and integers in boolean
contexts, so the fix-its would churn thousands of conditions against the project's own style.

### Const-dropping cast review (2026-09-12)

`clang-diagnostic-cast-qual` stays excluded, after a full attempt to enable it. The isolated sweep
reported 422 casts, almost all C-style, that remove `const` or `volatile`.

Four were writes through `const` that the cast was hiding, and are fixed at their design:

- `SHAEngine::Result()` is `const` but fills its digest cache on first use; `FinalResult` and
  `IsCached` are now `mutable`.
- Both games' `VectorClass` and `DynamicVectorClass` take a caller buffer as `const T* array` and
  placement-construct into it; the parameter is now `T*`. No caller passes one.
- `AbstractTypeClass::Set_Name` was `const` in both games while writing `IniName`; it no longer is.
- `WinModemClass::Write_To_Serial_Port` only reads its buffer, so it now takes
  `const unsigned char*`.

The rest could not be enabled honestly. clang's cast fix-its turn a pure `const` removal into
`const_cast`, and the casts that also change the type become a named cast to the `const` target;
compiling that showed about 60 more whose result is written. Every one of those writes ends in a
`const_cast` removing `const` — 256 in total — and `cppcoreguidelines-pro-type-const-cast`, which is
already enforced, rejects each of them. The two checks together require the writes themselves to go:
`const` type-class tables that are patched at load time, list nodes that hand out mutable parents
from `const` accessors, and blitters that decode into buffers typed as `const`. That is API work to
do module by module, after which this check can be revisited.

### Nodiscard review (2026-09-12)

`modernize-use-nodiscard` is now enforced. Its fix-its add `[[nodiscard]]` to 927 value-returning
functions. Before applying them to the tree, the whole change was built in a separate worktree and
every translation unit compiled with `-Wunused-result`, against a probe that confirms a discarded
`[[nodiscard]]` call is reported. That found 235 discarded results, and they came from five
functions whose results the engine ignores on purpose:

| Function                        | Discards | Why the result is ignored                           |
| ------------------------------- | -------- | --------------------------------------------------- |
| `Validate`                      | 215      | Debug self-check run for its assertions.            |
| `Create_One_Of`                 | 8        | The object heap owns what it creates.               |
| `AI_Build_*`, `AI_Raise_*`, ... | 8        | The planner does not act on whether a helper acted. |
| `Create_And_Place`              | 3        | Crate and editor drops tolerate a failed placement. |
| `CarryoverClass::Create`        | 1        | Recreated for its side effect.                      |

Those 47 declarations keep no attribute, each under a `NOLINTNEXTLINE` that says why. With them
excluded, no call site in either game discards a `[[nodiscard]]` result.

### Declaration and linkage review (2026-09-12)

`clang-diagnostic-missing-prototypes`, `clang-diagnostic-missing-variable-declarations` and
`misc-use-internal-linkage` are now enforced together. The isolated sweep reported 105 functions and
164 variables defined without a previous declaration, and 390 names that could have internal
linkage; the three lists describe the same underlying state, a global symbol that no header
declares.

The first thing they exposed was headers that had drifted from the code they describe:

- `ra/expand.h` declared `bool Expansion_Dialog()`, which RA never defines; the real function takes
  `bool bCounterstrike`, and `ra/init.cc` had worked around the stale header with its own prototype.
- `ra/nulldlg.h` declared `Smart_Printf(char*, ...)` and `ra/profile.h` declared
  `Write_Bin_Init(const char*, int)`; neither overload exists. The headers now match the
  definitions.
- Both games' `ccfile.cc` define a second `Load_Alloc_Data(const char* name, int)` overload that no
  header declared; it now sits beside `Load_Alloc_Data(FileClass&)` in `jshell.h`.
- `winvq/vqm32/compress.h` declared the C function `LCW_Uncompress` as
  `(char const*, char*, unsigned long)` while `tech` defines `(void*, void*, unsigned long)`. Both
  declarations are `extern "C"`, so every object referenced the same unmangled symbol and it linked,
  but the VQA drawer and loader were type-checked against the wrong parameters.
- `output(short, short)` was an empty DOS port-output stub in both games. Its only caller, RA's
  `MonoClass::Set_Cursor`, wrote the mono card's CRTC cursor register; the port's mono pages live in
  memory, so the writes and the stub are both gone and `Set_Cursor` just records the position.
- `Flag_To_Set_Palette`, the palette callback each game provides to the VQA player, was declared
  only inside `winvq/vqa32/drawer.cc`; it is now declared in the public `vqaplay.h`.
- The VQA test's link stubs include the real `compress.h` and `palette.h`, so their signatures are
  checked; `MainWindow`'s real declaration is Windows-only, so the test declares it itself.

The fix-its and a moving script took the rest in two steps. 53 symbols that other files reached
through local `extern` declarations or prototypes had those declarations moved into the header of
the defining file, or a module header for files without one: `ra/keyframe.h` for `2keyfram.cc`,
`ra/wol_main.h` for the Westwood Online dialogs, and each game's `externs.h` for the startup and
statistics globals. The local copies are gone. Declarations inside `extern "C"` blocks, Windows-only
sources and files the build does not compile were left alone. The internal-linkage fix-its then made
240 file-local functions and variables `static`.

That fix-it pass is not safe to take whole. The check sees one translation unit, so a definition
whose file does not include the declaring header looks unused elsewhere. The fix-its made 38 shared
symbols `static` and the link failed. Those went back, and each of the last 36 reports got a hand
fix:

- The shape-buffer globals, which the blitter in `tech/2keyfbuf.cc` declared locally, are declared
  in `tech/2keyfbuf.h`, and both keyframe loaders include it.
- The RA file wrappers in `ccfile.cc`, `Choose_Side`, both games' `Read_PCX_File` and RA's
  `Warheads` and `Weapons` gained the include of the header that already declared them.
  `tech/pcx_file.h` now takes `const char*`, matching TD's definition, and TD's `nondosstub.cc` uses
  the header's `PCX_HEADER` in place of its own copy. RA's `MaxDevice`, `DefaultDrive` and
  `CallingDOSInt`, which nothing read, are deleted.
- RA's Planet Westwood globals are declared in `ra/internet.h`, which `externs.h` includes. The
  handle, password, address and `ShowCommand` externs were deleted from `externs.h`: their
  definitions in `internet.cc` are `static`, so no use of them could ever have linked.
- `Extract_Compressed_Events` and `Extract_Uncompressed_Events` are declared in each game's
  `queue.h`, which the TD alignment test now uses. `Create_Main_Window` is declared in each game's
  `externs.h`. TD's `Keyboard_Process` and `CC_Texture_Fill` are declared in `td/conquer.h`.
  `WOL_PrintMessage` is declared in `ra/wolapiob.h`, `WOL_Download_Dialog` in `ra/rawolapi.h` and
  `bSpecialAftermathScenario` in `ra/wol_main.h`, replacing seven local copies.
- `sdllib` declared `Get_Font_Palette_Ptr`, `Load_Sample` and `Free_Sample` only under `#ifdef TD`,
  a macro the library itself is never built with, so its own definitions went unchecked. The guards
  are gone. `LCW_Comp`'s stub and `RandNumb` are declared in `sdllib/iff.h` and `sdllib/misc.h`, and
  `tech/lcwuncmp.cc` includes the `iff.h` declaration of `LCW_Uncompress`.
- TD's `Bibx3` smudge is `static`, as RA's already was.

With internal linkage, the compiler could see that 52 of the newly `static` names had no user. It
reported them as unused functions, unused variables or globals that are set but never read. They are
deleted:

- Empty DOS stubs: `Mono_Put_Char`, `Mono_Scroll`, `Mono_View_Page` and `Unfragment_File_Cache` in
  both games.
- Uncalled code: TD's copy of `Create_Palette_Interpolation_Table` (only RA builds the table at run
  time), `Timer_Test`, `Just_Path`, `IPX_Broadcast_Packet`, `Is_Disk_Inserted` and three VQA drawer
  accessors.
- Orphaned globals: `BlubCell`, `PlayerAborts`, TD's `Argv` and `Argc`, and the Planet Westwood
  handle, password and address strings.
- Write-only globals: RA's keyframe `Length`, `TotalTheaterShapes`, `VideoBackBufferAllowed` and
  `GameTimerInUse`, with their assignments.
- RA's `ending.cc` and `ending.h`, whose `GDI_Ending` and `Nod_Ending` were empty and uncalled.

RA's `dtable.cc` and `itable.cc` hold only the ADPCM tables for `adpcm.cc`, which the build already
excludes, so they join it in the excluded list.

Seven names are used only in code that the build compiles out: RA's `SHOW_MONO` screens and
`CS_DEBUG` names, TD's `FIX_ME_LATER` statistics, and TD's startup helpers in the round-trip test's
`TD_NO_ENTRY_POINT` build. They are `[[maybe_unused]]`.

`misc-use-internal-linkage.AnalyzeTypes` is off. A `.cc`-local struct, class or enum can only get
internal linkage from an anonymous namespace, and there is no `static` for types. Functions and
variables stay checked.

`misc-use-anonymous-namespace` stays excluded. It reported 766 file-local `static` functions and
variables in project sources. Google style accepts either `static` or an unnamed namespace, the tree
uses no namespaces, and moving 766 working definitions would change no linkage.

The full strict build of both games is clean and all 242 tests pass.

### Completed validation

The 2026-09-11 cross-unit global initialization review found 10 reports in the isolated sweep of 890
project translation units, including 431 generated header checks. All 10 are the same construct in
one file: TD's `hdata.cc` builds its ten `HouseTypeClass` objects passing a `Remap*` table declared
in `const.h` and defined in `const.cc`.

The construct is safe on two independent counts, neither of which the check models. The `Remap*`
arrays are `const unsigned char[256]` with all-integer-constant initializers, so they are
constant-initialized and sit in read-only data before any dynamic initialization runs; and
`HouseTypeClass`'s constructor stores the pointer in `RemapTable` without reading through it, so
nothing would depend on the contents even if the ordering were in doubt. RA reports nothing here
because its `HouseTypeClass` takes only a `PCOLOR_*` identifier and looks the table up later.

The ten reports are contiguous, so a single `NOLINTBEGIN`/`NOLINTEND` pair around the house table
carries the explanation once rather than repeating it ten times. Making the constructor `constexpr`
would remove the dynamic initialization altogether and retire the reports honestly, but it needs the
`strncpy` of the file suffix replaced and touches the table identity that `ioobj.cc` encodes in
saved games; that is a separate change.

Enabling the check is worth it because it catches the real thing: a probe whose global is
initialized from another translation unit's dynamically initialized global is reported. The final
isolated and full-config sweeps passed all 890 project translation units. Both strict game builds
and all 237 CTest tests passed. The RA save/load smoke check matched 240 object positions and the TD
default fixture matched 5,742 game states, though the change is comment-only. The probe is silent
with the previous exclusion restored.

The 2026-09-11 parent virtual call review found 13 sites in the isolated sweep of 890 project
translation units, including 431 generated header checks. Twelve are deliberate, and the row's own
note anticipated that; they now carry `NOLINTNEXTLINE` with the reason rather than staying invisible
behind a blanket exclusion.

Three kinds of deliberate skip appear. The gadget classes replace an intermediate override rather
than extend it: `TriColorGaugeClass::Draw_Me` draws its own three-colour body instead of
`GaugeClass`'s single-colour one and wants only `ControlClass`'s repaint-flag and peer handling,
`SliderClass::Draw_Me` draws a thumb when it belongs to a list and still delegates to `GaugeClass`
when it does not, and `WOLEditClass::Action` reimplements `EditClass::Action` with its own key
handling and finishes exactly as `EditClass` does. `ListClass::Draw_Me` and `TListClass::Draw_Me`
skip `ControlClass::Draw_Me` because it redraws the peer gadget, and a list's peer is the drop list
that owns it; events still reach the peer through `ControlClass::Action`. Finally, the game logic
reaches past an override for a base primitive it specifically wants: RA's
`UnitClass::Assign_Destination` calls `FootClass::Assign_Destination` once docking with a service
depot is already arranged, because `DriveClass` would run its refinery logic over the top and its
path reset is done by hand on the next line; TD's sniper case calls `TechnoClass::Greatest_Threat`
because `FootClass` would add `THREAT_GROUND` and put vehicles back in range; and RA's carrier calls
`TechnoClass::Receive_Message` for `RADIO_DOCKING`, which `FootClass::Receive_Message` has no case
for and would only forward.

One site was redundant. TD's `UnitClass::Debug_Dump` called `CargoClass::Debug_Dump`,
`MissionClass::Debug_Dump` and `TarComClass::Debug_Dump` in turn, but every link from `TarComClass`
up through `TurretClass`, `DriveClass`, `FootClass`, `TechnoClass` and `RadioClass` calls its base
unconditionally under the same `kCheatKeysEnabled` guard, so the mission section was already dumped
by the chain. The duplicate call is removed; RA's equivalent already used the single-chain form. The
cargo call stays, because `TechnoClass::Debug_Dump` dumps its flasher, stage and radio bases but not
its cargo base.

The final isolated and full-config sweeps passed all 890 project translation units. Both strict game
builds and all 237 CTest tests passed. The RA save/load smoke check matched 240 object positions;
the TD default, team and mobile fixtures matched 5,742, 5,951 and 6,212 game states. A leaf class
calling its grandparent's virtual confirms the enabled check reports an error under the repository
configuration and is silent with the previous exclusion restored.

The 2026-09-11 moved-storage review found one finding in the isolated sweep of 890 project
translation units, including 431 generated header checks: RA's `UnitTypeClass::One_Time` took a
pointer into the shape vector it was about to move into the type object, and used it after the
enclosing scope ended. The diagnostic hedges in its own text, and it was right to: `SetOwnedImage`
moves a `std::vector<std::byte>` into the type's variant, so the heap buffer and therefore the
pointer survive. The load now stores the data first and reads the pointer back through
`GetImageSpan`, which removes the loan into a moved-from local and drops the local's two `#ifdef`
spellings down to one expression each. `ptr` moves to its point of use and `<utility>` is no longer
needed.

An instrumented run confirmed the rewrite is byte for byte equivalent: with the exclusion
temporarily restored to build the original, both versions report the same non-null pointer and the
same shape size for all 22 unit types, so `MaxSize` is unchanged.

Note that plain `clang-diagnostic-lifetime-safety-use-after-scope` was never excluded and is already
enforced; this row covers only the narrower moved-storage case, which requires the pointer's owner
to be moved from and the pointee scope to end.

The final isolated and full-config sweeps passed all 890 project translation units. Both strict game
builds and all 237 CTest tests passed. The RA save/load smoke check matched 240 object positions;
the TD default and team fixtures matched 5,742 and 5,951 game states. A pointer taken into a vector
that is then moved out of an inner scope confirms the enabled diagnostic reports an error under the
repository configuration and is silent with the previous exclusion restored.

The 2026-09-11 use-after-free review found three findings in the isolated sweep of 890 project
translation units, including 431 generated header checks. All three are real defects on the same
path, and none is the fixed-heap false positive this row anticipated: the checker objected to
genuine reads of a projectile after `delete` returned its block to the heap. `BulletClass::Unlimbo`
fails whenever `ObjectClass::Unlimbo` cannot mark the bullet down at the firing coordinate, so these
paths are reachable, not theoretical.

Both games' `TechnoClass::Fire_At` deleted the projectile when unlimbo failed and then returned the
dangling pointer. RA's `UnitClass::Fire_At` reads that result to delete a demolition truck and to
set the V2 reload timer; TD's `AircraftClass::Fire_At` goes further and writes through it, calling
`Fly_Speed` on whatever the heap has since handed out. TD's `TechnoClass::Fire_At` also read
`bullet->Class->IsFueled` unconditionally after the delete, and TD's nuclear silo tested the freed
pointer to decide whether the missile had launched, advancing to `DONE_LAUNCH` with nothing in the
air. Each site now clears the pointer at the delete; TD's recoil decision reads the projectile type
into a local beforehand, since it depends only on the type. The success paths are byte for byte
unchanged. Eight lines of commented-out Mono debugger output inside the TD block were removed with
the restructuring.

No isolated regression test covers the three call sites: forcing `Unlimbo` to fail needs a full game
world (map, houses, and the object heaps), so a unit test would assert against a fixture rather than
the defect. A heap test covers the mechanism that makes these reads dangerous instead, showing that
a freed slot goes straight to the next allocation, so the stale pointer addresses a live object
rather than a dead block.

The final isolated and full-config sweeps passed all 890 project translation units. Both strict game
builds and all 237 CTest tests passed. The RA save/load smoke check matched 240 object positions;
the TD checks matched 5,742 to 6,371 game states across all eight fixtures, with the corrupt-save
rejections intact. A deliberate read after `delete` confirmed the enabled diagnostic reports an
error under the repository configuration and is silent with the previous exclusion restored.

The 2026-09-11 derived-method shadowing cleanup found 38 distinct hiding sites in the isolated sweep
of 908 project translation units, including 431 generated header checks. `Map` is always a
`MapEditClass`, so every reported call already dispatched statically to the most derived version;
the changes make that relationship explicit rather than altering it.

Two chains became real virtual chains. Both games' `ResetTransientUiState`, which each screen layer
redeclares and forwards up through `GScreenClass`, is now virtual with `override` on all eight
derived layers. TD's `DisplayClass::Write_INI` is likewise virtual so the map editor's version
overrides it. TD's `TechnoClass` gained a virtual `Made_A_Kill` delegating to its crew base, letting
`InfantryClass` override it; the kill dispatch in `TechnoClass::Record_The_Kill` no longer needs its
`dynamic_cast`, and `CrewClass::Made_A_Kill` is renamed `Add_Kill` in both games so the tally helper
and the virtual no longer share a name. The original comment claiming a virtual would complicate
save/load predates field-wise serialization; adding one to an already polymorphic class changes no
object layout.

Redundant redeclarations are gone: the unused `TFixedHeapClass` template in both games, the empty
`OptionsClass::Process` stub whose only caller reaches `GameOptionsClass::Process`, RA's
`KeyboardClass::Clear` and `MapEditClass::Write_INI` forwarders, RA's `TriggerTypeClass::As_Target`
(its `RTTI` is always `RTTI_TRIGGERTYPE`), TD's `UnitClass::operator UnitType`, and
`RAMFileClass::operator const char*`. RA's `MapEditClass` now unhides the other `Detach` overload
with a using-declaration instead of a forwarder. `GraphicBufferClass::Lock`/`Unlock` are renamed
`Lock_Surface`/`Unlock_Surface`; the 109 existing call sites keep calling the viewport versions,
which delegate to them, so the reattach step still runs exactly where it did.

Three hiding sites remain deliberate and carry `NOLINTNEXTLINE` with the reason: `Node`/`List` in
`tech/listnode.h` and RA's `KeyboardClass::Get`/`Check` exist only to narrow the base return types,
and giving them virtual dispatch would add vtables to types the raw-image loader still copies.

The final isolated and full-config sweeps passed all 889 unique project translation units. Both
strict game builds and all 228 CTest tests passed. The RA save/load smoke check matched 240 object
positions; the TD smoke check matched 5,742 to 6,371 game states across its default, team, world,
building, mobile, map, globals, and factory fixtures, with the corrupt-save rejections intact. A
deliberately hidden base method confirmed the enabled check reports an error under the repository
configuration and is silent with the previous exclusion restored.

Separately noted for later review: `WWKeyboardClass::Check` returns `bool`, so RA's
`KeyboardClass::Check` cannot actually deliver the key number its `KeyNumType` return type promises.
That predates this change and is left alone here.

The 2026-09-11 integer-to-pointer cast check passed both isolated and full-config sweeps across 908
project translation units, including 431 generated header checks, without findings or source fixes.
Both strict game builds and all 228 CTest tests passed. A sample casting an `int` to `char*`
confirmed the enabled diagnostic reports an error under the repository configuration and the strict
build's existing `-Weverything` flag, and is silent with the previous exclusion restored.

The 2026-09-11 implicit integer conversion cleanup found 518 narrowing sites in 115 files in the
isolated sweep of 908 project translation units, including 431 generated header checks, plus three
negation sites for the companion diagnostic. Most sites are intentional narrowing into byte and
16-bit storage: palette and remap indices, cell-lepton coordinate math into `LEPTON`, `Random_Pick`
results into animation parameters, and multiplayer packet fields. They now carry `static_cast` to
the destination's own type; class members, packet layouts, and saved data layouts are unchanged. A
few needlessly narrow locals are widened instead, and the null-modem connect ID now derives from
`std::bit_cast` of the buffer address rather than a C-style pointer cast.

Three defects were corrected along the way. Both games' `Load_Uncompress` stored the decompressed
byte count back into the 16-bit on-disk size field, truncating the return value for images over
65,535 bytes; the count is now returned from a separate variable. RA's `FootClass::Set_Speed` wrote
its byte through a reinterpreting reference into the `int` speed member and now assigns directly.
TD's `Get_Ownable` virtual returned a byte although the ownable mask covers ten houses; the
`ObjectClass` and `TechnoClass` overrides now return `int`, matching RA. Two fading-table builders
(`sdllib/misc.cc` and `td/support.cc`) read the red gun for the green target; that pre-existing
defect is noted here and left for separate review.

The final isolated and full-config sweeps passed all 908 translation units. Both strict game builds
and all 228 CTest tests passed. Headless save/load checks matched 240 RA object positions and 5,742
and 5,951 TD game states in the default and team fixtures. A sample narrowing an `int` to
`unsigned char` and a negated `unsigned short` confirms that both enabled diagnostics report errors
under the repository configuration and the strict build's existing `-Weverything` flag, and stay
silent with the previous exclusions restored.

The 2026-09-11 unsigned zero comparison review found two diagnostics in the isolated sweep of 908
project translation units, including 431 generated header checks: both games' `MapClass::Validate`
tested the unsigned-byte template icon index for negativity. The always-false test is removed; the
validators still reject icon indices at or beyond the template's width times height and indices that
map to an empty icon slot. Cell layouts, template data, and saved data layouts are unchanged.

Both affected files pass the isolated check, and the final full-config sweep passed all 908
translation units. Both strict game builds and all 228 CTest tests passed. A deliberately impossible
comparison of an unsigned value with zero confirms that the enabled diagnostic reports an error
under the repository configuration and the strict build's existing `-Weverything` flag, and is
silent with the previous exclusion restored.

The 2026-09-11 unsigned-enum zero comparison review found two diagnostics in the isolated sweep of
889 project translation units, including 431 generated header checks. TD event execution now
compares the event type as unsigned against `PROCESS_TIME`, preserving the upper-bound diagnostic
without testing an unsigned enum for negativity. TD cell loading no longer tests the byte-sized
`TemplateType` for negativity; it still rejects IDs at or above `TEMPLATE_COUNT` except for the
explicit `TEMPLATE_NONE` sentinel (255). Enum declarations, wire formats, and saved data layouts are
unchanged.

Both affected files pass the isolated check, and the final full-config sweep passed all 889
translation units. Both strict game builds and all 228 CTest tests passed. A deliberately invalid
unsigned-enum comparison with zero confirms that the enabled diagnostic reports an error under the
repository configuration and the strict build's existing `-Weverything` flag.

The 2026-09-11 constant-out-of-range comparison review found one diagnostic in the isolated sweep of
889 project translation units, including 431 generated header checks: RA's emergency shutdown
compared the boolean `ReadyToQuit` with 3. The existing shutdown protocol uses states 0 (running), 1
(clean shutdown), 2 (complete), and 3 (emergency). Its declaration and definition now use `int` so
those states remain distinct; initialization is still the running state. The existing state
assignments, comparisons, and SDL event handling are unchanged.

The affected startup file passes the isolated check, and the final full-config sweep passed all 889
translation units. A deliberately impossible comparison of an unsigned byte with 256 confirms that
the enabled diagnostic reports an error under the repository configuration and the strict build's
existing `-Weverything` flag.

The 2026-09-11 nonzero enum-to-bool conversion check passed both isolated and full-config sweeps
across 889 project translation units, including 431 generated header checks, without findings or
source fixes. No enum ignore list was added. Both strict game builds and all 228 CTest tests passed.
A sample converting a scoped enum with only nonzero enumerators to `bool` confirmed the enabled
check reports an error under the repository configuration.

The 2026-09-11 constructor-order cleanup reordered 16 initializer lists across 15 files. The initial
889-unit sweep reported diagnostics in 18 translation units, including repeated uses of the RA list
template. Initializer expressions, base/member declarations, layouts, and actual C++ initialization
order are unchanged. The changes cover RA UI, rules, scenario, and type constructors and TD object
and file constructors. The templated list fix was applied manually because Clang's suggested
replacement ranges omitted closing parentheses.

The final isolated and full-config sweeps passed all 889 project translation units, including 431
generated header checks. Both strict game builds and all 228 CTest tests passed. A deliberately
reordered constructor confirmed the diagnostic reports an error under the enabled repository
configuration and the strict build's existing `-Weverything` flag.

The 2026-09-11 uninitialized const-pointer argument check passed both isolated and full-config
sweeps across 889 project translation units, including 431 generated header checks, without findings
or source fixes. Both strict game builds and all 228 CTest tests passed. A sample passing an
uninitialized integer to a `const int*` parameter confirmed the diagnostic reports an error under
the enabled repository configuration and the strict build's existing `-Weverything` flag.

The 2026-09-11 cast-alignment cleanup found 147 diagnostic sites in 29 files across the initial
884-unit sweep. Packet and serial headers, CRCs, event records, audio samples, video rows, and
animation offsets now use byte copies where alignment is not guaranteed. Allocated packet buffers
and opaque list entries use checked object recovery; cached shape headers advance at their native
alignment. The multi-precision reduction copies half-word windows into aligned working storage. Byte
fills replace manual word stores, including an LCW run loop that could overwrite its end. Native
packet layouts and byte order are unchanged. Animation opening rejects misaligned caller storage
instead of constructing a misaligned header.

The final isolated sweep passed all 889 project translation units, including 431 generated header
checks. The full-config sweep also passed, followed by checks of the final receive-header fixes.
Both strict game builds and all 228 CTest tests passed. New regressions cover unaligned scalar
access, checked object recovery, odd video strides, bounded LCW runs, real TD packet/event parsing,
and modular multiplication against an independent remainder calculation. The original LCW code fails
a sentinel-boundary probe; the fixed code passes. UBSan alignment probes fail against the original
VQA, TD receive-header, and compressed-event code and pass with the fixes. Headless save/load checks
matched 240 RA object positions and 5,951 TD game states. A deliberately unsafe cast confirmed
enforcement under the repository configuration with `-Wcast-align`; strict compile commands already
enable the warning through `-Weverything`.

The 2026-09-11 standard C library argument check found one invalid socket option in the isolated
884-unit sweep: TD passed a one-byte `bool` to `setsockopt` with a four-byte length. The TCP_NODELAY
flag now uses an `int` and `sizeof` its actual storage. The affected file passed the isolated check,
and the full-config sweep passed all 884 project translation units, including 429 generated header
checks. Both strict game builds and all 218 CTest tests passed. Deliberately invalid `isalnum` and
`fseek` arguments confirmed the enabled check reports errors under the repository configuration.

The 2026-09-11 string-to-number conversion cleanup found 258 conversion sites across 49 files in the
initial 879-unit sweep. Shared checked decimal/hex parsers now reject malformed or overflowing
tokens, with explicit defaults at callers. INI readers retain the caller's default; legacy fields
without a supplied default generally use zero. The changes preserve signed/unsigned coordinate
spellings, 64-bit trigger data, hexadecimal INI forms, and fixed-width protocol fields. Team counts,
infantry sub-cell indices, and network address bytes also have bounds checks.

The final isolated and full-config sweeps passed across 884 project translation units, including 429
generated header checks; follow-up checks covered the 64-bit trigger/editor conversions. Both strict
game builds and all 218 CTest tests passed. New INI regression tests fail against the original RA
reader and pass with the fixes. The RA save/load smoke check matched 240 object positions; the TD
team-fixture smoke check matched 5,951 game states. An unchecked `atoi` sample confirmed both check
names report an error under the enabled repository configuration.

The 2026-09-11 copy-constructor base-initialization check cleanup covered 879 project translation
units, including 428 generated header checks and excluding dependencies. Both the isolated and
full-config sweeps passed without findings, so no source fixes were needed. Both strict game builds
and all 205 CTest tests passed. A sample copy constructor that omitted copying its base confirmed
the enabled check reports an error under the repository configuration.

The 2026-09-11 inner-pointer check cleanup covered 879 project translation units, including 428
generated header checks. Both the isolated and full-config sweeps passed without findings, so no
source fixes were needed. Both strict game builds and all 205 CTest tests passed. A sample using a
string buffer pointer after `clear()` confirmed the enabled check reports an error.

The 2026-09-11 C-string uninitialized-read cleanup covered 879 project translation units, including
428 generated header checks. The isolated scan found one path from the key-generation self-test
buffer into `PKey::Encrypt`; initializing that buffer preserves the existing random-fill loop. The
affected file passed the isolated check, the full-config sweep passed, both strict game builds
passed, and all 205 CTest tests passed. An uninitialized `memcpy` sample confirmed enforcement.

The 2026-09-11 string-view check cleanup covered 878 project translation units, including 428
generated header checks. The isolated scan found one call; its fix passed the isolated check and the
full-config sweep. Strict builds of both games and all 197 CTest tests passed. A deliberately unsafe
sample confirmed the enabled check reports an error.

### Workflow

1. Measure the check across both games and shared code, including generated header checks and
   excluding dependencies. Verify the check exists; handle aliases and options together.
2. Fix findings, run the candidate and full-config sweeps, then strict-build both games and run
   CTest. Add focused behavior tests and save/load checks when the changes warrant them.
3. Remove the exclusion, verify enforcement, and mark the row **Enabled** with its commit. If
   enabling is unsuitable, record the concrete reason rather than silently dropping the row.

Compiler diagnostic filters also depend on warning flags. In particular, `sign-conversion`,
`unsafe-buffer-usage`, `old-style-cast`, `padded`, and `covered-switch-default` are suppressed in
[CMakeLists.txt](../CMakeLists.txt); removing their tidy exclusions alone does not enable them. An
isolated diagnostic sweep needs its warning flag and at least one real clang-tidy check.

Preserve the [type-migration policy](TYPE_MIGRATION.md), deterministic simulation RNG, and
packet/recording layouts when applying broad rules. Field-wise savegames do not make all layout
changes safe.
