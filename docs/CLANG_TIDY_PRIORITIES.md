# Clang-tidy priorities

Updated: 2026-09-15, against [`.clang-tidy`](../.clang-tidy) and clang-tidy 23.1.2.

This tracks **all 104 currently excluded check names** and completed entries, in recommended work
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
| `abseil-unchecked-statusor-access`                            | Enabled | Commit `Enable the remaining P1 checks`: the PCX byte reader, the only `StatusOr` user and the unit LLVM 23.1.2 crashed on, returns `std::optional`; see review below.                                                                            |
| `clang-analyzer-unix.cstring.UninitializedRead`               | Enabled | Commit `Enable uninitialized C-string read checking`: initialize the public-key generation self-test buffer while preserving the random-fill loop.                                                                                                |
| `clang-analyzer-cplusplus.InnerPointer`                       | Enabled | Commit `Enable string inner-pointer checking`: detect string-buffer pointers used after invalidation.                                                                                                                                             |
| `bugprone-copy-constructor-init`                              | Enabled | Commit `Enable copy-constructor base initialization checking`: both games and shared code already initialize copied base state correctly; no source fixes needed.                                                                                 |
| `bugprone-unchecked-string-to-number-conversion`              | Enabled | Commit `Enable checked string-to-number conversions`: replace unchecked decimal/hex conversions with range-checked parsing and explicit defaults; preserve legacy INI, coordinate, and protocol formats.                                          |
| `cert-err34-c`                                                | Enabled | Alias enabled with `bugprone-unchecked-string-to-number-conversion` in commit `Enable checked string-to-number conversions`.                                                                                                                      |
| `clang-analyzer-unix.StdCLibraryFunctions`                    | Enabled | Commit `Fix TCP socket option size and enable C library checking`: pass an integer TCP_NODELAY flag with its actual size, avoiding a read past a one-byte bool.                                                                                   |
| `clang-diagnostic-cast-align`                                 | Enabled | Commit `Fix buffer alignment and enable cast alignment checking`: copy unaligned packet/media values, check typed buffer access, align cached shape headers, and bound legacy byte fills.                                                         |
| `clang-diagnostic-uninitialized-const-pointer`                | Enabled | Commit `Enable uninitialized const-pointer argument checking`: both games and shared code pass without source fixes.                                                                                                                              |
| `clang-diagnostic-reorder-ctor`                               | Enabled | Commit `Match constructor initialization order to declarations`: reorder 16 initializer lists while preserving expressions, member layouts, and actual initialization order.                                                                      |
| `bugprone-unhandled-code-paths`                               | Enabled | Commit `Enable the remaining P1 checks`: zero reports once the switch-fallback work had given every switch a `default`; see review below.                                                                                                         |
| `bugprone-non-zero-enum-to-bool-conversion`                   | Enabled | Commit `Enable nonzero enum-to-bool conversion checking`: both games and shared code pass without source fixes.                                                                                                                                   |
| `clang-analyzer-optin.core.EnumCastOutOfRange`                | Enabled | Commit `Enable the remaining P1 checks`: the flag enums carry `CNC_FLAG_ENUM`, computed directions go through `AsDirection`, `KeyNumType` has its own operators; plan in [P1_REMAINING_PLAN.md](P1_REMAINING_PLAN.md), review below.              |
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
| `clang-diagnostic-sign-conversion`                         | Enabled | Commit `Enable sign conversion checking`: 2,414 reports, fixed mostly at the type: signed heap and vector indices, money, text widths and fixed-point helpers, connection timing with an explicit `-1` "no limit", and the VQA player's `long` fields. Library parameters take `base::ToSize`/`base::ToSigned`; packets, recordings and saves keep their widths. See review below.                                                                                                                      |
| `bugprone-signed-bitwise`                                  | Skipped | Commit `Document sign and parameter check policy`: 3,715 reports even with `IgnorePositiveIntegerLiterals`; about 1,800 are enum flag ORs in the unit and building data tables, the rest deliberate bit manipulation in the VQA loader, blitters and crypto. See review below.                                                                                                                                                                                                                          |
| `hicpp-signed-bitwise`                                     | Legacy  | Unavailable in LLVM 23; review with `bugprone-signed-bitwise` on older tools.                                                                                                                                                                                                                                                                                                                                                                                                                           |
| `clang-diagnostic-switch-enum`                             | Skipped | Commit `Document variadic and thread-safety check policy`: all 293 reports are switches that already have a deliberate default over large type enums (up to 102 values); `-Wswitch` and `-Wswitch-default` already require an explicit fallback.                                                                                                                                                                                                                                                        |
| `clang-diagnostic-switch`                                  | Enabled | Commit `Give every switch a fallback and switch on key numbers as integers`: 792 reports; 639 were gadget-ID `ButtonKey()` cases in 98 `KeyNumType` switches, which now switch on the integer key number; the 153 unhandled-enumerator switches get the default below.                                                                                                                                                                                                                                  |
| `clang-diagnostic-switch-bool`                             | Enabled | Commit `Enable ten checks the tree already satisfies`: no findings across 460 translation units; a probe confirms it reports.                                                                                                                                                                                                                                                                                                                                                                           |
| `clang-diagnostic-duplicate-enum`                          | Enabled | Commit `Drop the implicit FIRST enum aliases and fix mixed enum operations`: 25 reports, all an `X_FIRST = 0` alias duplicating the first real enumerator (22 TD enums, three RA trigger/team enums); uses now name that enumerator, per the magic_enum no-alias rule.                                                                                                                                                                                                                                  |
| `clang-diagnostic-missing-braces`                          | Enabled | Commit `Brace the infantry animation control tables`: all 680 reports were rows of TD's `[DO_COUNT][3]` tables in `idata.cc`, now one brace pair per row; layout unchanged.                                                                                                                                                                                                                                                                                                                             |
| `clang-diagnostic-cast-qual`                               | Enabled | Commits `Const-qualify the read-only downcasts` through `Let ListClass own its item strings` (eleven commits): 387 sites, none fixed with a cast. Found TD dialogs corrupting the shared text table in place, `Base_Is_Attacked` mutating a const enemy, per-player house colors written into the shared type, and a Blowfish in-place path that wrote over its const source. See the enablement review below.                                                                                          |
| `misc-explicit-constructor`                                | Enabled | Commit `Make single-argument constructors explicit where conversion is unintended`: 125 reports; the check's fix-its made 69 constructors explicit (file, pipe, straw, heap, vector, dialog and game-object constructors) with no call site relying on the conversion. The 56 deliberate conversions stay implicit under a reasoned `NOLINTNEXTLINE`: object-to-type-ID operators, `CCPtr`, `TargetClass`, `FacingClass`, countdown timers, choice tables, palettes and big integers. See review below. |
| `cppcoreguidelines-explicit-constructor`                   | Enabled | Alias enabled with `misc-explicit-constructor` in commit `Make single-argument constructors explicit where conversion is unintended`.                                                                                                                                                                                                                                                                                                                                                                   |
| `google-explicit-constructor`                              | Enabled | Alias enabled with `misc-explicit-constructor` in commit `Make single-argument constructors explicit where conversion is unintended`.                                                                                                                                                                                                                                                                                                                                                                   |
| `hicpp-explicit-conversions`                               | Legacy  | Unavailable in LLVM 23; review with `misc-explicit-constructor` on older tools.                                                                                                                                                                                                                                                                                                                                                                                                                         |
| `cppcoreguidelines-pro-type-vararg`                        | Enabled | Commit `Enable the variadic function checks`: the printers are variadic templates over `absl::FormatSpec` and `port::FormatRuntime`, the C printf family is `absl::SNPrintF`/`PrintF`/`FPrintF` at 602 sites, and `Buffer_Frame_To_Page` takes a `ShapeEffects` struct; the POSIX `fcntl` call keeps a `NOLINT`. Plan in [VARIADIC_PLAN.md](VARIADIC_PLAN.md); see review below.                                                                                                                        |
| `hicpp-vararg`                                             | Legacy  | Unavailable in LLVM 23; review with `cppcoreguidelines-pro-type-vararg` on older tools.                                                                                                                                                                                                                                                                                                                                                                                                                 |
| `modernize-avoid-variadic-functions`                       | Enabled | Commit `Enable the variadic function checks`: the 19 printer definitions are templates; see the variadic printer enablement review below.                                                                                                                                                                                                                                                                                                                                                               |
| `cert-dcl50-cpp`                                           | Enabled | Alias of `modernize-avoid-variadic-functions`; enabled with it.                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| `clang-diagnostic-missing-format-attribute`                | Enabled | Commit `Enable ten checks the tree already satisfies`: no findings across 460 translation units; a probe confirms it reports.                                                                                                                                                                                                                                                                                                                                                                           |
| `clang-diagnostic-undef`                                   | Enabled | Commit `Test language and debug macros with defined()`: 105 reports; TD's `FRENCH`/`GERMAN`/`JAPANESE` builds and two WOL `SHOW_MONO` blocks tested undefined macros with `#if`, now `defined()` with the same result.                                                                                                                                                                                                                                                                                  |
| `clang-diagnostic-undefined-func-template`                 | Enabled | Commit `Declare the explicitly instantiated templates`: 121 reports; `extern template` declarations now sit beside `CCPtr`, TD's vectors, `ObjectPtr` and the out-of-line `Serialize` members whose definitions live in one `.cc` file.                                                                                                                                                                                                                                                                 |
| `clang-diagnostic-undefined-var-template`                  | Enabled | Commit `Declare the CCPtr heap specializations`: the 27 `CCPtr<T>::Heap` explicit specializations defined in `globals.cc` are now declared in `ccptr.h`, which also removes an ill-formed use-before-declaration.                                                                                                                                                                                                                                                                                       |
| `clang-diagnostic-shadow-field`                            | Enabled | Commit `Name each map layer's redraw flag after its layer`: the eight reports were one `IsToRedraw` bit-field redeclared at every step of both games' `GScreenClass` → `TabClass` chain; each layer's flag now has its own name, and every use was rebound by the compiler to the layer it already meant. See review below.                                                                                                                                                                             |
| `clang-diagnostic-shadow`                                  | Enabled | Commit `Give shadowing locals and parameters their own names`: 65 reports, none a use of the wrong variable; the 57 inner locals and eight member-named parameters are renamed within their scope. See review below.                                                                                                                                                                                                                                                                                    |
| `concurrency-mt-unsafe`                                    | Enabled | Commits `Add the helpers for the thread-unsafe function work` through `Enable concurrency-mt-unsafe`: 322 sites; `strtok` became `port::Tokenizer`, `rand` went away, `getenv`, `inet_ntoa`, `gethostbyname` and `glob` got modern replacements; `FunctionSet: posix` keeps `exit`. Plan in `docs/MT_UNSAFE_PLAN.md`; see review below.                                                                                                                                                                 |
| `clang-analyzer-optin.core.FixedAddressDereference`        | Enabled | Commit `Keep mono pages in memory and bound the box drawing`: the port addressed the DOS mono card at 0xB0000; the pages now live in memory, which exposed and fixed an off-by-one box clamp, an unclamped view size and `Fill_Attrib` testing `h` for `y` without the enable check.                                                                                                                                                                                                                    |
| `clang-analyzer-core.FixedAddressDereference`              | Legacy  | Unavailable in LLVM 23; review with `clang-analyzer-optin.core.FixedAddressDereference` on older tools.                                                                                                                                                                                                                                                                                                                                                                                                 |
| `bugprone-easily-swappable-parameters`                     | Skipped | Commit `Document sign and parameter check policy`: 426 reports, almost all adjacent same-typed coordinates, sizes and IDs in the legacy drawing, gadget and type APIs (`(int x, int y, int w, int h)`); renaming or wrapping them in strong types would touch most call sites for little defect value.                                                                                                                                                                                                  |
| `bugprone-random-generator-seed`                           | Enabled | Commit `Seed the C library generator from random_device`: five `srand(time(nullptr))` seeds use `std::random_device`; TD's dead `srand(0)` before `randomize()` is removed. `tech/rawfile.h` takes `UINT_MAX` from `std::numeric_limits`.                                                                                                                                                                                                                                                               |
| `cert-msc32-c`                                             | Enabled | Alias enabled with `bugprone-random-generator-seed` in the same commit.                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| `cert-msc51-cpp`                                           | Enabled | Alias enabled with `bugprone-random-generator-seed` in the same commit.                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| `modernize-use-integer-sign-comparison`                    | Enabled | Commit `Compare mixed-sign integers with std::cmp functions`: 238 reports, applied with the check's fix-its (`std::cmp_less` and friends, with `<utility>`); the values compared are unchanged, and the comparisons are now correct for negative operands.                                                                                                                                                                                                                                              |
| `modernize-use-nodiscard`                                  | Enabled | Commit `Mark value-returning functions nodiscard`: 927 reports; the check's fix-its applied 921 attributes; 49 declarations whose results the engine deliberately ignores (`Validate`, `Create_And_Place`, `Create_One_Of`, the `AI_*` helpers, the message boxes' `int` `Process`) keep no attribute under a reasoned suppression, and no call site discards a result. See review below.                                                                                                               |

## P3 — Broader safety and maintainability

