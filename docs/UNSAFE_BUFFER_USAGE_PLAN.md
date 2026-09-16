# Enable clang-diagnostic-unsafe-buffer-usage

The completion criterion is repository-wide enforcement, not a single defect: remove the clang-tidy
exclusion and the strict Clang warning suppression, then pass candidate analysis, the full
configured checks, both strict game builds, and the tests. Preserve simulation behavior, file
formats, and packet layouts.

## Work sequence

1. Commit the completed LCW destination-bounds fix and its regression tests as a standalone
   correctness change.
2. Measure the warning across all project translation units and generated header checks, excluding
   dependencies. Group unique findings by buffer/API and use compiler migration suggestions where
   they preserve behavior.
3. Migrate fixed arrays and their consumers to bounded access. Keep externally specified record
   layouts unchanged. Propagate spans/string views through internal APIs instead of recovering
   lengths from unbounded pointers.
4. Migrate byte-buffer, codec, rendering, and I/O operations with explicit sizes. Validate sizes at
   input boundaries. Any unavoidable C/OS interoperability boundary must have a narrow, reviewed
   bounds contract; do not suppress whole files or hide arbitrary indexing behind an unchecked
   helper.
5. Handle both games' remaining table, string, heap, and pointer-walking sites. Commit coherent
   API/component migrations and keep the inventory current.
6. Remove both exclusions. Run the candidate and full-config sweeps over all project units,
   strict-build both games, and run CTest. Use regression tests, sanitizer probes, and save/load
   smoke checks for behavior-sensitive changes.
7. Prove enforcement with a deliberately unsafe sample, mark the priorities row Enabled with the
   enabling commit, and commit the final documentation.

## Progress

- LCW destination-bounds fix complete and validated (see LCW_BUFFER_BOUNDS_PLAN.md).
- Baseline: 941 project translation units, 6,318 unique warnings (including the child diagnostics
  for unsafe C library calls and span construction). Two generated VQA header checks also exposed a
  missing public `base` dependency.
- Fixed-array batch in validation: 2,233 accesses migrated in 152 files; array storage layouts are
  unchanged. Checked element access rejects negative and excessive indices, while suffix views
  preserve legal one-past pointers.
- After the initial array rewrite: 943 units scanned, 4,345 warnings and no compile failures.
  Subsequent suffix fixes will be included in the next sweep.
- Next: null-terminated string views, then pointer/size APIs and buffer ownership.
- Enforcement remains pending until the entire remaining inventory is resolved.

### Fixed-array validation

Both strict game builds and all 486 CTest tests pass. Save/load smoke checks match 240 RA object
positions and 5,742 TD game states.

The new runtime checks exposed two existing defects during those smoke tests: RA's team-center
calculation indexed the mission list at the initial `-1` mission sentinel, and TD's display
initialization read palette ramps beyond a 256-byte row for higher house IDs. RA now checks that a
current mission exists before selecting hound-dog behavior. TD retains the base fading table when
the palette has no corresponding ramp; the only current consumer uses the unchanged GDI identity
row.

`base::At` and `EnumArray::end` keep their lifetime-bound contracts. Three narrow lifetime
diagnostic annotations document Clang 23's inability to trace the reference through libstdc++ span
indexing or the pointer through `std::end`. No unsafe-buffer diagnostic is suppressed in this batch.

### C-string view batch

Replaced 293 `strlen`/`strcmp` calls in 68 files with string-view operations, using equality
operators and `empty()` where appropriate. These retain the existing null-terminated-string
contracts and comparison ordering. Both strict game builds, all 486 CTest tests, and both save/load
smoke checks pass (240 RA positions and 5,742 TD states).
