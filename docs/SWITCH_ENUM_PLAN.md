# Exhaustive enum switch checking

Selected check: `clang-diagnostic-switch-enum`, the first remaining skipped P2 check.

## Plan

1. Measure the diagnostic across both games, shared code, tests, and generated header checks using
   the strict compilation database. Identify each switch and its intentionally shared fallback.
2. Express sparse bridge-template membership tests as direct comparisons; add explicit labels for
   known enum values that currently use the fallback, keeping their behavior and the default for
   invalid values. Do not cast away enum types, suppress the warning, or change enum values,
   layouts, or simulation logic.
3. Remove the global exclusion; verify the isolated and full-configuration sweeps, strict builds of
   both games, CTest, and a negative enforcement probe. Review the diff for preserved fallthrough.
4. Record results in this plan and the priorities document and commit this work. Preserve and
   exclude the pre-existing edits in `src/ra/conquer.cc` from the commit.

## Changes and behavior checks

The 952-unit baseline sweep (498 source files and 454 generated header checks) reported 335 switches
in 95 files, all in the two games: 195 RA and 140 TD.

- 328 switches gain 6,584 labels at their existing default branches. Enum aliases share a single
  label for each numeric value. A source audit removes those added groups and verifies the original
  source is recovered, apart from the separately reviewed bridge rewrites.
- Seven sparse bridge-template checks use direct comparisons: six in RA (bridge recognition,
  sabotage targeting, three bridge-damage updates, and intact bridge counting) and one in TD
  (tiberium spread exclusions). The branch actions, invalid-value behavior, and icon-6 requirement
  are preserved. Extracted selection probes using the real game enums matched all 65,536 input
  values at icon positions 0, 6, and 7.
- No enum values, object layouts, RNG calls, packet formats, default bodies, or fallthrough paths
  change. No check suppressions, casts to integer, or warning-flag exclusions were added.
- A standalone switch with an unhandled enum value and a default produces exactly
  `clang-diagnostic-switch-enum` under the full configuration. Restoring only the exclusion makes
  that probe pass. Both compilers already enable `-Wswitch-enum` in strict builds.

## Validation

The isolated sweep and final rechecks pass all 952 translation units. The full-configuration sweep
passes all 952 translation units with zero failures, including all 454 generated header checks. Both
strict game builds pass, and all 547 CTest tests pass. The existing compiler and tidy caches were
directed to writable directories under `/tmp` during the build.

The negative enforcement probe, label-group source audit, and exhaustive bridge selection probes
also pass. No new runtime feature or serialization change was introduced, so no additional save/load
smoke run was needed. The pre-existing `src/ra/conquer.cc` changes were present during validation
and are excluded from the commit.

Completed in commit `Make enum switch fallbacks explicit and enable exhaustive checking`.