| Check                                                           | Status  | Reason / result                                                                                                                                                                                                                                                                                                                                                                                                              |
| --------------------------------------------------------------- | ------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `clang-diagnostic-unsafe-buffer-usage`                          | Skipped | Commit `Document buffer, union and boolean conversion check policy`: 5,924 reports of raw pointer and array indexing across the blitters, codecs, packet and save code; enforcing it needs a span-based buffer API first. See review below.                                                                                                                                                                                  |
| `cppcoreguidelines-pro-bounds-avoid-unchecked-container-access` | Skipped | Commit `Document buffer, union and boolean conversion check policy`: 1,840 reports of `operator[]` on the engine's vectors and heaps. See review below.                                                                                                                                                                                                                                                                      |
| `cppcoreguidelines-pro-bounds-constant-array-index`             | Skipped | Commit `Document buffer, union and boolean conversion check policy`: 3,365 reports of runtime indices into fixed game tables. See review below.                                                                                                                                                                                                                                                                              |
| `cppcoreguidelines-owning-memory`                               | Skipped | Commit `Document P3 checks the legacy-code policy rules out`: 1,240 reports, raw `new`/`delete` ownership across the object heaps, dialogs and buffers; `gsl::owner` or smart pointers everywhere is exactly what CLAUDE.md's legacy-code rules list it under changes to avoid unless requested. See review below.                                                                                                           |
| `cppcoreguidelines-no-malloc`                                   | Enabled | Commit `Hold INI section and entry names in std::string`: the 3 reports were RA's `INIEntry` and `INISection` freeing names that `ini.cc` had `strdup`ed at 4 sites; the names are now `std::string`, and the 12 readers use `.c_str()`, `.data()` and `.size()`.                                                                                                                                                            |
| `hicpp-no-malloc`                                               | Legacy  | Unavailable in LLVM 23; review with `cppcoreguidelines-no-malloc` on older tools.                                                                                                                                                                                                                                                                                                                                            |
| `cppcoreguidelines-pro-type-union-access`                       | Skipped | Commit `Document buffer, union and boolean conversion check policy`: 1,438 reports on the event, target and packet unions, whose layouts are part of the network and save formats. See review below.                                                                                                                                                                                                                         |
| `cppcoreguidelines-pro-bounds-array-to-pointer-decay`           | Skipped | Commit `Document buffer, union and boolean conversion check policy`: 4,423 reports, mostly C strings and fixed arrays passed to C-style APIs. See review below.                                                                                                                                                                                                                                                              |
| `hicpp-no-array-decay`                                          | Legacy  | Unavailable in LLVM 23; review with `cppcoreguidelines-pro-bounds-array-to-pointer-decay` on older tools.                                                                                                                                                                                                                                                                                                                    |
| `cppcoreguidelines-pro-bounds-pointer-arithmetic`               | Skipped | Commit `Document buffer, union and boolean conversion check policy`: 2,491 reports in the pixel, audio, compression and crypto loops that walk raw buffers. See review below.                                                                                                                                                                                                                                                |
| `modernize-avoid-c-arrays`                                      | Skipped | Commit `Document P3 checks the legacy-code policy rules out`: 2,252 reports; many arrays are fixed-layout game data, packet and save structures, and STL containers everywhere is what CLAUDE.md's legacy-code rules list it under changes to avoid unless requested. See review below.                                                                                                                                      |
| `cppcoreguidelines-avoid-c-arrays`                              | Skipped | Alias of `modernize-avoid-c-arrays`; skipped with it.                                                                                                                                                                                                                                                                                                                                                                        |
| `hicpp-avoid-c-arrays`                                          | Legacy  | Unavailable in LLVM 23; review with `modernize-avoid-c-arrays` on older tools.                                                                                                                                                                                                                                                                                                                                               |
| `cppcoreguidelines-use-enum-class`                              | Skipped | Commit `Record the remaining P3 policy decisions`: 517 reports; unnamed integer-constant enums, ordered dialog redraw levels and the `defines.h` index enums. See review below.                                                                                                                                                                                                                                              |
| `modernize-avoid-c-style-cast`                                  | Enabled | Commits `Drop the int casts from the key-number case labels` through `Enable the numeric cast checks`: 416 sites, none replaced by a bare `reinterpret_cast`; the casting macros are constants. See the numeric cast enablement review below.                                                                                                                                                                                |
| `google-readability-casting`                                    | Enabled | Alias enabled with `modernize-avoid-c-style-cast` in commit `Enable the numeric cast checks`.                                                                                                                                                                                                                                                                                                                                |
| `cppcoreguidelines-pro-type-cstyle-cast`                        | Enabled | Commits `Add byte-view helpers for the C-style cast work` through `Enable cppcoreguidelines-pro-type-cstyle-cast`: 415 type-unsafe casts, none replaced by a bare `reinterpret_cast`. Found an RA team-editor overflow, a TD map validator that rejected every 64-bit heap pointer, TD mono output writing to address 0xB0000, and signed-char PCX palette reads. See the enablement review below.                           |
| `clang-diagnostic-old-style-cast`                               | Enabled | Commit `Enable the numeric cast checks`: `-Wno-old-style-cast` is gone from the clang strict flag set; the compiler found 58 casts inside macro expansions the tidy check skips, now constants. See the numeric cast enablement review below.                                                                                                                                                                                |
| `clang-diagnostic-deprecated-enum-enum-conversion`              | Enabled | Commit `Drop the implicit FIRST enum aliases and fix mixed enum operations`: 76 reports; the `WWKEY_*` modifier bits are flags, so they became integer constants, and five facing-to-animation offsets cast the facing to `int`.                                                                                                                                                                                             |
| `clang-diagnostic-deprecated-anon-enum-enum-conversion`         | Enabled | Commit `Drop the implicit FIRST enum aliases and fix mixed enum operations`: three TD editor house-button offsets now subtract from an integer key number.                                                                                                                                                                                                                                                                   |
| `clang-diagnostic-deprecated-enum-compare`                      | Enabled | Commit `Drop the implicit FIRST enum aliases and fix mixed enum operations`: six comparisons against the wrong enum's zero or 1002 constant (`RESULT_NONE` for `IMPACT_NONE`, `ACTION_NONE` for `TACTION_NONE`, `NET_FILE_CHUNK` for `SERIAL_FILE_CHUNK`); values were equal, so behavior is unchanged.                                                                                                                      |
| `google-runtime-int`                                            | Enabled | Commit `Enable google-runtime-int`: 2,179 reports in five commits; `short` became `int16_t`, and each `long` got a 32-bit width for wire and file fields or 64 bits where values already needed them. See review below.                                                                                                                                                                                                      |
| `modernize-use-default-member-init`                             | Enabled | Commit `Move member defaults into initializers and declarations`: 724 member declarations gained brace defaults; the fix-its' comma debris and header-invisible names (`CELL_LEPTON_W`, `TXT_NONE`, `INVALID_SOCKET`) were repaired. See review below.                                                                                                                                                                       |
| `cppcoreguidelines-use-default-member-init`                     | Enabled | Commit `Move member defaults into initializers and declarations`: enforced with `modernize-use-default-member-init`, which it aliases.                                                                                                                                                                                                                                                                                       |
| `cppcoreguidelines-prefer-member-initializer`                   | Enabled | Commit `Move member defaults into initializers and declarations`: constructor-body assignments moved into initializer lists. Doing so exposed `delete` on a `new[]` array in both games' `UnitTrackerClass`, and TD `CommBufferClass` copying unchecked packet lengths into fixed buffers; both fixed. See review below.                                                                                                     |
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
| `readability-implicit-bool-conversion`                          | Enabled | Commit `Enable readability-implicit-bool-conversion`: with `AllowPointerConditions` and `AllowIntegerConditions` (Google style), 1,572 reports; flags and predicates became `bool` in four commits. See review below.                                                                                                                                                                                                        |
| `readability-inconsistent-declaration-parameter-name`           | Enabled | Commit `Name declaration parameters after their definitions`: 164 reports, applied with the check's fix-its. TD's `WWGetPrivateProfileString` definition took RA's parameter names instead, because the header-side rename made 66 correct calls read as swapped arguments to `readability-suspicious-call-argument`.                                                                                                        |
| `misc-const-correctness`                                        | Enabled | Commit `Enable misc-const-correctness`: 4,072 reports; three commits add 4,126 `const` qualifiers with the check's fix-its. Pointee warnings are off (`WarnPointersAsPointers: false`) because LLVM 23 misses writes through `*p++`, arrays of pointers and function-pointer hooks, and about 40 of those fix-its did not compile. See review below.                                                                         |
| `readability-make-member-function-const`                        | Enabled | Commit `Enable readability-make-member-function-const`: 165 reports judged by logical constness; 122 functions became `const`, 45 that exist to change state keep a reasoned suppression, and three dead no-ops were deleted. See review below.                                                                                                                                                                              |
| `misc-override-with-different-visibility`                       | Enabled | Commit `Match override access to the base declarations`: 19 overrides moved to their base's access level in both games' gadget, turret, drive, building and list classes.                                                                                                                                                                                                                                                    |
| `misc-header-include-cycle`                                     | Enabled | Commit `Enable ten checks the tree already satisfies`: no findings across 460 translation units; a probe confirms it reports.                                                                                                                                                                                                                                                                                                |
| `misc-include-cleaner`                                          | Enabled | Commit `Enable misc-include-cleaner`: 3,285 reports; the fix-its add 767 includes and remove 312 in 303 files. `IgnoreHeaders` exempts glibc's private spellings of POSIX network headers, and five includes whose use the check cannot see stay under `IWYU pragma: keep`. See review below.                                                                                                                                |
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
| `cppcoreguidelines-macro-usage`                                 | Skipped | Commit `Record the remaining P3 policy decisions`: 2,134 reports, 1,310 of them the text-string IDs in the two `conquer.h` headers. See review below.                                                                                                                                                                                                                                                                        |
| `modernize-macro-to-enum`                                       | Skipped | Commit `Record the remaining P3 policy decisions`: 2,256 reports over the same macro groups; enumerators would change type in integer arithmetic and formatting. See review below.                                                                                                                                                                                                                                           |
| `cppcoreguidelines-macro-to-enum`                               | Skipped | Commit `Record the remaining P3 policy decisions`: alias of `modernize-macro-to-enum`, skipped with it.                                                                                                                                                                                                                                                                                                                      |
| `bugprone-reserved-identifier`                                  | Enabled | Commit `Rename reserved identifiers`: 133 underscore-prefixed names renamed token-wise (`_Kbd` is `ActiveKeyboard`, `_GreyScheme` is `DefaultColorScheme`); generated `ra/wolapi/` headers and four Windows SDK spellings in `port/win32/win32_com.h` suppressed. See review below.                                                                                                                                          |
| `cert-dcl37-c`                                                  | Enabled | Commit `Rename reserved identifiers`: enforced with `bugprone-reserved-identifier`, which it aliases.                                                                                                                                                                                                                                                                                                                        |
| `cert-dcl51-cpp`                                                | Enabled | Commit `Rename reserved identifiers`: enforced with `bugprone-reserved-identifier`, which it aliases.                                                                                                                                                                                                                                                                                                                        |
| `clang-diagnostic-reserved-identifier`                          | Enabled | Commit `Rename reserved identifiers`: the same 133 renames cleared all 144 compiler reports.                                                                                                                                                                                                                                                                                                                                 |
| `clang-diagnostic-reserved-macro-identifier`                    | Enabled | Commit `Rename reserved identifiers`: `_MAX_*` are `kMaxPath` and friends (105 uses), `_USERENTRY` and TD's `_RETRIEVE` are gone, removing the preprocessing warning `CLAUDE.md` names as the `clang-tidy-cache` blocker (not re-measured). See review below.                                                                                                                                                                |
| `clang-diagnostic-invalid-source-encoding`                      | Enabled | Commit `Enable ten checks the tree already satisfies`: no findings across 460 translation units; a probe confirms it reports.                                                                                                                                                                                                                                                                                                |
| `portability-template-virtual-member-function`                  | Skipped | Commit `Record the remaining P3 policy decisions`: 35 virtual members of the heap, vector and mix-file templates; silencing it needs explicit instantiation of every specialization or devirtualizing them. See review below.                                                                                                                                                                                                |
| `portability-avoid-pragma-once`                                 | Enabled | Commit `Enable ten checks the tree already satisfies`: no findings across 460 translation units; a probe confirms it reports.                                                                                                                                                                                                                                                                                                |

## P4 — Cleanup and design consistency

