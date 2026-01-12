# Clang-Tidy Check Prioritization Analysis

## Overview

The `.clang-tidy` file enables all checks (`'*'`) then disables ~200 specific checks. This analysis prioritizes which
disabled checks to re-enable and fix, ordered by **bug severity**, **fix effort**, and **value for legacy code**.

---

## TIER 1: HIGH PRIORITY - Real Bugs (Tackle First)

These find actual bugs, crashes, and security issues. Worth the effort even in legacy code.

### 1.1 Null/Uninitialized (Common crash sources)

| Check                                            | Why Enable                             | Effort                            |
|--------------------------------------------------|----------------------------------------|-----------------------------------|
| `clang-analyzer-core.NullDereference`            | Finds null pointer crashes             | Medium - may have false positives |
| `clang-analyzer-core.uninitialized.*` (3 checks) | Uninitialized variable bugs            | Medium                            |
| `clang-diagnostic-uninitialized`                 | Compiler-level uninitialized detection | Low                               |
| `clang-diagnostic-sometimes-uninitialized`       | Conditional uninitialized paths        | Low                               |

### 1.2 Memory Safety

| Check                                       | Why Enable                | Effort |
|---------------------------------------------|---------------------------|--------|
| `clang-analyzer-cplusplus.NewDelete`        | new/delete mismatches     | Medium |
| `clang-analyzer-unix.MismatchedDeallocator` | malloc/free vs new/delete | Medium |
| `clang-diagnostic-mismatched-new-delete`    | Array vs scalar delete    | Low    |
| `clang-diagnostic-delete-incomplete`        | Deleting incomplete types | Low    |

### 1.3 Critical UB/Security

| Check                                               | Why Enable                  | Effort         |
|-----------------------------------------------------|-----------------------------|----------------|
| `clang-diagnostic-return-stack-address`             | Returning dangling pointers | Low - must fix |
| `clang-analyzer-core.UndefinedBinaryOperatorResult` | Undefined behavior          | Medium         |
| `bugprone-not-null-terminated-result`               | String buffer overflows     | Medium         |

---

## TIER 2: MEDIUM PRIORITY - Code Quality (Valuable improvements)

Worth enabling incrementally. Can be fixed file-by-file.

### 2.1 Easy Wins (Low effort, high value)

| Check                                | Why Enable                 | Effort                  |
|--------------------------------------|----------------------------|-------------------------|
| `clang-diagnostic-suggest-override`  | Missing `override` keyword | Very Low - auto-fixable |
| `readability-redundant-control-flow` | Useless return/continue    | Very Low                |
| `readability-redundant-declaration`  | Duplicate declarations     | Very Low                |
| `clang-diagnostic-self-assign`       | `x = x` bugs               | Very Low                |
| `misc-redundant-expression`          | `x == x` type bugs         | Low                     |

### 2.2 Switch Statement Issues

| Check                                  | Why Enable                 | Effort |
|----------------------------------------|----------------------------|--------|
| `bugprone-switch-missing-default-case` | Unhandled cases            | Medium |
| `clang-diagnostic-switch-enum`         | Missing enum cases         | Medium |
| `hicpp-multiway-paths-covered`         | Incomplete switch coverage | Medium |

### 2.3 Type Safety (Prevent subtle bugs)

| Check                            | Why Enable                 | Effort             |
|----------------------------------|----------------------------|--------------------|
| `bugprone-narrowing-conversions` | Data loss in conversions   | High - pervasive   |
| `clang-diagnostic-sign-compare`  | Signed/unsigned comparison | High - very common |
| `bugprone-signed-char-misuse`    | char vs unsigned char      | Medium             |

### 2.4 Dead Code / Unused

| Check                                | Why Enable             | Effort                       |
|--------------------------------------|------------------------|------------------------------|
| `clang-analyzer-deadcode.DeadStores` | Assignments never read | Medium                       |
| `clang-diagnostic-unused-variable`   | Unused variables       | Low                          |
| `clang-diagnostic-unused-function`   | Dead functions         | Low                          |
| `clang-diagnostic-unused-parameter`  | Unused params          | Low - use `[[maybe_unused]]` |

---

## TIER 3: LOW PRIORITY - Style/Modernization (Tackle Last)

