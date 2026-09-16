# LCW destination bounds plan

Selected from the skipped `clang-diagnostic-unsafe-buffer-usage` row. The higher-priority lifetime
invalidation diagnostic still flags valid consecutive vector mutations on installed Clang 23.1.2;
switch and parameter checks retain their documented policy tradeoffs.

1. Reproduce a literal command writing past the advertised destination capacity.
2. Bound every output command in `LCW_Uncompress`, validate back-references before forming pointers,
   and return the decoded prefix on invalid references. Preserve forward overlapping copies,
   zero-count commands, and the existing C ABI.
3. Add hand-encoded regression cases for all five command forms, invalid references, nonpositive
   capacity, overlap, and end markers. Demonstrate failure before the fix.
4. Run focused sanitizer checks, candidate and full-config analysis on changed code, strict builds
   of both games, CTest, and both save/load smoke checks.
5. Record results in the priorities document. Keep the broad exclusion: this fixes destination
   safety in one decoder, not all raw-buffer APIs. Source truncation requires a source-size API and
   is outside this destination-bounds issue.

## Completed results

- Reproduced the original overrun: a one-byte capacity returned three bytes and changed the guard
  from `0xcc` to `0x62`. The fixed decoder returns one and preserves the guard. `BoundsEveryCommand`
  fails against the original decoder.
- All seven new tests pass under AddressSanitizer and UBSan; leak detection was disabled for the
  sandbox's process-inspection restriction.
- Full-config clang-tidy passes the decoder and its new tests. The isolated unsafe-buffer diagnostic
  still reports 14 raw-pointer operations; the global exclusion is retained and no local
  suppressions were added.
- Both games build with the existing strict configuration. All 482 tests in the rebuilt CTest suite
  pass, including all seven new regressions.
- Save/load checks match 240 RA object positions and 5,742 TD game states.
- The priorities row and follow-up review record the fix and remaining API scope.