| Check                                                               | Status  | Reason / result                                                                                                                                                                                                                                                                                              |
| ------------------------------------------------------------------- | ------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `readability-braces-around-statements`                              | Enabled | Commit `Enforce the remaining P4 fix-it checks`: every control-statement body is braced, as `.clang-format`'s `InsertBraces` already expects.                                                                                                                                                                |
| `hicpp-braces-around-statements`                                    | Legacy  | Unavailable in LLVM 23; review with `readability-braces-around-statements` on older tools.                                                                                                                                                                                                                   |
| `readability-inconsistent-ifelse-braces`                            | Enabled | Commit `Apply the mechanical P4 fix-its`: no reports; enabled to keep it that way.                                                                                                                                                                                                                           |
| `readability-avoid-nested-conditional-operator`                     | Enabled | Commit `Enforce the remaining P4 fix-it checks`: 182 language selections in `ra/wolstrng.cc` use a `noexcept` `Localized` helper; the other build-language picks are immediately invoked lambdas. See review below.                                                                                          |
| `readability-function-cognitive-complexity`                         | Skipped | Commit `Record the P4 policy decisions`: 607 functions over the default threshold of 25; complexity alone is not a defect, and splitting them needs behavior coverage first. See review below.                                                                                                               |
| `readability-function-size`                                         | Enabled | Commit `Enforce the remaining P4 fix-it checks`: the two oversized legacy dialog loops carry reasoned suppressions.                                                                                                                                                                                          |
| `google-readability-function-size`                                  | Enabled | Commit `Enforce the remaining P4 fix-it checks`: enforced with `readability-function-size`, which it aliases.                                                                                                                                                                                                |
| `hicpp-function-size`                                               | Legacy  | Unavailable in LLVM 23; review with `readability-function-size` on older tools.                                                                                                                                                                                                                              |
| `readability-magic-numbers`                                         | Skipped | Commit `Record the P4 policy decisions`: 17,934 reports; the largest groups are the bignum arithmetic in `tech/mp.cc`, dialog and score-screen layout coordinates, and unit data tables. Naming every literal would bury the constants that matter. See review below.                                        |
| `cppcoreguidelines-avoid-magic-numbers`                             | Skipped | Commit `Record the P4 policy decisions`: alias of `readability-magic-numbers`, skipped with it.                                                                                                                                                                                                              |
| `readability-enum-initial-value`                                    | Enabled | Commit `Apply the mechanical P4 fix-its`: partly initialized enums initialize every enumerator; one fix-it misplaced a value after an attribute and was repaired. See review below.                                                                                                                          |
| `cert-int09-c`                                                      | Enabled | Commit `Apply the mechanical P4 fix-its`: enforced with `readability-enum-initial-value`, which it aliases.                                                                                                                                                                                                  |
| `readability-named-parameter`                                       | Enabled | Commit `Apply the mechanical P4 fix-its`: unnamed parameters carry a commented name. See review below.                                                                                                                                                                                                       |
| `hicpp-named-parameter`                                             | Legacy  | Unavailable in LLVM 23; review with `readability-named-parameter` on older tools.                                                                                                                                                                                                                            |
| `readability-isolate-declaration`                                   | Enabled | Commit `Enforce the remaining P4 fix-it checks`: each declaration statement declares one variable.                                                                                                                                                                                                           |
| `readability-avoid-unconditional-preprocessor-if`                   | Enabled | Commit `Enforce the remaining P4 fix-it checks`: TD's `#if (true)` and two `#if (false)` blocks are resolved.                                                                                                                                                                                                |
| `readability-redundant-nested-if`                                   | Enabled | Commit `Enforce the remaining P4 fix-it checks`: nested ifs without an else are single `&&` conditions; comments stranded before the brace moved above the if. See review below.                                                                                                                             |
| `readability-trivial-switch`                                        | Enabled | Commit `Enforce the remaining P4 fix-it checks`: 24 single-case and default-only switches are if/else or plain blocks, keeping their comments. See review below.                                                                                                                                             |
| `modernize-use-bool-literals`                                       | Enabled | Commit `Apply the mechanical P4 fix-its`: integer literals assigned to `bool` are `true`/`false`. It exposed TD's `CCMessageBox::Process` holding a three-way answer in a `bool`; that is an `int` now. See review below.                                                                                    |
| `readability-const-return-type`                                     | Enabled | Commit `Apply the mechanical P4 fix-its`: `Rect::Intersect` and `Union` return plain `Rect`; TD's deleted `CCFileClass` copy assignment lost the same `const`. See review below.                                                                                                                             |
| `clang-diagnostic-ignored-qualifiers`                               | Enabled | Commit `Apply the mechanical P4 fix-its`: no reports; enabled to keep it that way.                                                                                                                                                                                                                           |
| `readability-convert-member-functions-to-static`                    | Enabled | Commit `Enforce the remaining P4 fix-it checks`: members that never use `this` are `static`; fix-its that put `static` after `[[nodiscard]]` were reordered and callers no longer reach them through an instance. See review below.                                                                          |
| `modernize-use-using`                                               | Skipped | Commit `Enforce the remaining P4 fix-it checks`: tried twice. Its fix-its turn `typedef struct {...} Name;` into `using Name = struct {...};`, which leaves the unnamed type without a name for linkage, and they wrote 81 anonymous tag bodies twice even one translation unit at a time. See review below. |
| `modernize-use-auto`                                                | Enabled | Commit `Enforce the remaining P4 fix-it checks`: declarations that repeated the type of a `new` expression or cast use `auto`. See review below.                                                                                                                                                             |
| `hicpp-use-auto`                                                    | Legacy  | Unavailable in LLVM 23; review with `modernize-use-auto` on older tools.                                                                                                                                                                                                                                     |
| `modernize-return-braced-init-list`                                 | Enabled | Commit `Apply the mechanical P4 fix-its`: returns that repeated the return type use braces.                                                                                                                                                                                                                  |
| `modernize-use-designated-initializers`                             | Skipped | Commit `Record the P4 policy decisions`: 2,431 reports, mostly positional rows of data tables: both games' vehicle track tables in `drive.cc`, infantry data and audio tables. See review below.                                                                                                             |
| `readability-uppercase-literal-suffix`                              | Enabled | Commit `Apply the mechanical P4 fix-its`: literal suffixes are upper case. See review below.                                                                                                                                                                                                                 |
| `cert-dcl16-c`                                                      | Enabled | Commit `Apply the mechanical P4 fix-its`: enforced with `readability-uppercase-literal-suffix`, which it aliases.                                                                                                                                                                                            |
| `hicpp-uppercase-literal-suffix`                                    | Legacy  | Unavailable in LLVM 23; review with `readability-uppercase-literal-suffix` on older tools.                                                                                                                                                                                                                   |
| `readability-trailing-comma`                                        | Skipped | Commit `Record the P4 policy decisions`: 1,027 reports. With a trailing comma clang-format puts one element per line, which turned 3,239 compact initializer-list entries (1,425 in `ra/netdlg.cc` alone) into single lines; the check has no enum-only mode. See review below.                              |
| `clang-diagnostic-missing-noreturn`                                 | Enabled | Commit `Enforce the remaining P4 fix-it checks`: eleven always-exiting functions are `[[noreturn]]` on their first declaration, and the 16 returns after TD's `Validate_Error` calls are gone. See review below.                                                                                             |
| `clang-diagnostic-nrvo`                                             | Enabled | Commit `Enforce the remaining P4 fix-it checks`: `Rect::Intersect` and `Union` return one named result, checked equivalent over 390,625 rectangle pairs. See review below.                                                                                                                                   |
| `performance-no-int-to-ptr`                                         | Enabled | Commit `Enforce the remaining P4 fix-it checks`: shape-buffer and mono-page code advance pointers instead of round-tripping through integers; two trigger-loader casts carry reasoned suppressions. See review below.                                                                                        |
| `performance-enum-size`                                             | Skipped | Commit `Record the P4 policy decisions`: 502 reports; narrowing enum storage changes class layouts and integer promotion at every arithmetic use, so it belongs per enum with a measured saving. See review below.                                                                                           |
| `clang-diagnostic-padded-bitfield`                                  | Enabled | Commit `Apply the mechanical P4 fix-its`: no reports; enabled to keep it that way.                                                                                                                                                                                                                           |
| `clang-diagnostic-ms-bitfield-padding`                              | Enabled | Commit `Enforce the remaining P4 fix-it checks`: RA's `RulesClass` flags are all `bool` bit-fields; the class is re-read from INI on load, so saves are unaffected. See review below.                                                                                                                        |
| `clang-diagnostic-weak-vtables`                                     | Skipped | Commit `Record the P4 policy decisions`: 33 reports; anchoring each vtable only trims build output. See review below.                                                                                                                                                                                        |
| `cppcoreguidelines-avoid-const-or-ref-data-members`                 | Skipped | Commit `Record the P4 policy decisions`: 13 reports: the serializer and archive helpers' reference members and the queues' `const` capacity, none of which is ever reassigned by design. See review below.                                                                                                   |
| `cppcoreguidelines-avoid-non-const-global-variables`                | Skipped | Commit `Record the P4 policy decisions`: 1,399 reports; removing globals is architecture work `CLAUDE.md` defers. See review below.                                                                                                                                                                          |
| `cppcoreguidelines-non-private-member-variables-in-classes`         | Skipped | Commit `Record the P4 policy decisions`: 1,219 reports; encapsulating the legacy classes is a hierarchy refactor. See review below.                                                                                                                                                                          |
| `misc-non-private-member-variables-in-classes`                      | Skipped | Commit `Record the P4 policy decisions`: 2,394 reports over the same exposed members. See review below.                                                                                                                                                                                                      |
| `misc-multiple-inheritance`                                         | Skipped | Commit `Record the P4 policy decisions`: 11 reports: the aircraft, anim, bullet, techno and terrain classes in both games and `GraphicBufferClass`; changing them is a hierarchy refactor. See review below.                                                                                                 |
| `misc-no-recursion`                                                 | Skipped | Commit `Record the P4 policy decisions`: 47 reports, mostly mutually recursive virtual calls through the object model (`Assign_Destination`, `Transmit_Message`, `Do_Action`), bounded by game state. See review below.                                                                                      |
| `cppcoreguidelines-avoid-do-while`                                  | Skipped | Commit `Record the P4 policy decisions`: 100 reports; do-while is valid and often the clearest loop. See review below.                                                                                                                                                                                       |
| `clang-diagnostic-global-constructors`                              | Skipped | Commit `Record the P4 policy decisions`: 1,143 reports, 716 of them the static type-class tables in the `*data.cc` files. See review below.                                                                                                                                                                  |
| `clang-diagnostic-exit-time-destructors`                            | Skipped | Commit `Record the P4 policy decisions`: 838 reports, 706 of them the same static tables. See review below.                                                                                                                                                                                                  |
| `clang-diagnostic-lifetime-safety-intra-tu-suggestions`             | Enabled | Commit `Enforce the remaining P4 fix-it checks`: parameters and implicit objects a returned reference or pointer can outlive are `ABSL_ATTRIBUTE_LIFETIME_BOUND`. See review below.                                                                                                                          |
| `clang-diagnostic-lifetime-safety-intra-tu-constructor-suggestions` | Enabled | Commit `Enforce the remaining P4 fix-it checks`: constructor parameters kept by reference or pointer are annotated; see review below.                                                                                                                                                                        |
| `clang-diagnostic-lifetime-safety-cross-tu-suggestions`             | Enabled | Commit `Enforce the remaining P4 fix-it checks`: annotated with the intra-TU suggestions; see review below.                                                                                                                                                                                                  |
| `clang-diagnostic-lifetime-safety-cross-tu-constructor-suggestions` | Enabled | Commit `Enforce the remaining P4 fix-it checks`: annotated with the intra-TU constructor suggestions; see review below.                                                                                                                                                                                      |
| `clang-diagnostic-date-time`                                        | Enabled | Commit `Apply the mechanical P4 fix-its`: no reports; enabled to keep it that way.                                                                                                                                                                                                                           |
| `clang-diagnostic-documentation-unknown-command`                    | Enabled | Commit `Enforce the remaining P4 fix-it checks`: backslashes in doc comments no longer read as documentation commands.                                                                                                                                                                                       |
| `clang-diagnostic-pedantic`                                         | Enabled | Commit `Apply the mechanical P4 fix-its`: no reports; enabled to keep it that way.                                                                                                                                                                                                                           |
| `clang-diagnostic-c99-extensions`                                   | Enabled | Commit `Enforce the remaining P4 fix-it checks`: the shape block's flexible array member carries a reasoned suppression.                                                                                                                                                                                     |
| `clang-diagnostic-nested-anon-types`                                | Skipped | Commit `Record the P4 policy decisions`: 20 anonymous structs in anonymous unions, an extension GCC, Clang and MSVC all accept. See review below.                                                                                                                                                            |
| `clang-diagnostic-nullability-extension`                            | Skipped | Commit `Record the P4 policy decisions`: 278 reports, all Abseil's `absl_nonnull`/`absl_nullable` expanding inside `CHECK` macros. See review below.                                                                                                                                                         |
| `misc-confusable-identifiers`                                       | Enabled | Commit `Enforce the remaining P4 fix-it checks`: five key-name constants that spell their key carry reasoned suppressions.                                                                                                                                                                                   |
| `bugprone-copy-constructor-mutates-argument`                        | Covered | Already enforced through `cert-oop58-cpp`; reconcile the excluded name.                                                                                                                                                                                                                                      |
| `cert-arr39-c`                                                      | Covered | Already enforced through `bugprone-sizeof-expression`; reconcile the excluded name.                                                                                                                                                                                                                          |
| `cert-err33-c`                                                      | Covered | Already enforced through `bugprone-unused-return-value`; reconcile the excluded name.                                                                                                                                                                                                                        |
| `cert-exp42-c`                                                      | Covered | Already enforced through `bugprone-suspicious-memory-comparison`; reconcile the excluded name.                                                                                                                                                                                                               |
| `cert-oop54-cpp`                                                    | Covered | Already enforced through `bugprone-unhandled-self-assignment`; reconcile the excluded name.                                                                                                                                                                                                                  |
| `cppcoreguidelines-c-copy-assignment-signature`                     | Covered | Already enforced through `misc-unconventional-assign-operator`; reconcile the excluded name.                                                                                                                                                                                                                 |
| `cppcoreguidelines-explicit-virtual-functions`                      | Covered | Already enforced through `modernize-use-override`; reconcile the excluded name.                                                                                                                                                                                                                              |
| `cppcoreguidelines-narrowing-conversions`                           | Covered | Already enforced through `bugprone-narrowing-conversions`; reconcile the excluded name.                                                                                                                                                                                                                      |
| `llvm-else-after-return`                                            | Covered | Already enforced through `readability-else-after-return`; reconcile the excluded name.                                                                                                                                                                                                                       |
| `llvm-qualified-auto`                                               | Covered | Already enforced through `readability-qualified-auto`; reconcile the excluded name.                                                                                                                                                                                                                          |

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
| `misc-predictable-rand`                       | Enabled       | Commit `Enable concurrency-mt-unsafe`: no `rand` or `srand` call remains.          |
| `cert-msc30-c`                                | Enabled       | Alias of `misc-predictable-rand`; enabled with it.                                 |
| `cert-msc50-cpp`                              | Enabled       | Alias of `misc-predictable-rand`; enabled with it.                                 |
| `clang-analyzer-security.insecureAPI.rand`    | Enabled       | Commit `Enable concurrency-mt-unsafe`: nothing left for it to report.              |
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

