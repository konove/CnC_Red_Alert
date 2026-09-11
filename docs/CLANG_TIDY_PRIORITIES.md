# Clang-tidy priorities

Updated: 2026-09-11, against [`.clang-tidy`](../.clang-tidy) and clang-tidy 23.1.2.

This tracks **all 237 currently excluded check names** and completed entries, in recommended work order.
Priorities reflect likely defect prevention, relevance to this engine, and the cost of useful fixes; they are judgments,
not fresh finding counts. Start at P1 and work downward. Aliases stay beside their related check so a
single cleanup can handle them together. Previously deferred checks are back on the list for review.

Keep completed rows in place: change **Status** to **Enabled** and record the commit in **Reason / result**.
Use **Skipped** with a short reason only after deciding against a check. P5 entries are recommendations
to skip, not completed decisions. Add newly excluded checks when the configuration changes.

- **Pending:** excluded and awaiting work.
- **Enabled:** enforced by the configuration; the result records the enabling commit.
- **Skipped:** reviewed but excluded for the recorded reason; revisit toolchain limitations after upgrades.
- **Covered:** this name is excluded, but an enabled equivalent already supplies the check.
- **Legacy:** excluded name unavailable in the installed LLVM 23 toolchain; review older-toolchain
  compatibility alongside the related check. Do not mistake zero findings for successful enforcement.
- **Skip proposed:** low expected value or an unsuitable platform/style policy.

