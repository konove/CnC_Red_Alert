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
- Repository-wide measurement and migration pending.