Measured in the combined sweep, and retained as exclusions. `clang-diagnostic-sign-conversion` was
reviewed here too and later enabled; see the sign conversion review.

- `bugprone-signed-bitwise` reported 4,191 uses of a signed operand with a bitwise operator, 3,715
  with `IgnorePositiveIntegerLiterals`. About 1,800 are OR'd flag enumerators in the unit, building,
  infantry and terrain data tables (`td/bdata.cc` alone has 643); the rest are the VQA loader,
  blitters, big-integer code and random straw, all manipulating bit patterns on purpose. Making the
  flag enums unsigned is the useful version of this change and belongs with a flag-type redesign.
- `bugprone-easily-swappable-parameters` reported 426 functions with adjacent same-typed parameters,
  almost all coordinates, sizes and IDs in the drawing, gadget and type APIs. Strong types for those
  would touch most call sites for little defect value.

### Sign conversion review (2026-09-13)

`clang-diagnostic-sign-conversion` is now enforced, and `-Wno-sign-conversion` is gone from
`CMakeLists.txt`. The 2026-09-12 policy skipped it because each report looked like a `static_cast`.
Measured again with each translation unit's own flags plus `-Wsign-conversion`, there were 2,414
reports (RA 1,191, TD 866, winvq 149, tech 148, sdllib 58, port 2), and most went away by fixing the
type the value comes from:

| Change                                                                                                                                               | Reports   |
| ---------------------------------------------------------------------------------------------------------------------------------------------------- | --------- |
| Shared libraries: VQA player `long` fields to `int32_t` numbers and `uint32_t` flags, WSA and audio sizes, big-integer bit counts, LZW/LZO byte data | 357       |
| RA heap `Ptr`/`Raw_Ptr` take `int`; TD vector capacity is `base::ssize`, with RA's subscript `DCHECK`                                                | 319       |
| Money functions take `int`; text print colors and `String_Pixel_Width` are `int`; `Cardinal_To_Fixed`/`Fixed_To_Cardinal` take `int`                 | about 370 |
| Connection timing: `Set_Timing`, retry delay, retries and timeout are `int32_t` with `-1` for no limit; the connection clock is `int64_t`            | 313       |
| Null-modem, IPX and Westwood Online dialogs                                                                                                          | 323       |
| Gameplay and coordinates: target values, stage, radar origin, unit limits and `MaxStrength` signed at the same width; prerequisite masks `uint64_t`  | 417       |
| Menus, options, profiles, scores, keyframes and the other UI code                                                                                    | 319       |

What remains explicit is a crossing the reader should see:

- `base::ToSize` and `base::ToSigned` (`base/numeric.h`) convert counts at `memcpy`, `new[]`,
  container and `size_t` library parameters, and check the value in debug builds.
- Packing a signed component into a `COORDINATE`, `TARGET` or `LEPTON`, or reading one back, is a
  `static_cast` at that point.
- CRC inputs are cast to the CRC's unsigned type. The CRC bits are unchanged. (TD's CRC has since
  become `uint32_t`; see the runtime integer review.)
- The sdllib seek and size wrappers keep plain casts: callers pass negative `SEEK_CUR` offsets and
  receive `ftell`'s -1 through `size_t`, which a checked conversion would reject.

Behavior is preserved. The fixed-point helpers still compute in `uint32_t`, and new tests compare
them with the original implementations over the game's range. Serialized fields and wire structs
kept their widths; the headless RA and TD save/load checks pass. Two consequences are intended: a
64-bit build now sends RA's unit tracker totals as the 4-byte values the stats packet declares, and
the VQA frame clock goes negative instead of wrapping to a huge tick count.

Reviewing the code turned up suspected defects, which were then fixed with tests:

- `CELL` values used as a `COORDINATE` or `TARGET` in the chronal vortex, fallback build placement,
  editor Home key, house defeat and TD edge reinforcements; the pathfinding overlap words are now
  32-bit, TD's `Overlap_Bit` no longer shifts by -1, and `Adjacent_Cell` checks the index before
  forming the pointer.
- The VQA FINF seek table read into 8-byte `long` entries, codebook, palette and pointer chunks that
  could overflow their buffers, drawer origins other than top-left, and chunk sizes of 2 GiB.
- TD packet IDs, which were 64-bit on Linux and so never wrapped to the first packet: acknowledged
  packets were never delivered. The oldest unacknowledged packet search no longer stops working
  after 2^32 ticks.
- A WSA clear truncated to 16 bits and unchecked frame sizes, corrupt MIX headers, LZO/LCW/LZW block
  counts trusted from the stream, an unchecked LZO decoder, and the stubbed LCW compressor, which
  wrote empty map packs.
- TD object validation exited with status 0, which ended the TD test binary early as a success.

### Runtime integer review (2026-09-13)

`google-runtime-int` is now enforced, with `TypeSuffix: '_t'` so its messages name `int64_t` rather
than `int64`. The 2,717 reports in the 2026-09-12 policy table were stale. Measured again across
both games and shared code, the Linux build reported 2,179 locations in 338 files: 738 `long`, 282
`unsigned long`, 634 `short` and 525 `unsigned short`. They were fixed in five commits:

| Commit                                                          | Reports |
| --------------------------------------------------------------- | ------- |
| `Spell 16-bit integers as fixed-width types`                    | 1,159   |
| `Use fixed-width integers in file, packet and codec interfaces` | 310     |
| `Use fixed-width integers in sdllib and the VQA player`         | 293     |
| `Use fixed-width integers in Red Alert`                         | 235     |
| `Use fixed-width integers in Tiberian Dawn`                     | 182     |

`short` and `unsigned short` became `int16_t` and `uint16_t`. Those are the same types on every
supported platform, so that commit changes no layout, overload or behavior. This includes the RA
`LEPTON` and TD `TARGET` typedefs.

`long` is 64 bits on Linux and macOS and 32 bits on Windows, so each `long` site needed a width:

- Wire, file-format and protocol fields became 32-bit, the width the original Win32 build used.
  Packet `TYPE_LONG` fields had been sent as 8 bytes on LP64 and are 4 bytes again. TD's frame CRC
  is `uint32_t` like RA's; `EventLength` is computed from `sizeof`, so peers stay consistent, and
  the CRC's low 32 bits are unchanged.
- The `FileClass` chain and the `Read_File` family use `int32_t`, casting at the `int64_t` POSIX
  boundary. `Buffer` sizes are `base::ssize`, `Alloc` takes `base::ssize`, and `Mem_Copy` takes
  `size_t`.
- Tick counts, credits and timers that already carried 64-bit values kept 64 bits (`int64_t`), as
  did the multiprecision trial-quotient intermediates and the pathfinding cross product.
- Prerequisite masks are `uint64_t` in both games, like the flag constants they are compared with.
  The houses' deliberate 32-bit truncation of the mask is unchanged.
- The generated Westwood Online header `ra/wolapi/wolapi.h` keeps its Win32 COM spellings under its
  existing `NOLINTBEGIN`, as do the two event-sink overrides that must match it.

No serialized member changed width, so neither game's save version moved; the headless RA and TD
save/load checks pass. New tests cover packet field round trips and wire layout, and Blowfish
against Eric Young's reference vectors.

The migration exposed two defects:

- Old-format RA INI triggers stored a `new[]`'d name string in `CCPtr`'s `int` ID, which truncated
  the pointer on 64-bit builds. The name now waits in `TActionClass::PendingTriggerName`.
- RA's `Get_Buildings` shifted a 32-bit `long` by building numbers above 31 on Windows.

The unused `FileClass::Get_Date_Time`/`Set_Date_Time` virtuals, `Transfer_Block_Size`, and the
legacy two-argument `Get_CPU_Clock` were removed. Enforcement covers code the Linux build compiles.
Windows-only, DOS and unbuilt `winvq` sources still contain `long` and are reported only if they are
built with clang-tidy.

### Implicit bool conversion review (2026-09-13)

`readability-implicit-bool-conversion` is enforced with `AllowPointerConditions` and
`AllowIntegerConditions`, which permit exactly what the Google C++ style guide allows: `if (ptr)`
and `while (count)`. The 6,938 reports of the earlier policy were mostly those conditions; with the
options set, the Linux build reported 1,572, all real crossings between `bool` and integers. The fix
was to change types, not to add casts:

| Commit                                            | Change                                                                                          |
| ------------------------------------------------- | ----------------------------------------------------------------------------------------------- |
| `Store one-bit flags as bool bitfields`           | 597 `unsigned X : 1` flags became `bool X : 1`; 0/1 assignments became `false`/`true`           |
| `Return bool from shared predicates`              | The `FileClass` predicates, `Keyboard::Down`, the viewport `Blit` wrappers, the bignum bit test |
| `Use bool for Red Alert flags and predicates`     | The gadget virtual chain, about 30 predicates and operators, flag globals, members and locals   |
| `Use bool for Tiberian Dawn flags and predicates` | The same for Tiberian Dawn, plus its vector and heap members                                    |

Some values look like flags but carry more states, and stay `int` with explicit `1`/`0`: the dialog
return codes (`-1` means cancelled), `ScenarioInit` and `Blockage` (counters), `Activate`'s -1/0/1
control, the game options sent in packets, the network send and receive status results, and the
recorded `GameOptionsClass`. Layout-sensitive bitfields keep `unsigned`: `SpecialClass`,
`EventClass`, the packet structs and the cell flag unions. Small multi-bit fields next to the new
`bool` flags use `uint8_t` storage so the Microsoft ABI still packs them. Saves copy bitfields
through `bool` temporaries, so the archive format and the save versions are unchanged; the RA and TD
save/load checks pass.

The conversions exposed defects:

- Both games' VQA movie I/O tested `Open(...) == -1`, which a true/false result never matched, so a
  failed open went unnoticed.
- RA scenario loading compared `CCINIClass::Load`'s `bool` result with 2 to catch a bad digest; the
  branch could never run and is removed.
- Three TD calls pass `true` to `Scatter()`'s `COORDINATE` threat parameter, probably meaning a
  forced scatter. The value (1) is kept, with a comment, so behavior does not change.

A TD save/load run crashed once at exit while this landed: the SDL audio callback reads sample data
from a mix file that `Uninit_Game` has already freed. The race predates this work and is not fixed
here.

### Const correctness review (2026-09-13)

`misc-const-correctness` is enforced with LLVM 23's defaults except `WarnPointersAsPointers: false`.
With the defaults, the Linux build reported 4,072 variables that are never modified. The check's
fix-its, collected per translation unit and merged with `clang-apply-replacements`, add 4,126
`const` qualifiers in three commits; about 510 of them make a pointer or reference point to `const`,
and 209 are range-for variables:

| Commit                                                           | Files |
| ---------------------------------------------------------------- | ----- |
| `Declare never-modified variables const in the shared libraries` | 62    |
| `Declare never-modified variables const in Red Alert`            | 153   |
| `Declare never-modified variables const in Tiberian Dawn`        | 120   |

Pointer parameters that became pointer-to-`const` changed in their header declarations too. Once
WSA's `Apply_XOR_Delta` stopped casting its delta to a mutable pointer,
`readability-non-const-parameter` asked for `const char*` on that parameter as well. Adding `const`
changes no value, class layout or save format, and a `const` local can still be elided on return.
The full strict build of both games is clean, all 359 tests pass, and the RA and TD save/load checks
report identical state.

**Pointee analysis.** The default also asks for `const T*` on pointers whose target is never
written. LLVM 23 misses three kinds of write, and about 40 of the 454 pointee fix-its did not
compile:

- A write through a post-incremented pointer, `*p++ = value`: the blitters, palette fades, the
  base64 and Blowfish coders and the bignum routines in `tech/mp.cc`.
- An array of pointers used to call non-const members or swapped in place: the dialog `buttons`
  arrays, the radar terrain sort and the techno defender sort.