Alias relationships can be checked in the [LLVM check index](https://clang.llvm.org/extra/clang-tidy/checks/list.html).
Availability above comes from the installed tool, since the online documentation follows LLVM development.

## P1 — Direct correctness and memory safety

| Check                                                         | Status  | Reason / result                                                                                                                                        |
|---------------------------------------------------------------|---------|--------------------------------------------------------------------------------------------------------------------------------------------------------|
| `bugprone-suspicious-stringview-data-usage`                   | Enabled | Commit `Enable string-view data usage checking`: copy the RA title-screen filename to a terminated string before calling the PCX reader. |
| `abseil-unchecked-statusor-access`                            | Skipped | Commit `Handle failed PCX byte reads`: guard both games' byte reads, but retain the exclusion because LLVM 23.1.2 crashes even on checked access with Abseil 20260107.0. Revisit after a toolchain fix; see reproduction below. |
| `clang-analyzer-unix.cstring.UninitializedRead`               | Enabled | Commit `Enable uninitialized C-string read checking`: initialize the public-key generation self-test buffer while preserving the random-fill loop. |
| `clang-analyzer-cplusplus.InnerPointer`                       | Enabled | Commit `Enable string inner-pointer checking`: detect string-buffer pointers used after invalidation. |
| `bugprone-copy-constructor-init`                              | Pending | Prevent copied objects from silently losing base/member state.                                                                                         |
| `bugprone-unchecked-string-to-number-conversion`              | Pending | Reject malformed and out-of-range input at parsing boundaries.                                                                                         |
| `cert-err34-c`                                                | Pending | Alias of `bugprone-unchecked-string-to-number-conversion`; handle together.                                                                            |
| `clang-analyzer-unix.StdCLibraryFunctions`                    | Pending | Find invalid arguments to modeled C library calls.                                                                                                     |
| `clang-diagnostic-cast-align`                                 | Pending | Catch pointers cast to types requiring stronger alignment.                                                                                             |
| `clang-diagnostic-uninitialized-const-pointer`                | Pending | Review pointer arguments that may expose uninitialized storage.                                                                                        |
| `clang-diagnostic-reorder-ctor`                               | Pending | Make constructor order explicit; check dependencies between members.                                                                                   |
| `bugprone-unhandled-code-paths`                               | Pending | Find missing outcomes in conditional control flow.                                                                                                     |
| `bugprone-non-zero-enum-to-bool-conversion`                   | Pending | Catch enum tests that are always true.                                                                                                                 |
| `clang-analyzer-optin.core.EnumCastOutOfRange`                | Pending | Validate integer-to-enum boundaries; distinguish bit masks.                                                                                            |
| `clang-diagnostic-tautological-constant-out-of-range-compare` | Pending | Find impossible comparisons hiding range-check mistakes.                                                                                               |
| `clang-diagnostic-tautological-unsigned-enum-zero-compare`    | Pending | Find enum checks that cannot detect invalid values.                                                                                                    |
| `clang-diagnostic-tautological-unsigned-zero-compare`         | Pending | Find ineffective negative checks on unsigned values.                                                                                                   |
| `clang-diagnostic-implicit-int-conversion`                    | Pending | Find remaining implicit loss of integer range or precision.                                                                                            |
| `clang-diagnostic-implicit-int-conversion-on-negation`        | Pending | Review negation that changes range during conversion.                                                                                                  |
| `clang-diagnostic-int-to-pointer-cast`                        | Pending | Find truncated or invalid addresses in legacy casts.                                                                                                   |
| `bugprone-derived-method-shadowing-base-method`               | Pending | Find unintended hiding in the game class hierarchies.                                                                                                  |

## P2 — Further correctness and targeted safety

| Check                                                      | Status  | Reason / result                                                                                         |
|------------------------------------------------------------|---------|---------------------------------------------------------------------------------------------------------|
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
|-----------------------------------------------------------------|---------|-----------------------------------------------------------------------------------------------------------|
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
|---------------------------------------------------------------------|---------|------------------------------------------------------------------------------------------------|
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
|-----------------------------------------------|---------------|------------------------------------------------------------------------------------|
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
`clang::dataflow::statusor_model::getSyntheticFields` on both games' `Read_PCX_File`, before and after
guarding the byte reads. The following checked access also reproduces it with LLVM 23.1.2 and the
project's Abseil 20260107.0 headers:

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
missing-pixel regression test aborts against the original TD reader
with an unchecked `OUT_OF_RANGE` access and passes with the fix.

### Completed validation

The 2026-09-11 inner-pointer check cleanup covered 879 project translation units, including 428
generated header checks. Both the isolated and full-config sweeps passed without findings, so no
source fixes were needed. Both strict game builds and all 205 CTest tests passed. A sample using a
string buffer pointer after `clear()` confirmed the enabled check reports an error.

The 2026-09-11 C-string uninitialized-read cleanup covered 879 project translation units, including
428 generated header checks. The isolated scan found one path from the key-generation self-test
buffer into `PKey::Encrypt`; initializing that buffer preserves the existing random-fill loop.
The affected file passed the isolated check, the full-config sweep passed, both strict game builds
passed, and all 205 CTest tests passed. An uninitialized `memcpy` sample confirmed enforcement.

The 2026-09-11 string-view check cleanup covered 878 project translation units, including 428 generated
header checks. The isolated scan found one call; its fix passed the isolated check and the full-config
sweep. Strict builds of both games and all 197 CTest tests passed. A deliberately unsafe sample confirmed
the enabled check reports an error.

### Workflow

1. Measure the check across both games and shared code, including generated header checks and excluding
   dependencies. Verify the check exists; handle aliases and options together.
2. Fix findings, run the candidate and full-config sweeps, then strict-build both games and run CTest.
   Add focused behavior tests and save/load checks when the changes warrant them.
3. Remove the exclusion, verify enforcement, and mark the row **Enabled** with its commit. If enabling
   is unsuitable, record the concrete reason rather than silently dropping the row.

Compiler diagnostic filters also depend on warning flags. In particular, `sign-conversion`,
`unsafe-buffer-usage`, `old-style-cast`, `padded`, and `covered-switch-default` are suppressed in
[CMakeLists.txt](../CMakeLists.txt); removing their tidy exclusions alone does not enable them.
An isolated diagnostic sweep needs its warning flag and at least one real clang-tidy check.

Preserve the [type-migration policy](TYPE_MIGRATION.md), deterministic simulation RNG, and packet/recording
layouts when applying broad rules. Field-wise savegames do not make all layout changes safe.
