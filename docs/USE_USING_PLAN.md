# Enable `modernize-use-using`

## Selection and scope

Revisit the skipped `modernize-use-using` check. The previous attempt failed because automatic
replacements duplicated inline type definitions and removed anonymous types' linkage names. Named
definitions and equivalent aliases address those failures while preserving existing interfaces,
storage, and behavior.

Cover both games, shared project sources, and generated header checks in the strict compilation
database. Keep dependency and generated vendor code unchanged. Use the installed LLVM 23 toolchain.

## Implementation

1. Run an isolated baseline sweep and inventory the unique diagnostic sites.
2. Replace ordinary typedefs with equivalent `using` declarations. Turn anonymous inline type
   definitions into named definitions using their existing typedef name. For already named types,
   retain the tag and introduce an alias only when the typedef name differs. Preserve enum values,
   fields, packing, array extents, callback signatures, and conditional compilation.
3. Review replacements manually instead of applying the broken inline-definition fix-its. Remove the
   check's global exclusion without adding suppressions or weakening its options.
4. Run the isolated and full-configuration sweeps; build both games with strict checks and also with
   GCC, which caught the previous linkage failure. Run CTest and save/load smoke checks.
5. Verify enforcement with a deliberately invalid typedef, update the priority entry and validation
   record, format documentation, and commit the complete change.

## Results

- Baseline: 112 unique `modernize-use-using` findings in 952 project translation units, including
  454 generated header checks.
- Converted 143 declarations in 72 files: 109 inline type definitions and 34 ordinary aliases.
  Conditional and unbuilt project declarations received the same conversion. The generated WOL
  header remains unchanged under the check's default `IgnoreExternC: true` behavior.
- Inline type bodies remain unchanged. Before/after Clang record-layout dumps for the RA and TD
  save/load translation units match after normalizing tag keywords and source locations (2,073 and
  1,774 layouts respectively).
- Giving records names exposes initialization diagnostics from the already enabled
  `cppcoreguidelines-pro-type-member-init` check. Initialized 44 local records in audio/video,
  encoding, packet, modem-dialog, and mono-display code, using their existing assigned values
  directly where possible. Serial packets explicitly initialize their nonzero command enum and all
  other fields, following the existing packet initializers. The type definitions remain trivial.
- An isolated typedef probe is rejected as an error by the repository configuration and passes with
  the old exclusion restored.
- The final isolated sweep passed all 952 translation units. The full-configuration sweep plus
  follow-up checks of all 30 changed compiled source files passed; every initially failing unit was
  included in the follow-up.
- Both games build and link under GCC and strict Clang (including the configured IWYU pass). All 557
  CTest tests pass with each compiler on the final source state.
- Both compilers pass the headless RA and TD save/load checks: 240 RA object positions and 5,742 TD
  game states match uninterrupted runs.
- Documentation passes Prettier, and `git diff --check` is clean.