- A parameter of a function whose address is stored in a `void (*)(char*)` hook, as both games'
  `Print_Error_End_Exit` is in `Memory_Error_Exit`.

```cpp
void Fill(unsigned char* dest) {
  unsigned char* p = dest;  // "pointee of variable 'p' ... can be declared 'const'"
  *p++ = 0;
}
```

The pointee fix-its that compiled are kept. Warning on the rest would mean a `NOLINT` on every
output-pointer loop, so the option is off; revisit it after an LLVM upgrade.

**What the fix-its could not do.** Thirteen function-local `static struct { ... } table[]` lookup
tables report with an empty fix-it; they are `static const struct` by hand. The fix-its also place
`const` after the type (`int const x`). `.clang-format` now sets `QualifierAlignment: Left`, which
moved those to the tree's `const int x` spelling; it leaves all-caps typedefs alone because they
could be macros, so the 345 lines with `CELL`, `COORDINATE`, `TARGET`, `LEPTON`, `HRESULT` and
`DWORD` were reordered by script. Elsewhere the setting would move 23 qualifiers in the unbuilt
`winvq/vqm32` and `vqaview` sources, which are left as they are.

**Suppressions.** Four variables carry `NOLINTNEXTLINE(misc-const-correctness)`, all template false
positives. Both games' save tests call the `ArchiveWriter` and `ArchiveReader` call operators, which
are non-const, with a template-dependent argument the check does not see as a mutation. TD's
`SerializeObjectList` sets its `seen` bitset only when reading, and the check reports it from the
writer instantiation, where that `if constexpr` branch is discarded.

### Include cleaner review (2026-09-13)

`misc-include-cleaner` is now enforced. For each `.cc` file it reports a symbol whose declaring
header is not included directly, and an include that nothing in the file uses: 3,285 reports, 2,993
missing includes and 292 unused ones in 308 files. The fix-its, collected per translation unit and
merged with `clang-apply-replacements`, add 767 includes and remove 312 across 303 files.

The earlier review kept it excluded because include-what-you-use already runs in the strict build.
IWYU runs without `--error`, though, so its suggestions never fail a build; this check is the first
include rule that does. IWYU keeps running as advice.

**glibc private headers.** For `htonl`, `in_addr`, `SOL_SOCKET` and `timeval` the check names the
header glibc declares them in (`<netinet/in.h>`, `<asm-generic/socket.h>`,
`<bits/types/struct_timeval.h>`) and calls the POSIX `<arpa/inet.h>` and `<sys/time.h>` includes
unused. Those includes sit in the non-Windows branch that macOS shares, so `IgnoreHeaders` exempts
the five spellings, which accounted for 33 reports.

**One file, two configurations.** TD's `startup.cc` is also compiled into two tests with
`TD_NO_ENTRY_POINT`, which drops `main()` and the 17 headers only it uses. The fix-its from the two
configurations contradict each other, so the file was fixed by hand: those headers now sit under the
same `#ifndef`.

**Uses the check does not see.** Removing `techno.h` from both games' `radio.cc` and `terrain.h`
from RA's `target.cc` broke implicit derived-to-base pointer conversions. Removing
`td/vector_impl.h` from `vector.cc` and `heap_test.cc` left TD's explicit vector instantiations
without member definitions, which failed only at link time. The five includes are back under
`// IWYU pragma: keep`.

**Conflicts and odd suggestions.** `explicit_bzero` is declared only in `<string.h>`, which
`modernize-deprecated-headers` rejects; `tech/rndstraw.cc` includes it under a `NOLINT`. Both
`fly.cc` files called the global `div` with `<cstdlib>` included, and the check traced that
declaration to `absl/log/check.h`; they call `std::div` now. The two `absl` includes the fix-its
spelled with angle brackets use quotes.

**Scope.** The check analyzes only a translation unit's main file. Headers are compiled through the
`*_verify_interface_header_sets` wrappers, whose main file is the generated `.cxx`, so includes in
headers remain IWYU's advice. Only the Linux configuration is analyzed. An include inside
`#ifdef _WIN32` is invisible to it, but one outside such a block whose only uses are inside would
read as unused; the removals were checked for that, and the network headers are the case
`IgnoreHeaders` covers.

Includes change no code. The full strict build of both games is clean, all 359 tests pass, and the
RA and TD save/load checks report identical state. A probe that includes `<vector>` without using it
confirms the check reports an error.

### Member function const review (2026-09-13)

`readability-make-member-function-const` is now enforced. It reports a member function that writes
no field of its own object and calls nothing non-const on it: bitwise constness, 165 reports. The
earlier review kept it excluded because many of those functions change game or network state through
globals and pointer members. This time each report was judged by logical constness instead, whether
the function leaves alone the state it is responsible for:

| Result                                         | Functions |
| ---------------------------------------------- | --------- |
| Declared `const` with the check's fix-its      | 117       |
| Kept non-const under a reasoned suppression    | 45        |
| Deleted as dead no-ops                         | 3         |
| Declared `const` once their callees were const | 5         |

**Declared `const`.** Accessors (the connection, queue, radar, vortex, viewport, mouse and heap
getters), comparisons, coordinate conversions (`Coord_To_Pixel`, `Push_Onto_TacMap`,
`Cell_On_Radar`, `Click_In_Radar`), drawing that only reads the object (`Draw_Names`,
`Render_Terrain`, `EgoClass::Render`, `TeamMissionClass::Draw_It`), formatting and saving
(`Save_Settings`, `WritePlayerListItem`, `SetGParamsToCurrent`), and both message boxes, whose
`Process` shows a modal box and returns the button without changing the box. RA's
`BaseNodeClass::operator!=` and the other two `Process` overloads in each game followed once their
callees were `const`. RA's `xTargetClass::operator==` now takes a `const` reference, since a `const`
operator with a mutable parameter is ambiguous with its C++20 reversed form.

**Suppressed.** Functions that exist to change state keep no qualifier, under a comment that names
the state (`// Not const: sends a chat request.`) and a `NOLINTNEXTLINE`:

- `CellClass::Incoming`, `Adjust_Threat` and `Shimmer`, and RA's `Goodie_Check`, which act on the
  cell's occupants, the houses' threat tables and the crate.
- `DisplayClass::Select_These`, `EventClass::Execute`, RA's `TActionClass::operator()` and
  `ChronalVortexClass::Set_Redraw`.
- The null-modem `Dial_Modem`, `Answer_Modem` and `Hangup_Modem`, which drive the serial port, and
  twelve Westwood Online chat requests (`Kick`, `Ban`, `Squelch`, `ChannelJoin`, `SendGo`, ...).
- `OptionsClass::One_Time`, the sidebar strips' `Activate` and `Deactivate`, and the score screen's
  `Do_Nod_Buildings_Graph` and `Do_Nod_Casualties_Graph` animations.
- TD's `GameOptionsClass::Process`, `InfantryClass::Clear_Occupy_Bit`,
  `TeamClass::Coordinate_Conscript` and `TechnoClass::Base_Is_Attacked`.

**Deleted.** RA's `MapClass::Shroud_From` had no callers. TD's `BuildingClass::Update_Specials` and
`HouseClass::Detach` only ran `Validate()`. Dropping the `Update_Specials` call left a nested `if`,
merged for `readability-redundant-nested-if`.

**Knock-on checks.** `modernize-use-nodiscard` reports only `const` member functions, so the newly
`const` ones that return a value added 90 reports; the fix-its applied 88 attributes, and the
message boxes' `int` `Process` overload stays without one (see the nodiscard review). The `--format`
pass of `clang-apply-replacements` split the one-line accessors in both `combuf.h` headers around
their trailing comments; those comments now sit above the accessors.

Adding `const` changes no value, class layout or save format. The full strict build of both games is
clean, all 359 tests pass, and the RA and TD save/load checks report identical state. A probe class
with a non-const getter confirms the check reports an error.

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

Three P3 names are retained because the change they ask for is one the project's own rules steer
away from. CLAUDE.md's legacy-code section lists smart pointers everywhere and STL containers
everywhere under changes to avoid unless requested. Measured in the combined sweep:

| Check                                                               | Reports | What enabling would require                                     |
| ------------------------------------------------------------------- | ------- | --------------------------------------------------------------- |
| `cppcoreguidelines-owning-memory`                                   | 1,240   | `gsl::owner` or smart pointers on every raw `new`/`delete`      |
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

The five bounds checks describe one property from different angles: the engine walks raw buffers.
The blitters, the LCW/LZO/LZW codecs, audio mixing, the crypto code and the packet and save readers
all index and advance pointers into memory whose size lives elsewhere. Enforcing them one site at a
time would add casts and suppressions without adding a bound; the useful change is a span-based
buffer API that carries the size, after which these checks can be revisited per module. The
`port::AlignedObject` and typed buffer helpers already added for cast alignment are the start of
that API.

The union reports are the event, target and packet unions, whose layouts are part of the network and
savegame formats; replacing them with variants changes those formats.

`readability-implicit-bool-conversion` was later enabled with the options that allow pointers and
integers as conditions; see the implicit bool conversion review.

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
do module by module, after which this check can be revisited. That work is done; see the next
section.

### Const-dropping cast enablement (2026-09-14)

`clang-diagnostic-cast-qual` is now enforced. A fresh sweep
(`-Weverything -Wcast-qual -fsyntax-only` over the 475 project translation units of the strict
compile database) found 387 sites: ra 217, td 139, sdllib 17, tech 14. Every one was fixed at the
declaration that lied, in eleven commits, and none with `const_cast`; the fix-its were not used
because they only spell the same cast differently. Class downcasts became `dynamic_cast` to the
const type, since `cppcoreguidelines-pro-type-static-cast-downcast` is also enforced.

| Group                                          | Sites | Fix                                                                                                                                                       |
| ---------------------------------------------- | ----- | --------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Read-only downcasts of `this` and const params | 119   | Cast to the const type; `Adjacent_Cell` overloads share `Adjacent_Offset`; TD's facing pun became enum arithmetic.                                        |
| Receivers of `MixArchive::Retrieve` data       | ~45   | `const void*` members and locals; `Extract_Shape`, the mouse cursor setters, `LCW_Uncompress` and `Buffer_To_Page` take const data.                       |
| String handles                                 | ~25   | `const char*` locals; `WWGetPrivateProfileString` returns const; four dialogs stop formatting the shared text table in place.                             |
| Const members that mutate                      | ~35   | `What_Action(ObjectClass*)`, `Ok_To_Move`, `Find_Docking_Bay`, `Base_Is_Attacked`, `CCINIClass::Save`, team creation are non-const; caches are `mutable`. |
| Type tables patched at load                    | 36    | `mutable` image, cameo, radar-icon, buildup and animation fields behind const setters; the tables stay const.                                             |
| TD house colors                                | 14    | `Color`/`BrightColor` live in `HouseClass`, seeded from the type and serialized.                                                                          |
| Dead uncompressed-shape cache                  | 2     | `UseBigShapeBuffer` was never true; the header-stamping path and RA's unbuilt `keyframe.cc` are deleted.                                                  |
| Codecs and allocator                           | 10    | Blowfish drops its null-destination aliasing; LZW takes spans; `Free` takes `void*`; dead `GenericNode::Main_List` deleted.                               |
| List-box items                                 | 50    | `ListClass` owns `std::vector<std::string>` items with `Set_Item`/`Clear`; the dialogs stop allocating and freeing lines themselves.                      |

Four bugs came out of it. TD's `Net_Fake_New_Dialog`, `Net_Fake_Join_Dialog` and the internet
dialog, and RA's file-transfer dialog, passed `Text_String()` results to `Format_Window_String`,
which inserts line breaks in place, so every call corrupted the shared string table (and the later
prints depended on it). `Base_Is_Attacked` set a timer on an enemy it took as `const`. TD wrote each
player's chosen colors into the shared `HouseTypeClass`, so two houses of one type clobbered each
other. `BlowfishEngine::Encrypt`/`Decrypt` with a null destination wrote over the const source; no
caller used it, and a test now covers explicit in-place use.

GCC's `-Wcast-qual`, which the GCC strict flag set already carries, also reports C-style casts to
plain `void*`, which clang's does not. A GCC syntax-only sweep of the same translation units found
four more, the largest being `Add_Long_To_Pointer` returning `void*` for a `const void*` input; it
has const and non-const overloads now, and both sweeps report zero. The GCC sweep needs the strict
database's commands with the clang-only flags removed and **without** `-w`, which silences every
later `-W` flag and reads as a clean tree.

Verification: clang-tidy with the enabled configuration over all 475 translation units, both sweeps
at zero, a probe confirming the check reports, CTest (438 tests, including the new `list_test.cc`),
and the RA and TD save/load smoke tests including the TD team and building fixtures. The network and
modem dialogs, whose list boxes changed most, were not exercised at run time; that remains a manual
check.

Follow-ups noted while working: `tech/2keyfbuf.cc` still reads the three big-shape-buffer globals
that are now always false, `IsTheaterShape` is set but never read, and TD's `Get_Last_Frame_Length`
is always zero.

### C-style cast enablement (2026-09-14)

