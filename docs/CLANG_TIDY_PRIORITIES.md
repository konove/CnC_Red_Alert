# Clang-tidy priorities

Updated: 2026-09-11, against [`.clang-tidy`](../.clang-tidy) and clang-tidy 23.1.2.

This tracks **all 226 currently excluded check names** and completed entries, in recommended work
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

| Check                                                         | Status  | Reason / result                                                                                                                                                                                                                        |
| ------------------------------------------------------------- | ------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `bugprone-suspicious-stringview-data-usage`                   | Enabled | Commit `Enable string-view data usage checking`: copy the RA title-screen filename to a terminated string before calling the PCX reader.                                                                                               |
| `abseil-unchecked-statusor-access`                            | Skipped | Commit `Handle failed PCX byte reads`: guard both games' byte reads, but retain the exclusion because LLVM 23.1.2 crashes even on checked access with Abseil 20260107.0. Revisit after a toolchain fix; see reproduction below.        |
| `clang-analyzer-unix.cstring.UninitializedRead`               | Enabled | Commit `Enable uninitialized C-string read checking`: initialize the public-key generation self-test buffer while preserving the random-fill loop.                                                                                     |
| `clang-analyzer-cplusplus.InnerPointer`                       | Enabled | Commit `Enable string inner-pointer checking`: detect string-buffer pointers used after invalidation.                                                                                                                                  |
| `bugprone-copy-constructor-init`                              | Enabled | Commit `Enable copy-constructor base initialization checking`: both games and shared code already initialize copied base state correctly; no source fixes needed.                                                                      |
| `bugprone-unchecked-string-to-number-conversion`              | Enabled | Commit `Enable checked string-to-number conversions`: replace unchecked decimal/hex conversions with range-checked parsing and explicit defaults; preserve legacy INI, coordinate, and protocol formats.                               |
| `cert-err34-c`                                                | Enabled | Alias enabled with `bugprone-unchecked-string-to-number-conversion` in commit `Enable checked string-to-number conversions`.                                                                                                           |
| `clang-analyzer-unix.StdCLibraryFunctions`                    | Enabled | Commit `Fix TCP socket option size and enable C library checking`: pass an integer TCP_NODELAY flag with its actual size, avoiding a read past a one-byte bool.                                                                        |
| `clang-diagnostic-cast-align`                                 | Enabled | Commit `Fix buffer alignment and enable cast alignment checking`: copy unaligned packet/media values, check typed buffer access, align cached shape headers, and bound legacy byte fills.                                              |
| `clang-diagnostic-uninitialized-const-pointer`                | Enabled | Commit `Enable uninitialized const-pointer argument checking`: both games and shared code pass without source fixes.                                                                                                                   |
| `clang-diagnostic-reorder-ctor`                               | Enabled | Commit `Match constructor initialization order to declarations`: reorder 16 initializer lists while preserving expressions, member layouts, and actual initialization order.                                                           |
| `bugprone-unhandled-code-paths`                               | Skipped | Commit `Document missing-default check policy`: default mode flags switches with valid post-switch fallbacks and bounded inputs; retain exclusion rather than require redundant defaults. See review below.                            |
| `bugprone-non-zero-enum-to-bool-conversion`                   | Enabled | Commit `Enable nonzero enum-to-bool conversion checking`: both games and shared code pass without source fixes.                                                                                                                        |
| `clang-analyzer-optin.core.EnumCastOutOfRange`                | Skipped | Commit `Document enum cast range check policy`: LLVM 23.1.2 rejects intentional intermediate directions, flag combinations, and path-command sentinels; retain exclusion. See review below.                                            |
| `clang-diagnostic-tautological-constant-out-of-range-compare` | Enabled | Commit `Preserve shutdown states and enable constant range comparison checking`: store RA shutdown states 0 through 3 in an integer instead of collapsing them to bool.                                                                |
| `clang-diagnostic-tautological-unsigned-enum-zero-compare`    | Enabled | Commit `Simplify unsigned enum bounds and enable zero comparison checking`: use an unsigned event range check and remove an impossible negative template-ID check while preserving the no-template sentinel.                           |
| `clang-diagnostic-tautological-unsigned-zero-compare`         | Enabled | Commit `Remove impossible icon checks and enable unsigned zero comparison checking`: drop the always-false negative test on the unsigned template icon index in both map validators while keeping the upper bound and icon-map checks. |
| `clang-diagnostic-implicit-int-conversion`                    | Pending | Find remaining implicit loss of integer range or precision.                                                                                                                                                                            |
| `clang-diagnostic-implicit-int-conversion-on-negation`        | Pending | Review negation that changes range during conversion.                                                                                                                                                                                  |
| `clang-diagnostic-int-to-pointer-cast`                        | Pending | Find truncated or invalid addresses in legacy casts.                                                                                                                                                                                   |
| `bugprone-derived-method-shadowing-base-method`               | Pending | Find unintended hiding in the game class hierarchies.                                                                                                                                                                                  |

