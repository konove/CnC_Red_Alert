# Lifetime invalidation check

Selected check: `clang-diagnostic-lifetime-safety-invalidation`, the first skipped entry in P2. The
previous review fixed dangling globals but found false positives on valid container mutations in
LLVM 23.1.2.

## Plan

1. Run the diagnostic across both games, shared code, tests, and generated header checks using the
   strict compilation database. Reproduce the reported container limitation independently and verify
   that genuine invalidation is detected.
2. Fix genuine lifetime errors. Annotate confirmed compiler false positives only at the offending
   statements, with an explanation; preserve normal container operations and keep the diagnostic
   active elsewhere.
3. Remove the global exclusion. Run isolated and full-configuration checks, strict builds of both
   games, and CTest. Verify enforcement with a negative probe.
4. Record the measured results in the priorities document and commit only this work, preserving the
   pre-existing edits in `src/ra/conquer.cc`.

## Results

The isolated scan of 952 translation units (498 source files and 454 generated header checks) found
16 diagnostic sites in nine files. Every site is a confirmed LLVM 23.1.2 container false positive.
All nine affected files pass the isolated recheck after adding statement/declaration-specific
annotations.

| Pattern                                        | Sites | Why the reported object remains valid                                              |
| ---------------------------------------------- | ----: | ---------------------------------------------------------------------------------- |
| Test helpers appending bytes                   |     9 | Appending may invalidate element storage, but not the vector reference.            |
| RA INI writers clearing/assigning their output |     2 | The string reference survives mutations; no old character view is retained.        |
| RA capture-frame vector resize                 |     1 | The vector object survives resizing; element views are acquired afterward.         |
| Both games clearing palette vectors in arrays  |     4 | Mutating an inner vector does not invalidate iteration over the fixed outer array. |

The global exclusion is removed. No runtime operations, ownership, layouts, or serialization formats
change. Each annotation names only this diagnostic and explains the lifetime reasoning. These
workarounds should be reviewed after an LLVM upgrade; they do not fix the compiler's underlying
analysis limitation.

## Enforcement probe

With the repository configuration, the following deliberately invalid global produces
`clang-diagnostic-lifetime-safety-invalidation` as an error:

```cpp
int* dangling;
void InvalidGlobal() {
  dangling = new int(1);
  delete dangling;
}
```

```sh
clang-tidy --config-file=.clang-tidy /tmp/lifetime-probe.cc -- -std=c++23 -Weverything
```

Restoring only `--checks=-clang-diagnostic-lifetime-safety-invalidation` suppresses that diagnostic.
A separate probe detects use of an element reference after `vector::clear()`. The valid
two-`push_back` reproduction still triggers on this toolchain, confirming why the local annotations
are necessary.

The full-configuration sweep passed all 952 translation units, including all 454 generated header
checks, with no errors. Both strict game builds passed, and all 547 CTest tests passed. The existing
compiler and tidy caches were redirected to `/tmp` because the normal compiler-cache directory is
read-only in the sandbox.

No new runtime tests or save/load smoke runs were needed: the source changes are comments only. The
pre-existing edits in `src/ra/conquer.cc` were present during validation and are excluded from this
commit.

Completed in commit `Enable lifetime invalidation checking with scoped LLVM workarounds`.