`cppcoreguidelines-pro-type-cstyle-cast` is now enforced; the plan is
[CSTYLE_CAST_PLAN.md](CSTYLE_CAST_PLAN.md). The isolated sweep over the 475 translation units of the
strict compile database found 415 sites (327 between unrelated types, 88 downcasts): ra 302, td 84,
tech 25, sdllib 4. The check has no fix-its. Every site was fixed without a bare `reinterpret_cast`
at the call site, since `cppcoreguidelines-pro-type-reinterpret-cast` is also enforced: byte views
go through one documented helper each, and downcasts became `dynamic_cast`.

| Group                                                 | Sites | Fix                                                                                                                                                                                                                  |
| ----------------------------------------------------- | ----- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| WOL text fields, `void*` list data, COM and registry  | 138   | `WolText` for the IDL structs' `unsigned char` text, `ComOut` for COM out-parameters, `port::BytesOf` for registry values, `static_cast` from `void*`; `iChannelLobbyNumber` takes `const char*`.                    |
| Object-model downcasts                                | 99    | `dynamic_cast`, hoisted to one local per block; reference casts where the result is dereferenced unconditionally, so the analyzer sees no null path. `xTargetClass::As_*` and `Contact_With_Whom` moved out of line. |
| Packed coordinate, cell and target words              | 48    | RA: `std::bit_cast` to the `*_COMPOSITE` unions (`CELL_COMPOSITE` now uses `uint16_t` bit-fields so it is the size of a `CELL`). TD: shifts and `HighWord`/`LowWord`; `td/coord_inline_test.cc` pins the old bytes.  |
| Unaligned words, packet structs, sockets, bit helpers | 43    | `port::ReadUnaligned`/`WriteUnaligned` for CRC and length words and packet copies; `SocketAddress(sockaddr_in&)`; `static_cast` from the `const void*` parameters of `jshell.h` and `search.h`.                      |
| Byte views, palettes, graphics buffers                | 43    | `port::BytesOf`; `Scale` remap tables are `const unsigned char*`; `Apply_XOR_Delta` takes `void*`; the PCX palette loops walk bytes instead of `RGB` structs.                                                        |
| `tech/mp.cc` half-word digit views                    | 15    | One file-local `XMP_Halves` overload pair carries the documented `reinterpret_cast`.                                                                                                                                 |
| Connection and serial-port byte sinks                 | 13    | `ConnectionClass::Send`, `Send_To` and `Broadcast` take `void*`; `Compute_CRC` and the serial port read and write take `void*`/`const void*`.                                                                        |
| Defects                                               | 16    | See below.                                                                                                                                                                                                           |

Five bugs came out of it:

- RA's team editor carved two arrays out of `SysMemPage`, sized by a `MAX_TEAM_CLASSES` that left
  out vessels. The vessel type pointers ran into the count array, and zeroing the counts overwrote
  them. The arrays are locals now and the constant counts vessels.
- TD's `MapClass::Validate` rejected any occupier, `Next` or trigger pointer with a bit in
  `0xff000000`, a DOS-era range test that every 64-bit heap pointer fails. The pointer tests are
  gone; the limbo and cell-range tests stay.
- TD's `MonoClass` wrote through `MonoSegment = 0x000b0000` once `MonoClass::Enable()` ran
  (reachable from `init.cc` and `debug.cc`). Its pages now live in memory, as RA's already did.
- `Read_PCX_File` shifted the palette through `char*`, so on signed-char platforms every component
  of 0x80 or more became 0xE0-0xFF instead of a 6-bit value. The palette parameter is
  `unsigned char*` in both games.
- RA's carrier docking stored a `VesselClass*` in a `BuildingClass*` local, and `sendfile.cc` read
  an `int` packet length through an `unsigned int`. Both locals have the honest type now.

Verification: the isolated sweep at zero; a probe confirming the enabled configuration reports a
C-style downcast; the full strict build; CTest (446 tests, including the new
`port/bytes_of_test.cc`, the `SocketAddress` test and `td/coord_inline_test.cc`); and the RA and TD
save/load smoke tests, including the TD team and building fixtures. The network, modem and WOL
dialogs and the TD mono display were not exercised at run time; that remains a manual check.

`modernize-avoid-c-style-cast`, `google-readability-casting` and `clang-diagnostic-old-style-cast`
stay skipped: what remains is numeric casts, which `docs/TYPE_MIGRATION.md` keeps opportunistic.

Follow-ups left on purpose: `tech/mp.cc` still reads `uint32_t` digits through `uint16_t*`, and
`rawolapi.cc` `strtok`s wolapi's server list in place (both marked `TODO`).

### Thread-unsafe function removal (2026-09-15)

`concurrency-mt-unsafe` is now enforced, with `FunctionSet: posix`; the plan is
[MT_UNSAFE_PLAN.md](MT_UNSAFE_PLAN.md). The isolated sweep over the 920 compile-database entries
found 322 sites: 268 `strtok`, 22 `exit`, 21 `rand`, 4 `getenv`, 3 `inet_ntoa`, 2 `gethostbyname`, 2
`glob`. The check has no fix-its and its message never names the function, so the sites were
classified from the source column.

| Group                                                    | Sites | Fix                                                                                                                                                                                                                                                                        |
| -------------------------------------------------------- | ----- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| INI object, team, trigger and rules parsers, phone books | 218   | `port::Tokenizer` (`port/tokenizer.h`): strtok's splitting rules with the cursor in the object. RA's event and action `Read_INI` take the trigger entry's tokenizer as a parameter instead of continuing a global one.                                                     |
| WOL game-setup messages and the server list              | 50    | A tokenizer per message; `Remaining()` for the two length-prefixed strings; the "SetPlayerColor may call strtok" offset hops became plain `Next()` calls; the server list is parsed from a copy (the follow-up left by the cast work); the SKU list uses `absl::StrSplit`. |
| Seeds and picks                                          | 21    | `port::RandomSeed()` for the eight seed sites; `Sim_Random_Pick` for the WOL sound effects; `std::minstd_rand` in the public-key self-test. `randomize`, `IRandom` and `Get_Random_Mask` in `sdllib/misc.cc` and TD's `srand(Seed)` were dead afterwards and are deleted.  |
| Environment, addresses, file search                      | 11    | `port::GetEnv` (a copy, `std::optional<std::string>`); `port::Ipv4Text` (`inet_ntop`); `getaddrinfo` with an `AF_INET` hint in `ra/stats.cc` and `ra/wspudp.cc`; `std::filesystem::directory_iterator` + `fnmatch(FNM_CASEFOLD)` in `sdllib/file.cc`.                      |
| `exit`                                                   | 22    | Kept. `FunctionSet: posix` drops glibc's additions, of which `exit` was the only one still used; its hazard is two threads exiting at once.                                                                                                                                |

Two behaviour changes worth knowing: the WOL setup dialog read `Seed = rand()` before the `srand`
that was meant to seed it, so it was the unseeded glibc sequence and is now a real seed; and the
POSIX file search matches mixed-case names, where the two-pass glob matched only the pattern's own
case and its lowercase.

With no `rand` call left, `misc-predictable-rand`, `cert-msc30-c`, `cert-msc50-cpp` and
`clang-analyzer-security.insecureAPI.rand` are enabled in the same commit.

Verification: the isolated sweep at zero; a probe confirming the enabled configuration reports
`strtok` and `rand`; the full strict build; CTest (456 tests, including the new tokenizer,
environment, seed and address-text tests); and the RA and TD save/load smoke tests, including the TD
team, building, mobile, map and globals fixtures. The phone books, WOL messages and address lookups
are not on the smoke path; that remains a manual check. The excluded-name count drops from 118
to 113.

### Numeric cast enablement (2026-09-15)

`modernize-avoid-c-style-cast`, its alias `google-readability-casting` and
`clang-diagnostic-old-style-cast` are now enforced; the plan is
[NUMERIC_CAST_PLAN.md](NUMERIC_CAST_PLAN.md). The isolated sweep over the 486 translation units of
the strict compile database found 416 unique sites (ra 225, td 185, tech 6), down from the 1,393
recorded on 2026-09-12: the cast-qual and `pro-type-cstyle-cast` series had removed the rest on the
way. `-Wold-style-cast` then found 58 more inside macro expansions, which the tidy check does not
visit.

| Group                                                          | Sites | Fix                                                                                                                                                                                                                                                              |
| -------------------------------------------------------------- | ----- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Key-number case labels in the map editors and debug handlers   | 196   | `case KN_UP \| KN_ALT_BIT:` with no cast; `jshell.h`'s `constexpr operator\|` combines the enumerators and the label converts to the switch's `int`.                                                                                                             |
| Enum and numeric conversions                                   | ~150  | The check's fix-its (`static_cast<T>`), applied with `clang-apply-replacements`; the `jshell.h` enum operator templates and the facing arithmetic in `face.h`, `facing.h` and `defines.h` by hand in the same form.                                              |
| Enumerator promotions                                          | 17    | No cast: `HOUSE_MULTI1 + i`, `Scen.IntroMovie + 1`, `Change_Window(WINDOW_EDITOR)`, `Fear + morefear`.                                                                                                                                                           |
| `(bool)` on parsed integers and registry values                | 8     | `!= 0`.                                                                                                                                                                                                                                                          |
| Casts to the operand's own type                                | 16    | Deleted: the `char[32]` scenario digest, the `unsigned char[]` palette, an `int` sight range, `ObjectClass* Next`, `(UnitClass*)this`, and `((ObjectClass&)*this).Mark(...)`, which dispatched virtually anyway.                                                 |
| Pointer and reference conversions the unsafe check let through | 16    | `static_cast` for the COM upcasts and the `bsearch` result; nothing for `T*` into the `void*` list data; `TDropListClass::Add`/`Remove` use `dynamic_cast` like `DropListClass` already did.                                                                     |
| `(unsigned)index < count` range idiom                          | 11    | `index >= 0 && index < count` in `list.h`, `teamtype.cc` and TD's `display.cc`; `magic_enum::enum_contains` for the trigger special-weapon and quarry fields. The bit-manipulation `(unsigned)` casts in `target.h`, `inline.h` and `foot.cc` are `static_cast`. |
| Casting macros seen only by the compiler                       | 58    | `constexpr` constants: the path-finder `kClockwise`/`kCounterclockwise`/`kEmptyCommand`, the stats packet types, `tech/mp.h`'s `kSemiMask`; `sha.cc` uses `std::min`; the IFF `MAKE_ID` macro is a `constexpr int32_t MakeId(char, char, char, char)`.           |

No defect came out of it; the `(ObjectClass&)` casts around `Mark` and the `(int32_t)` on TD's
already-`int` pixel maths were the only sites that did nothing at all.

Verification: the isolated sweep at zero with `-Wold-style-cast` added; a probe confirming the
enabled configuration reports `(int)x` under both names; the full strict build; CTest (456 tests);
and the RA and TD save/load smoke tests, including the TD team, building, mobile, map and globals
fixtures. The map editors, debug key handlers and WOL dialogs are not on the smoke path; that
remains a manual check. The excluded-name count drops from 113 to 110.

### Variadic printer enablement (2026-09-15)

`cppcoreguidelines-pro-type-vararg`, `modernize-avoid-variadic-functions` and its alias
`cert-dcl50-cpp` are now enforced; the plan is [VARIADIC_PLAN.md](VARIADIC_PLAN.md). The rows had
been skipped because most printers format a translated string looked up at run time, which needs a
runtime-typed formatter. `port::FormatRuntime` is that formatter: it wraps `absl::FormatUntyped`,
which checks each conversion against the argument it receives, and returns the text unformatted when
it is not a format for those arguments, so a translated string holding a stray `%` still prints. The
isolated sweep over the 486 source translation units found 1,599 sites.

| Group                                                                                                                | Sites | Fix                                                                                                                                                                                                                                                                    |
| -------------------------------------------------------------------------------------------------------------------- | ----- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Printers over run-time text: `Fancy_Text_Print`, `Plain_Text_Print`, `MonoClass::Printf(int)`, `Format_Runtime_Text` | 605   | An `absl::Span<const absl::FormatArg>` parameter on the out-of-line function plus a variadic template overload that packs the arguments; call sites unchanged.                                                                                                         |
| Printers every caller passes a literal to: `MonoClass::Printf(const char*)`, `Mono_Printf`, `Smart_Printf`, `Fatal`  | 340   | `absl::FormatSpec<Args...>` templates, checked at compile time under clang; the two `Printf` calls handing a run-time string with no arguments are `Print`.                                                                                                            |
| `sprintf`, `snprintf`, `printf`, `fprintf`, the `OutputDebugString` macro                                            | 602   | `absl::SNPrintF` with the buffer's real size (`sizeof` on the array, the size parameter, the EditClass buffer), `absl::PrintF`, `absl::FPrintF`; five heap `new char[]` message buffers in the WOL code are `absl::StrFormat` strings. Fanned out to five fork agents. |
| `Buffer_Frame_To_Page(..., int flags, ...)`                                                                          | 8+5   | A `ShapeEffects` struct (ghost table, fading table and count, predator offset, partial predator) replaces the flag-driven `va_arg` reads; each `CC_Draw_Shape` makes one call instead of choosing among four argument lists.                                           |
| `va_list` declarations, the printer definitions                                                                      | 17+19 | Gone with the templates.                                                                                                                                                                                                                                               |
| `fcntl(fd, F_SETFL, flags)`                                                                                          | 1     | `NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)`: POSIX declares it variadic.                                                                                                                                                                                       |