These are "nice to have" but require significant refactoring. Consider for new code only.

### 3.1 Modern C++ (Massive effort)

| Check                    | Why Defer                         | Notes                         |
|--------------------------|-----------------------------------|-------------------------------|
| `modernize-use-nullptr`  | Replace `NULL`/`0` with `nullptr` | Thousands of changes          |
| `modernize-use-auto`     | Use `auto` keyword                | Style preference              |
| `modernize-loop-convert` | Range-based for loops             | Many loops to convert         |
| `modernize-use-using`    | `using` vs `typedef`              | Low value                     |
| `modernize-use-override` | Add `override`                    | Duplicate of suggest-override |

### 3.2 C-Style Casts (Everywhere in legacy code)

| Check                                    | Why Defer                | Notes              |
|------------------------------------------|--------------------------|--------------------|
| `clang-diagnostic-old-style-cast`        | C-style casts everywhere | Thousands of casts |
| `cppcoreguidelines-pro-type-cstyle-cast` | Same                     | Duplicate          |
| `google-readability-casting`             | Same                     | Duplicate          |

### 3.3 Member Initialization

| Check                                         | Why Defer              | Notes          |
|-----------------------------------------------|------------------------|----------------|
| `cppcoreguidelines-pro-type-member-init`      | Initialize all members | Major refactor |
| `cppcoreguidelines-prefer-member-initializer` | Use init lists         | Style          |

---

## TIER 4: KEEP DISABLED (Inappropriate for this codebase)

### 4.1 Platform-Specific (Not applicable)

- `altera-*` - FPGA specific
- `android-*` - Android specific
- `fuchsia-*` - Fuchsia OS specific
- `llvmlibc-*` - LLVM libc specific

### 4.2 Architectural (Would require complete rewrite)

| Check                                                | Why Keep Disabled                             |
|------------------------------------------------------|-----------------------------------------------|
| `cppcoreguidelines-avoid-non-const-global-variables` | Entire game architecture is globals           |
| `cppcoreguidelines-no-malloc`                        | Legacy memory management                      |
| `cppcoreguidelines-owning-memory`                    | No smart pointers in legacy code              |
| `readability-magic-numbers`                          | Game code is inherently full of magic numbers |
| `cppcoreguidelines-avoid-c-arrays`                   | C arrays everywhere                           |
| `cppcoreguidelines-pro-bounds-pointer-arithmetic`    | Fundamental to the code                       |

### 4.3 Style Preferences (Subjective)

- `readability-braces-around-statements` - Style choice
- `readability-identifier-length` - Short names are fine
- `readability-else-after-return` - Style preference
- `llvm-else-after-return` - Same

### 4.4 False Positive Prone

| Check                                  | Why Keep Disabled                     |
|----------------------------------------|---------------------------------------|
| `bugprone-easily-swappable-parameters` | Too many false positives in game code |
| `bugprone-branch-clone`                | Intentional similar branches          |
| `cert-err58-cpp`                       | Static init order, hard to fix        |

---

## RECOMMENDED ACTION PLAN

### Phase 1: Quick Wins

1. Enable `clang-diagnostic-suggest-override` - auto-fix with clang-tidy
2. Enable `clang-diagnostic-self-assign`
3. Enable `readability-redundant-control-flow`
4. Enable `clang-diagnostic-return-stack-address`

### Phase 2: Memory Safety

1. Enable `clang-analyzer-core.NullDereference` - fix warnings
2. Enable `clang-diagnostic-delete-incomplete`
3. Enable `clang-diagnostic-mismatched-new-delete`

### Phase 3: Uninitialized Variables

1. Enable `clang-diagnostic-uninitialized`
2. Enable `clang-analyzer-core.uninitialized.*`

### Phase 4: Incremental (ongoing)

1. Enable checks file-by-file using `// NOLINTBEGIN` / `// NOLINTEND` for legacy files
2. Require new code to pass all reasonable checks

---

## Summary

| Tier   | Checks | Action                        |
|--------|--------|-------------------------------|
| Tier 1 | ~12    | Enable ASAP - finds real bugs |
| Tier 2 | ~20    | Enable incrementally          |
| Tier 3 | ~50    | New code only, or never       |
| Tier 4 | ~120   | Keep disabled permanently     |