## P2 — Further correctness and targeted safety

| Check                                                      | Status  | Reason / result                                                                                         |
| ---------------------------------------------------------- | ------- | ------------------------------------------------------------------------------------------------------- |
| `clang-diagnostic-lifetime-safety-use-after-free`          | Pending | Reassess dangling-pointer findings; verify fixed-heap false positives.                                  |
| `clang-diagnostic-lifetime-safety-invalidation`            | Pending | Review container/storage invalidation against custom lifetimes.                                         |
| `clang-diagnostic-lifetime-safety-use-after-scope-moved`   | Pending | Review escaping locals and ownership-transfer false positives.                                          |
| `bugprone-parent-virtual-call`                             | Pending | Check skipped overrides; retain intentional grandparent dispatch.                                       |
| `cppcoreguidelines-interfaces-global-init`                 | Pending | Find cross-unit global initialization dependencies.                                                     |
| `bugprone-throwing-static-initialization`                  | Pending | Prevent failures before normal startup error handling.                                                  |
| `cert-err58-cpp`                                           | Pending | Alias of `bugprone-throwing-static-initialization`; handle together.                                    |
| `cppcoreguidelines-init-variables`                         | Pending | Review local initialization; avoid masking missing assignments with zeroes.                             |
| `cppcoreguidelines-special-member-functions`               | Pending | Audit copy/move/destruction consistency for owning types.                                               |
| `hicpp-special-member-functions`                           | Legacy  | Unavailable in LLVM 23; review with `cppcoreguidelines-special-member-functions` on older tools.        |
| `clang-diagnostic-deprecated-copy-with-user-provided-copy` | Pending | Review implicit copy operations paired with custom copying.                                             |
| `clang-diagnostic-deprecated-copy-with-user-provided-dtor` | Pending | Review implicit copying of types with custom destruction.                                               |
| `clang-diagnostic-deprecated-copy-with-dtor`               | Pending | Complete the destructor/copy audit, including defaulted destructors.                                    |
| `bugprone-macro-parentheses`                               | Pending | Prevent macro expansion from changing expression meaning.                                               |
| `clang-diagnostic-logical-op-parentheses`                  | Pending | Review ambiguous conditions for precedence mistakes.                                                    |
| `clang-diagnostic-bitwise-op-parentheses`                  | Pending | Review mixed bitwise expressions for precedence mistakes.                                               |
| `clang-diagnostic-shift-op-parentheses`                    | Pending | Review ambiguous shifts, especially packed values.                                                      |
| `readability-math-missing-parentheses`                     | Pending | Expose arithmetic grouping that is easy to misread.                                                     |
| `bugprone-branch-clone`                                    | Pending | Review duplicate branches for copy/paste bugs; preserve intentional symmetry.                           |
| `clang-diagnostic-sign-conversion`                         | Pending | Review signed sentinels and range changes; follow the type policy.                                      |
| `bugprone-signed-bitwise`                                  | Pending | Review signed shifts and masks without breaking deliberate bit patterns.                                |
| `hicpp-signed-bitwise`                                     | Legacy  | Unavailable in LLVM 23; review with `bugprone-signed-bitwise` on older tools.                           |
| `clang-diagnostic-switch-enum`                             | Pending | Audit missing enum cases even when a default exists.                                                    |
| `clang-diagnostic-switch`                                  | Pending | Review missing cases; intentional ButtonKey(n) labels need a policy.                                    |
| `clang-diagnostic-switch-bool`                             | Pending | Find accidental boolean switch expressions.                                                             |
| `clang-diagnostic-duplicate-enum`                          | Pending | Separate accidental duplicate values from deliberate aliases.                                           |
| `clang-diagnostic-missing-braces`                          | Pending | Check aggregate/subobject initialization before adding braces.                                          |
| `clang-diagnostic-cast-qual`                               | Pending | Review casts discarding const or volatile guarantees.                                                   |
| `misc-explicit-constructor`                                | Pending | Prevent unintended implicit construction and conversions.                                               |
| `cppcoreguidelines-explicit-constructor`                   | Pending | Alias of `misc-explicit-constructor`; handle together.                                                  |
| `google-explicit-constructor`                              | Pending | Alias of `misc-explicit-constructor`; handle together.                                                  |
| `hicpp-explicit-conversions`                               | Legacy  | Unavailable in LLVM 23; review with `misc-explicit-constructor` on older tools.                         |
| `cppcoreguidelines-pro-type-vararg`                        | Pending | Audit untyped call boundaries and argument agreement.                                                   |
| `hicpp-vararg`                                             | Legacy  | Unavailable in LLVM 23; review with `cppcoreguidelines-pro-type-vararg` on older tools.                 |
| `modernize-avoid-variadic-functions`                       | Pending | Replace unsafe variadic interfaces where practical.                                                     |
| `cert-dcl50-cpp`                                           | Pending | Alias of `modernize-avoid-variadic-functions`; handle together.                                         |
| `clang-diagnostic-missing-format-attribute`                | Pending | Extend compiler format validation to project wrappers.                                                  |
| `clang-diagnostic-undef`                                   | Pending | Catch misspelled or missing feature macros in conditional builds.                                       |
| `clang-diagnostic-undefined-func-template`                 | Pending | Catch unavailable template definitions on instantiated paths.                                           |
| `clang-diagnostic-undefined-var-template`                  | Pending | Catch template variables lacking required definitions.                                                  |
| `clang-diagnostic-shadow-field`                            | Pending | Find locals or parameters accidentally hiding object state.                                             |
| `clang-diagnostic-shadow`                                  | Pending | Find scope mistakes; expect more noise than field shadowing.                                            |
| `concurrency-mt-unsafe`                                    | Pending | Audit audio/timer callbacks and shared library state.                                                   |
| `clang-analyzer-optin.core.FixedAddressDereference`        | Pending | Review hard-coded addresses for invalid legacy assumptions.                                             |
| `clang-analyzer-core.FixedAddressDereference`              | Legacy  | Unavailable in LLVM 23; review with `clang-analyzer-optin.core.FixedAddressDereference` on older tools. |
| `bugprone-easily-swappable-parameters`                     | Pending | Improve error-prone APIs when names alone cannot prevent swaps.                                         |
| `bugprone-random-generator-seed`                           | Pending | Review seed mistakes while preserving deterministic simulation RNG.                                     |
| `cert-msc32-c`                                             | Pending | Alias of `bugprone-random-generator-seed`; handle together.                                             |
| `cert-msc51-cpp`                                           | Pending | Alias of `bugprone-random-generator-seed`; handle together.                                             |
| `modernize-use-integer-sign-comparison`                    | Pending | Use safe mixed-sign comparisons where still needed.                                                     |
| `modernize-use-nodiscard`                                  | Pending | Make important results harder to discard accidentally.                                                  |