The compile-time check found eight broken formats that `vsnprintf` had been reading off the stack:
`"%03"` with no conversion for the harvester's gem and gold counts on the mono page, `"%04X"` and
`"%08X"` printing a `FootClass*` in both games' cargo and RA's techno debug pages (now `%p`), TD's
shape-buffer overflow message printing the frame pointer where it meant the frame length, and the
WOL patch path and file, `unsigned char` arrays, printed with `%s`. The strict build then reported
one analyzer finding the reference parameters exposed: TD's `Make_Enemy` formatted `enemy->Name`
after testing `enemy` for null; the message is now guarded.

The unused `Dialog_Message`, `Mono_Printf(int)` and `Debug_Printf` declarations and the
`#ifdef NEVER` printers in TD's `monoc.cc` are gone. Two follow-ups were left in place:
`Format_Runtime_Text` still writes into a caller's `char` buffer, and `DisconnectPingResultsString`
and `Set_Scenario_Name` still take an unsized `char*` (their sizes are local constants now).

Verification: the isolated sweep over all 932 translation units at zero (the one `error` line is the
`base/numeric.h` header check, whose unit has no Abseil include path and fails before any check
runs); a probe confirming the enabled configuration reports `printf("%d", 1)`; the full strict
build; CTest (463 tests); and the RA and TD save/load smoke tests including the TD team, building,
mobile, map and globals fixtures. The dialogs, map editors, mono debug pages and WOL code are not on
the smoke path; those remain a manual check. The excluded-name count drops from 110 to 107.

### Remaining P1 enablement (2026-09-15)

`bugprone-unhandled-code-paths`, `abseil-unchecked-statusor-access` and
`clang-analyzer-optin.core.EnumCastOutOfRange` are now enforced; the plan is
[P1_REMAINING_PLAN.md](P1_REMAINING_PLAN.md). Each had been skipped on 2026-09-11 for a reason that
no longer held on re-measurement.

- **Unhandled code paths** reported nothing: the switch-fallback work of 2026-09-12 gave every
  switch a `default`, which is all the check asks for in its default mode. The exclusion is simply
  gone.
- **StatusOr** still crashes LLVM 23.1.2's dataflow model (`getSyntheticFields` under
  `runTypeErasedDataflowAnalysis`), but only on the two PCX readers, which were also the only
  `absl::StatusOr` users left. Their `ReadByte` returned a `StatusOr<uint8_t>` whose error string
  nobody read; it returns `std::optional<uint8_t>` now, both games drop the `absl::status` link, and
  the check guards whatever uses `StatusOr` next.
- **Enum cast range** had 27 locations and 274 messages. 246 came from the generic `|`, `&` and `~`
  templates in each game's `jshell.h` being instantiated for flag enums. The checker accepts any
  cast to an enum marked `[[clang::flag_enum]]` (probed: zero, a combination, a mask and `~` all
  pass), and GCC rejects the bare attribute, so `base/attributes.h` wraps it as `CNC_FLAG_ENUM`;
  `TextPrintType`, `ThreatType`, the gadget `FlagEnum`, RA's `AttachType`, `GBC_Enum`,
  `ShapeFlags_Type`, the WSA open flags and the modem status bits carry it. `KeyNumType` is a key
  code with modifier bits above it, which `-Wflag-enum` rightly refuses, so `keyboard.h` gives it
  its own three operators and `ButtonKey` its own `NOLINT`, each explaining the representation.
  `DirType` is a 256-step direction of which only the compass points are named; `AsDirection(int)`
  beside each definition wraps an angle to the circle and holds the one `NOLINT`, and the eleven
  computed casts (`Desired_Facing*`, the nuclear launch angles, the aircraft search, the debug
  random directions, the harvester unloading adjustment) plus both games' `DirType` arithmetic
  operators use it; the ~800 constant `static_cast<DirType>` in the data tables are initializers the
  analyzer never runs. The path optimizer's `-2` marker was already `kEmptyCommand`; its table now
  says so.

No defect came out of it. Verification: the isolated sweep of the three checks over all 932
translation units reports nothing (apart from the `base/numeric.h` header check, which has no Abseil
include path and fails before any check runs); the full strict build; CTest (463 tests); and the RA
and TD save/load smoke tests with every fixture. The excluded-name count drops from 107 to 104.

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
excluded, no call site in either game discards a `[[nodiscard]]` result. The member function const
review below added two more: the `int` overload of `WWMessageBox::Process` and
`CCMessageBox::Process`, whose result 142 calls ignore because an informational box has nothing to
answer.

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

### Reserved identifier review (2026-09-12)

`bugprone-reserved-identifier`, its CERT aliases `cert-dcl37-c` and `cert-dcl51-cpp`,
`clang-diagnostic-reserved-identifier` and `clang-diagnostic-reserved-macro-identifier` are now
enforced together. The sweep reported 188 declarations and 34 macros. Most were Westwood's habit of
prefixing file-local helpers and struct tags with an underscore (`_Consists_Only_Of_Infantry`,
`typedef struct _VQAConfig {...} VQAConfig`). None of them collided with the implementation, but
nothing stopped the next one from doing so.

A token-level rename covered 133 names in 73 files, leaving comments and strings alone, and it
refused any new name already spelled in the file. Most names just lose the underscore; a struct tag
taking its typedef's name is legal C++. A few needed a name of their own:

- TD's `_Kbd` was the pointer the `sdllib/keyboard.h` helpers call through, next to the `Kbd` object
  it points at. It is `ActiveKeyboard`.
- RA's gadget colour scheme `_GreyScheme` would have collided with the `GreyScheme` global in
  `externs.h`. It is `DefaultColorScheme`.
- The base64 tables became `kEncoder`, `kDecoder` and `kPad`, and the `_wsproto.h` guard became
  `CNC_RED_ALERT_RA_WSPROTO_IMPL_H_`.
- `sdllib`'s `_ShapeBuffer` and `_ShapeBufferSize` lost the underscore, which retires RA's
  `#define ShapeBufferSize _ShapeBufferSize` alias in `compat.h`.

The macros:

- `port/ex_string.h` defined `_MAX_PATH`, `_MAX_FNAME`, `_MAX_EXT` and `_MAX_DRIVE`, the Microsoft
  CRT spellings. They are now `kMaxPath`, `kMaxFname`, `kMaxExt` and `kMaxDrive`, and their 105 uses
  in 25 files follow. Plain `MAX_PATH` stays with Windows. This was also the preprocessor warning
  that `CLAUDE.md` measured as keeping `clang-tidy-cache` from hashing most translation units; the
  cache rate has not been re-measured since.
- `ra/search.h` defined `_USERENTRY`, an empty Borland calling-convention macro, and used it twice.
  It is gone.
- TD's `display.cc` and `sidebar.cc` each defined `_RETRIEVE` so that `Init_Theater` loads the
  palette fading and translucency tables rather than building them and writing them to disk. The
  build-and-write branches (thirteen blocks in `display.cc`, one in `sidebar.cc`) were always
  compiled out. They are deleted and the loads are unconditional. Renaming the macro alone would
  have silently switched them back on.

