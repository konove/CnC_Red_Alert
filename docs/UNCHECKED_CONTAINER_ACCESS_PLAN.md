# Checked container access

Selected check: `cppcoreguidelines-pro-bounds-avoid-unchecked-container-access`.

The earlier P2 parameter-order check primarily reports conventional coordinate and size arguments.
This first skipped P3 bounds check can add runtime protection to container access without changing
object layouts, serialized data, or simulation arithmetic.

## Plan

1. Inventory the installed check across both games, shared code, tests, and generated header
   translation units, excluding dependencies. Review the reported container types and verify the
   available fix modes.
2. Replace unchecked accesses with checked accessors. Reuse the existing `base::At` infrastructure
   where standard containers lack a suitable accessor. Preserve reference semantics, constness, and
   single evaluation of operands. Test valid access and rejected indices, including release-mode
   bounds enforcement.
3. Remove the global exclusion and resolve every finding. Avoid broad suppressions or exclusions of
   container classes; keep any necessary implementation-level annotation narrowly scoped to an
   operation protected by an explicit bounds check.
4. Run isolated and full-configuration clang-tidy sweeps, strict builds of both games, CTest, and
   headless save/load checks. Verify enforcement with an intentionally unsafe sample. Record results
   in the priorities document and commit the completed change.

## Results

The LLVM 23.1.2 inventory covered 952 project translation units and found 4,089 unique diagnostic
sites in 292 files. It includes both games, shared libraries, tests, and generated header checks,
and excludes third-party dependencies.

- Standard containers now use `.at()`. The legacy vectors, heaps, map, enum tables, palettes, UI
  lists, digit cursors, and queues expose named `at()` accessors; their subscript operators delegate
  to those accessors for compatibility.
- `base::At` now accepts fixed- and dynamic-extent spans, checks signed and unsigned indices, and
  returns the original element reference. Iterator subscripts in rendering, palette, and compression
  loops use the owning span's bounds.
- Queue access checks the logical active count before applying the circular storage offset.
  Previously an empty queue, a negative index after advancing the head, or a past-count index could
  return an inactive storage slot. Vector capacity access now checks bounds in release builds as
  well as debug builds.
- Object fields, record layouts, index conversions, RNG calls, and valid access results are
  unchanged. The incidental TD tokenizer initialization uses braces to avoid GCC's most-vexing-parse
  error encountered during validation.
- No container exclusions or suppressions for the selected check were added. LLVM's fix-its were
  reviewed and corrected where dependent or parenthesized expressions produced invalid replacement
  ranges. FixMode remains at its default so the repository does not advertise those malformed
  automatic fixes.
- Removed 71 includes made obsolete by named member access. Kept the existing signed-facing-shift
  suppression attached to its expression after formatting.

## Validation

- Both GCC game builds and all 557 CTest tests passed.
- Thirteen focused array/span/vector/queue/bit-access tests also passed with `NDEBUG`. Removing the
  queue's two new guards in a temporary header made the regression test fail as expected, without
  changing the repository sources.
- GCC headless save/load checks matched 240 RA object positions and 5,951 TD states in the team
  fixture. The map and mobile fixtures matched 5,891 and 6,212 states, respectively. The map fixture
  preserved 16 sparse cells and the wide map value; the corrupt-save checks rejected eight map and
  seven mobile saves. The existing RA save fixture also loaded and reached frame 120.
- An unchecked vector subscript reports the selected check as an error with the repository
  configuration; restoring the previous exclusion makes the probe pass without errors.
- Final isolated and full-configuration sweeps passed all 952 translation units. The two
  subsequently added bit-access tests also pass a full-configuration recheck.
- Both strict Clang game builds passed, including the final incremental build. All 557 CTest tests
  passed against that build. The strict binaries matched 240 RA object positions and 5,951 TD
  team-fixture states across save/load.

Completed in commit `Enable checked container access across both games`.