## P3 — Broader safety and maintainability

| Check                                                           | Status  | Reason / result                                                                                           |
| --------------------------------------------------------------- | ------- | --------------------------------------------------------------------------------------------------------- |
| `clang-diagnostic-unsafe-buffer-usage`                          | Pending | Map remaining buffer hazards; requires staged API/container work.                                         |
| `cppcoreguidelines-pro-bounds-avoid-unchecked-container-access` | Pending | Review unchecked indexing; choose bounds policy at real boundaries.                                       |
| `cppcoreguidelines-pro-bounds-constant-array-index`             | Pending | Replace unverifiable C-array indexing where practical.                                                    |
| `cppcoreguidelines-owning-memory`                               | Pending | Clarify ownership while preserving the custom heap model.                                                 |
| `cppcoreguidelines-no-malloc`                                   | Pending | Move suitable allocations to typed lifetime management.                                                   |
| `hicpp-no-malloc`                                               | Legacy  | Unavailable in LLVM 23; review with `cppcoreguidelines-no-malloc` on older tools.                         |
| `cppcoreguidelines-pro-type-union-access`                       | Pending | Audit active members; coordinate/event unions need deliberate treatment.                                  |
| `cppcoreguidelines-pro-bounds-array-to-pointer-decay`           | Pending | Preserve size information across buffer interfaces.                                                       |
| `hicpp-no-array-decay`                                          | Legacy  | Unavailable in LLVM 23; review with `cppcoreguidelines-pro-bounds-array-to-pointer-decay` on older tools. |
| `cppcoreguidelines-pro-bounds-pointer-arithmetic`               | Pending | Reduce unchecked pointer traversal after buffer APIs improve.                                             |
| `modernize-avoid-c-arrays`                                      | Pending | Migrate arrays selectively after ownership and layout review.                                             |
| `cppcoreguidelines-avoid-c-arrays`                              | Pending | Alias of `modernize-avoid-c-arrays`; handle together.                                                     |
| `hicpp-avoid-c-arrays`                                          | Legacy  | Unavailable in LLVM 23; review with `modernize-avoid-c-arrays` on older tools.                            |
| `cppcoreguidelines-use-enum-class`                              | Pending | Strengthen enum boundaries; account for flags and serialized values.                                      |
| `modernize-avoid-c-style-cast`                                  | Pending | Make conversion intent visible across remaining casts.                                                    |
| `google-readability-casting`                                    | Pending | Alias of `modernize-avoid-c-style-cast`; handle together.                                                 |
| `cppcoreguidelines-pro-type-cstyle-cast`                        | Pending | Review C-style casts that bypass type safety.                                                             |
| `clang-diagnostic-old-style-cast`                               | Pending | Finish compiler enforcement after the cast migration.                                                     |
| `clang-diagnostic-deprecated-enum-enum-conversion`              | Pending | Separate arithmetic on unrelated enums from intentional flag use.                                         |
| `clang-diagnostic-deprecated-anon-enum-enum-conversion`         | Pending | Replace anonymous-enum arithmetic with deliberate types/constants.                                        |
| `clang-diagnostic-deprecated-enum-compare`                      | Pending | Review comparisons between unrelated enum domains.                                                        |
| `google-runtime-int`                                            | Pending | Bring remaining integer spellings into the project's fixed-width policy.                                  |
| `modernize-use-default-member-init`                             | Pending | Centralize common defaults and reduce constructor drift.                                                  |
| `cppcoreguidelines-use-default-member-init`                     | Pending | Alias of `modernize-use-default-member-init`; handle together.                                            |
| `cppcoreguidelines-prefer-member-initializer`                   | Pending | Initialize members directly; preserve construction-order semantics.                                       |
| `modernize-use-equals-delete`                                   | Pending | Express prohibited operations explicitly.                                                                 |
| `hicpp-use-equals-delete`                                       | Legacy  | Unavailable in LLVM 23; review with `modernize-use-equals-delete` on older tools.                         |
| `modernize-use-equals-default`                                  | Pending | Let the compiler implement genuinely default operations.                                                  |
| `hicpp-use-equals-default`                                      | Legacy  | Unavailable in LLVM 23; review with `modernize-use-equals-default` on older tools.                        |
| `performance-noexcept-swap`                                     | Pending | Make non-throwing swap guarantees explicit where valid.                                                   |
| `cppcoreguidelines-noexcept-swap`                               | Pending | Alias of `performance-noexcept-swap`; handle together.                                                    |
| `bugprone-switch-missing-default-case`                          | Pending | Review fallback policy; empty defaults alone add little value.                                            |
| `hicpp-multiway-paths-covered`                                  | Legacy  | Unavailable in LLVM 23; review with `bugprone-switch-missing-default-case` on older tools.                |
| `clang-diagnostic-switch-default`                               | Pending | Align compiler fallback enforcement with the switch policy.                                               |
| `clang-diagnostic-covered-switch-default`                       | Pending | Resolve tension between exhaustive switches and defensive defaults.                                       |
| `readability-implicit-bool-conversion`                          | Pending | Clarify boolean intent at numeric and pointer boundaries.                                                 |
| `readability-inconsistent-declaration-parameter-name`           | Pending | Remove declaration/definition mismatches that mislead callers.                                            |
| `misc-const-correctness`                                        | Pending | Protect local values from unintended writes; avoid indiscriminate churn.                                  |
| `readability-make-member-function-const`                        | Pending | Expose read-only operations for safer interfaces.                                                         |
| `misc-override-with-different-visibility`                       | Pending | Review surprising access changes across virtual interfaces.                                               |
| `misc-header-include-cycle`                                     | Pending | Reduce header coupling and fragile build dependencies.                                                    |
| `misc-include-cleaner`                                          | Pending | Review missing/redundant includes alongside existing IWYU checks.                                         |
| `clang-diagnostic-missing-prototypes`                           | Pending | Give externally visible functions consistent declarations.                                                |
| `clang-diagnostic-missing-variable-declarations`                | Pending | Give shared variables an explicit interface.                                                              |
| `misc-use-internal-linkage`                                     | Pending | Limit accidental symbol exposure and cross-unit coupling.                                                 |
| `misc-use-anonymous-namespace`                                  | Pending | Keep implementation details local to their translation unit.                                              |
| `clang-diagnostic-unneeded-internal-declaration`                | Pending | Remove unused internal declarations after configuration review.                                           |
| `clang-diagnostic-unused-but-set-global`                        | Pending | Find dead global state or missing consumers.                                                              |
| `misc-static-assert`                                            | Pending | Check compile-time invariants at compile time.                                                            |
| `cert-dcl03-c`                                                  | Pending | Alias of `misc-static-assert`; handle together.                                                           |
| `modernize-use-std-format`                                      | Pending | Improve format type safety where existing Abseil helpers do not suffice.                                  |
| `modernize-use-std-print`                                       | Pending | Modernize direct printing where it improves type safety and clarity.                                      |
| `performance-string-view-conversions`                           | Pending | Avoid unnecessary string copies at view boundaries.                                                       |
| `modernize-loop-convert`                                        | Pending | Simplify traversal after reviewing mutation and iterator behavior.                                        |
| `modernize-use-ranges`                                          | Pending | Simplify algorithms where ranges make intent clearer.                                                     |
| `cppcoreguidelines-macro-usage`                                 | Pending | Replace avoidable macros with typed language constructs.                                                  |
| `modernize-macro-to-enum`                                       | Pending | Replace suitable integral macro groups with typed constants/enums.                                        |
| `cppcoreguidelines-macro-to-enum`                               | Pending | Alias of `modernize-macro-to-enum`; handle together.                                                      |
| `bugprone-reserved-identifier`                                  | Pending | Avoid collisions with implementation-reserved identifiers.                                                |
| `cert-dcl37-c`                                                  | Pending | Alias of `bugprone-reserved-identifier`; handle together.                                                 |
| `cert-dcl51-cpp`                                                | Pending | Alias of `bugprone-reserved-identifier`; handle together.                                                 |
| `clang-diagnostic-reserved-identifier`                          | Pending | Enforce compiler-detected reserved names.                                                                 |
| `clang-diagnostic-reserved-macro-identifier`                    | Pending | Fix reserved macros; also reduce preprocessing warnings/cache misses.                                     |
| `clang-diagnostic-invalid-source-encoding`                      | Pending | Keep source portable across compiler/platform encodings.                                                  |
| `portability-template-virtual-member-function`                  | Pending | Review compiler-dependent template/virtual behavior.                                                      |
| `portability-avoid-pragma-once`                                 | Pending | Align headers with the project's include-guard convention.                                                |

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

### Completed validation

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