The Westwood Online headers under `ra/wolapi/` are MIDL output, kept faithful to the IDL, and full
of generated reserved names (`__IChat_FWD_DEFINED__`, `__RPC_FAR`, struct tags). Each is wrapped in
a `NOLINTBEGIN`/`NOLINTEND` for these five checks. The matching Windows SDK spellings in
`port/win32/win32_com.h` (`__RPC_FAR`, `__RPC_USER`, `__RPC_STUB` and the `__IID_DEFINED__` guard
that MIDL's IID file tests) carry line suppressions: the generated code needs exactly those names.

The full strict build of both games is clean and all 242 tests pass.

### Remaining P3 policy decisions (2026-09-12)

These P3 names stay excluded. Each was measured in an isolated sweep of project sources, and either
its fix contradicts a project rule or its automated fix is wrong often enough to need a person on
every site.

| Check                                                        | Reports | Why it stays excluded                                                                                                                                                                                                   |
| ------------------------------------------------------------ | ------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `modernize-avoid-c-style-cast`, `google-readability-casting` | 1,393   | `docs/TYPE_MIGRATION.md` makes cast replacement opportunistic, inside code already being migrated, and forbids a codebase-wide cast hunt.                                                                               |
| `clang-diagnostic-old-style-cast`                            | 0       | The clang flag set passes `-Wno-old-style-cast`, so enabling the name enforces nothing; the GCC strict set already has `-Wold-style-cast`.                                                                              |
| `cppcoreguidelines-macro-usage`                              | 2,134   | 2,094 are constants and 40 function-like macros. 1,310 are the text-string IDs in each game's `conquer.h`, and the next largest group is the `sdllib/keyboard.h` key codes.                                             |
| `modernize-macro-to-enum`, `cppcoreguidelines-macro-to-enum` | 2,256   | The same macro groups. As enumerators they would change type wherever they meet integer arithmetic and `printf`-style formatting.                                                                                       |
| `cppcoreguidelines-use-enum-class`                           | 517     | 178 are unnamed enums used as integer constants. 46 are the per-dialog `RedrawType` levels, compared as ordered values 110 times. 137 are the core type enums in each game's `defines.h`, which index arrays and loops. |
| `portability-template-virtual-member-function`               | 35      | See below.                                                                                                                                                                                                              |

`portability-template-virtual-member-function` reports 35 virtual members of the heap, vector and
mix-file class templates. Clang and GCC only instantiate a virtual member that some specialization
uses, while some compilers instantiate all of them, so an unused member that does not compile for a
given `T` would fail only there. Silencing the check means explicitly instantiating every
specialization, or making those members non-virtual. That is a design change to the heap and vector
interfaces, which both games use for every object type, and neither the clang nor the GCC build has
a member that fails to instantiate.

### Member initializer review (2026-09-12)

`cppcoreguidelines-prefer-member-initializer`, `modernize-use-default-member-init` and its alias
`cppcoreguidelines-use-default-member-init` are now enforced. The savegame migration made this
possible: no raw-image loads or `NoInitClass` constructors remain, so a default member initializer
can no longer overwrite loaded state. The sweep reported 806 constructor-body assignments that
belonged in the initializer list and 654 constant initializers that belonged on the member
declaration.

The fix-its did most of the work, in two separate passes: initializer list first, then member
declarations. Run together, the two passes edit the same initializer lists and corrupt them. Even
run separately, each needed repair:

- `prefer-member-initializer` copied a legacy duplicate `Contrast = 0x80;` from TD's `OptionsClass`
  constructor into the initializer list twice. The duplicate is gone. The pass also moved the
  unconditional half of a `GERMAN`/`FRENCH` conditional, which is harmless: the localized branch
  still assigns `true` in the body afterwards.
- `use-default-member-init` removes an initializer but keeps its separating comma. Across 200 edited
  hunks this left lists such as `: , , MagicNum(magicnum), , {`. A repair pass collapsed the empty
  slots, touching only lines the fix-its had changed, and the compiler checked the result.
- Moving a default from a `.cc` constructor into a header can bring names the header never saw:
  - `CELL_LEPTON_W` lived in the heavy `ra/display.h`. It now sits in a new
    `ra/display_constants.h`, which `rules.h` includes, mirroring TD's `display_constants.h`.
  - Both games' `help.h` now include `conquer.h` for `TXT_NONE`.
  - RA's `INVALID_SOCKET` moved from two `.cc` files into `wsproto.h`, next to `typedef int SOCKET`.
- Both passes carried an assignment's trailing comment out of the constructor body as a bare line,
  sometimes left sitting under unrelated code. Each note now sits on its member's declaration, or
  was dropped where the declaration already says the same.
- The brace form rejects narrowing. `MaxRetries = -1` meant "retry forever", so its default is now
  `std::numeric_limits<unsigned long>::max()`.
- The strict build then reported initializer lists out of declaration order and constructors left
  empty. Its fix-its reordered them and turned them into `= default`, except for one reorder in RA's
  `list.h` and TD's now-empty `TabClass` constructor, which were done by hand. Initialization order
  was already declaration order, so the textual reordering changes nothing at run time.

Moving `UnitTrackerClass`'s allocation into its initializer list exposed a real bug in both games:
the destructor freed the `new long[]` array with scalar `delete`. It is `delete[]` now.

TD's `CommBufferClass` showed a second one. Once its `MaxPacketSize` assignment became an
initializer, clang reported the field as never read. RA's copy of the class checks
`buflen > MaxPacketSize` before copying a packet into a queue buffer allocated as
`new char[maxlen]`; TD's `Queue_Send` and `Queue_Receive` did not, so an oversized packet, including
one received from the network, overran the heap. Both now reject it as RA does.

In the end, 724 member declarations gained a default initializer and 801 constructor-body
assignments are gone, across 120 headers and 152 source files. The defaults use the check's brace
form, `int Count{0};`.

The full strict build of both games is clean, all 242 tests pass, and the RA and TD save/load smoke
tests match their uninterrupted runs.

### P4 fix-it pass A (2026-09-12)

Six P4 checks with reliable fix-its are enforced, together with their CERT aliases and five compiler
diagnostics that already report nothing:

- `readability-uppercase-literal-suffix` (and `cert-dcl16-c`): the 27 lower-case suffixes (20 `u`, 6
  `f`, 1 `l`) are upper case, so an `l` can no longer pass for a `1`.
- `readability-const-return-type`: `Rect::Intersect` and `Union` return plain `Rect` values, which
  callers can move from. TD's deleted `CCFileClass` copy assignment lost the same `const`.
- `modernize-return-braced-init-list`: returns that repeated the return type now use braces.
- `modernize-use-bool-literals`: integer literals assigned to `bool` are `true` and `false`.
- `readability-named-parameter`: unnamed parameters now carry a commented name, `/*unused*/` where
  the body ignores them, so every declaration reads the same.
- `readability-enum-initial-value` (and `cert-int09-c`): an enum that initializes some enumerators
  now initializes all of them, so no value depends on counting from the last explicit one.

`readability-inconsistent-ifelse-braces`, `clang-diagnostic-ignored-qualifiers`,
`clang-diagnostic-padded-bitfield`, `clang-diagnostic-date-time` and `clang-diagnostic-pedantic`
reported nothing and are enabled to keep it that way.

`modernize-use-bool-literals` exposed a latent bug. TD's `CCMessageBox::Process` returns the pressed
button's index but kept it in a `bool`, so a third button read back as `1`, the same as the second.
The fix-it turned `retval = 2` into `true`, and `bugprone-branch-clone` then flagged two identical
branches. `retval` is an `int` now, as in RA; no TD dialog uses a third button today.

One fix-it produced invalid code. `readability-enum-initial-value` wrote
`CD_ALLIED = 1 [[maybe_unused]]` in RA's `conquer.cc`, putting the value before the attribute; it is
`CD_ALLIED [[maybe_unused]] = 1`.

The fix-its touched 282 files.

The full strict build of both games is clean and all 242 tests pass.

### P4 policy decisions (2026-09-12)

These P4 names stay excluded. Each was measured in an isolated sweep of project sources, and in each
case the rewrite would be large, would contradict a project rule, or would fight third-party code.

| Check                                                                                                       | Reports       | Why it stays excluded                                                                                                                                                                                                                                                                                                          |
| ----------------------------------------------------------------------------------------------------------- | ------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `readability-magic-numbers`, `cppcoreguidelines-avoid-magic-numbers`                                        | 17,934        | The largest groups are the bignum arithmetic in `tech/mp.cc` (3,544), dialog and score-screen layout coordinates (`ra/netdlg.cc` alone has 1,991) and unit data tables. Naming every literal would bury the constants that matter.                                                                                             |
| `modernize-use-designated-initializers`                                                                     | 2,431         | Mostly positional rows of data tables: both games' vehicle track tables in `drive.cc` (969), infantry data and audio tables.                                                                                                                                                                                                   |
| `cppcoreguidelines-avoid-non-const-global-variables`                                                        | 1,399         | `CLAUDE.md` lists removing globals among changes to avoid unless requested.                                                                                                                                                                                                                                                    |
| `cppcoreguidelines-non-private-member-variables-in-classes`, `misc-non-private-member-variables-in-classes` | 1,219 / 2,394 | Encapsulating the legacy classes' public state is a class-hierarchy refactor, which `CLAUDE.md` also defers.                                                                                                                                                                                                                   |
| `clang-diagnostic-global-constructors`                                                                      | 1,143         | 716 are the static type-class tables in the `*data.cc` files; startup cost is not a measured problem.                                                                                                                                                                                                                          |
| `clang-diagnostic-exit-time-destructors`                                                                    | 838           | 706 are the same static tables.                                                                                                                                                                                                                                                                                                |
| `readability-trailing-comma`                                                                                | 1,027         | Tried: a trailing comma makes clang-format put every element on its own line. Its fix-its added 3,411 one-element lines, 3,239 of them compact initializer-list entries (1,425 in `ra/netdlg.cc`) into single lines, against 172 enum values. The check's policies are per single- or multi-line list, with no enum-only mode. |
| `readability-function-cognitive-complexity`                                                                 | 607           | Functions over the default threshold of 25. Complexity alone is not a defect, and splitting them safely needs behavior coverage first.                                                                                                                                                                                         |
| `performance-enum-size`                                                                                     | 502           | Narrowing enum storage changes class layouts and integer promotion at every arithmetic use; worth doing only per enum, with a measured saving.                                                                                                                                                                                 |
| `clang-diagnostic-nullability-extension`                                                                    | 278           | Every report is Abseil's `absl_nonnull`/`absl_nullable` expanding to `_Nonnull`/`_Nullable` inside `CHECK` macros.                                                                                                                                                                                                             |
| `cppcoreguidelines-avoid-do-while`                                                                          | 100           | `do`-`while` is valid and often the clearest form of a loop that must run once.                                                                                                                                                                                                                                                |
| `misc-no-recursion`                                                                                         | 47            | Mostly mutually recursive virtual calls through the object model (`Assign_Destination`, `Transmit_Message`, `Do_Action`), bounded by game state rather than input.                                                                                                                                                             |
| `clang-diagnostic-weak-vtables`                                                                             | 33            | Anchoring each vtable only trims duplicated build output.                                                                                                                                                                                                                                                                      |
| `clang-diagnostic-nested-anon-types`                                                                        | 20            | Anonymous structs in anonymous unions, which GCC, Clang and MSVC all accept; naming them would rename every member access.                                                                                                                                                                                                     |
| `cppcoreguidelines-avoid-const-or-ref-data-members`                                                         | 13            | The serializer and archive helpers' reference members and the queues' `const` capacity. None is ever reassigned, by design.                                                                                                                                                                                                    |
| `misc-multiple-inheritance`                                                                                 | 11            | The aircraft, anim, bullet, techno and terrain classes in both games and `GraphicBufferClass`; changing the hierarchy is a refactor `CLAUDE.md` defers.                                                                                                                                                                        |

### P4 fix-it pass B and hand fixes (2026-09-12)

The rest of the P4 checks are enforced. Run together, their fix-its conflicted in shared headers, so
each ran alone on top of the last good tree, behind a GCC build that rolled back any step that did
not compile.

Fix-it checks:

- `modernize-use-auto`, `readability-isolate-declaration` and `readability-braces-around-statements`
  applied cleanly.
- `readability-redundant-nested-if` joined `if (a) { if (b) {...} }` into `if ((a) && (b))`. Where a
  comment sat between the two conditions, the fix-it left it stranded between the joined condition
  and its brace; those comments moved above the `if`. `&&` short-circuits, so `b` still runs only
  when `a` holds.
- `readability-convert-member-functions-to-static` made members that never use `this` `static`.
  Where a declaration was `[[nodiscard]]`, the fix-it wrote `static [[nodiscard]]`, which is
  ill-formed; a repair pass put the attribute first. `readability-static-accessed-through-instance`
  then flagged the callers that still reached those members through an object, and its fix-its call
  them through the class.
- The joined conditions kept their per-operand parentheses, so `readability-redundant-parentheses`
  removed them, and `readability-else-after-return` straightened the branches the switch conversion
  left behind. Once `CommBufferClass::Mono_Debug_Print` was `static`, RA's
  `IPXManagerClass::Mono_Debug_Print` called it the same way on both of its branches;
  `bugprone-branch-clone` flagged the duplicate, and the two conditions were joined by hand.
  `SidebarClass::Reload_Sidebar` is `static` too, and the save loader calls it through the class. A
  modem-init error check became one `return succeeded || !cancelled`, so the message box still
  appears only after a failure, and the `break` after a `Fatal` call is gone.

Hand fixes:

- `readability-trivial-switch`: a script converted 24 switches with one `case` plus `default`, or a
  `default` only, into `if`/`else` or plain blocks. It accepted only the provably safe shape (no
  fallthrough into `default`, no `break` inside a branch other than the trailing one) and moved the
  comments between `switch (...) {` and the first label above the new `if`, such as the explanation
  of `RADIO_REDRAW` in both games' `ObjectClass::Receive_Message`. Controlling expressions with side
  effects, such as `Random_Pick(0, 3)`, are still evaluated once.
- `clang-diagnostic-nrvo`: `Rect::Intersect` returned a dummy local on each early exit and the
  working rectangle at the end, and `Union` returned `result` beside its parameters, so neither copy
  could be elided. `Intersect` now clips all four edges before one validity test, since each clip
  only shrinks the rectangle, and writes the `x`/`y` out-parameters only on success, as before;
  `Union` returns an invalid parameter before declaring `result`. A harness compared old and new
  over 390,625 rectangle pairs, out-parameters included, with no mismatch.
- `readability-avoid-nested-conditional-operator`: 182 of the reports were one shape in
  `ra/wolstrng.cc`, `config::kIsGerman ? de : config::kIsFrench ? fr : en`. A
  `Localized(german, french, english)` helper replaces them with the literals unchanged; it is
  `noexcept`, because `bugprone-throwing-static-initialization` otherwise treats each of those 182
  static constants as an initialization that may throw. The other build-language picks (the patch
  SKU, `kLanguageText`, the mission-name table, the window title, score and dialog positions, the
  version suffix) are immediately invoked lambdas, `noexcept` where they initialize a static.
- `performance-no-int-to-ptr`: the shape-buffer allocators in both keyframe loaders rounded pointers
  through `uintptr_t` and back. They now advance the pointer itself (`std::bit_cast` reads its low
  bits), so provenance survives. TD's mono page code adds offsets to `MonoSegment` as a `char*`, and
  `Store_Cell` copies with `memcpy`. RA's trigger loader keeps two casts, where a trigger
  reference's ID briefly holds a `new[]`'d name string, under reasoned suppressions.
- `clang-diagnostic-missing-noreturn`: eleven always-exiting functions are `[[noreturn]]` on their
  first declaration: both games' `Fatal`, `MapEditClass::Fatal` and `Memory_Error_Handler`,
  `Print_Error_End_Exit` and `Print_Error_Exit`, and TD's `Validate_Error`. The `return 0;` after
  each of TD's 16 `Validate_Error` calls became unreachable and is gone.
- `clang-diagnostic-ms-bitfield-padding`: RA's `RulesClass` mixed `bool` and `unsigned` flag
  bit-fields, which the Microsoft ABI stores in separate units; all 20 `unsigned` flags are `bool`
  now. `RulesClass` has no `Serialize()` and is re-read from the rules INI on load, so the save
  format is unchanged.
- `misc-confusable-identifiers` (key names such as `KA_I` that spell their key),
  `readability-function-size` (RA's `Com_Scenario_Dialog` and TD's `Select_Game`) and
  `clang-diagnostic-c99-extensions` (the shape block's flexible `Offsets[]`) carry reasoned line
  suppressions. `clang-diagnostic-documentation-unknown-command` backslashes are plain text, and
  `readability-avoid-unconditional-preprocessor-if` resolved TD's `#if (true)` and two `#if (false)`
  blocks.
- The lifetime-safety suggestions (`clang-diagnostic-lifetime-safety-*`) mark parameters and
  implicit objects that a returned reference or pointer can outlive. The fix-its wrote
  `[[clang::lifetimebound]]` in some translation units and `ABSL_ATTRIBUTE_LIFETIME_BOUND` in
  others, so `clang-apply-replacements` refused the conflicting header edits until every replacement
  was normalized to the Abseil macro, which also expands to nothing on compilers without the
  attribute. Each annotated file includes `absl/base/attributes.h`, and four test targets that could
  not see Abseil link `absl::core_headers`. On an array parameter (`const unsigned char pal[]`) the
  attribute bound to the array type, so both games' `ScoreScaleClass` take
  `const unsigned char* pal`. Annotating one function exposes its callers, so the export and apply
  repeated until no suggestion remained. Clang could not verify five of the resulting annotations
  (the `std::visit` lambdas in RA's `TypeClass`, the `VectorClass` constructors' `array` and
  `BufferPipe`'s `buffer`) yet still suggests them, so those five sites carry a suppression saying
  so. The constructor annotation on `TextLabelClass` let clang follow
  `MessageListClass::Add_Message` handing a label its local message copy before re-pointing it at a
  message buffer. The pointer never escaped, but the label is now built on the buffer directly. In
  all, 257 annotations were added.

`modernize-use-using` stays excluded. Its fix-its rewrite `typedef struct {...} Name;` as
`using Name = struct {...};`. For an unnamed struct, enum or union that is not equivalent: the
typedef gives the unnamed type a name for linkage, and the alias does not, so GCC rejected functions
declared with those types as "used but never defined". The fix-its also wrote 81 anonymous tag
bodies twice (`using X = enum {...} {...};`), even when run one translation unit at a time; with
those repaired, `face.h` and `SessionClass` were still corrupted.

The full strict build of both games is clean, all 242 tests pass, and the RA and TD save/load smoke
tests (RA's default scenario; TD's mobile, building, map and globals fixtures) match their
uninterrupted runs.

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

Compiler diagnostic filters also depend on warning flags. In particular, `unsafe-buffer-usage`,
`padded`, and `covered-switch-default` are suppressed in [CMakeLists.txt](../CMakeLists.txt);
removing their tidy exclusions alone does not enable them. An isolated diagnostic sweep needs its
warning flag and at least one real clang-tidy check.

Preserve the [type-migration policy](TYPE_MIGRATION.md), deterministic simulation RNG, and
packet/recording layouts when applying broad rules. Field-wise savegames do not make all layout
changes safe.
