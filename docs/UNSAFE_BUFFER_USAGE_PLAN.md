# Enable clang-diagnostic-unsafe-buffer-usage

Completed: the whole diagnostic family is enabled, including its unsafe libc-call and span-container
children. Both the `.clang-tidy` exclusion and strict Clang warning suppression are removed. The
completion criterion was repository-wide enforcement, not a single defect.

## Plan and completed work

1. Fix LCW destination overruns and invalid back-references with regression tests.
2. Inventory every configured project translation unit and generated header, excluding dependencies.
3. Replace raw fixed-array indexing with checked access while retaining storage layouts.
4. Carry string and byte extents through internal APIs; validate sizes at input boundaries.
5. Migrate both games, shared codecs, rendering/audio, networking, profiles, containers, and UI.
   Parallel agents handled independent API chains while integration and validation remained central.
6. Enable the complete diagnostic family, run all configured checks, strict-build both games, and
   verify tests, sanitizers, save/load behavior, and generated headers.
7. Prove enforcement with deliberately unsafe code, update the priorities row, and commit.

## Implementation history

| Commit                                                    | Completed change                                                                   |
| --------------------------------------------------------- | ---------------------------------------------------------------------------------- |
| `ccb16f17`                                                | LCW output command bounds and back-reference validation.                           |
| `c48eea0b`                                                | Repository-wide implementation and validation plan.                                |
| `a0efc4fa`                                                | 2,233 fixed-array accesses in 152 files; checked access preserves storage layouts. |
| `0e136dda`                                                | 293 C-string length/comparison operations in 68 files use string views.            |
| `91e85664`                                                | 155 bounded memory operations and extent-preserving address APIs.                  |
| `070fd49d`                                                | Bounded byte views and checked span cursor consumption.                            |
| `452b8a5c`                                                | Complete buffer API migration, both games, and regression coverage.                |
| `Enforce clang unsafe buffer diagnostics repository-wide` | Configuration enablement and final record.                                         |

The initial 941-unit inventory contained 6,318 unique diagnostics, including child diagnostics. The
final inventory contains 952 units, reflecting added tests and generated header checks.

Bounds now travel from an allocation, container, or archive resource to its consumers. Internal
pointer/size pairs became spans, sentinel traversals consume bounded views, and packet/resource
parsers validate truncated data before access. Existing save, network, and binary record layouts
remain unchanged. Vector/heap adapters retain serialized fields and derive views from their owners
or validated external spans.

Runtime checks also exposed defects beyond the original LCW overrun: a negative initial RA mission
index, a TD palette-row overread, malformed packet lengths, path/occupancy boundary cases, short PCX
input, profile/message buffer limits, and ignored SDL lock failures. Regression coverage and
save/load comparisons accompany the relevant fixes. Rendering rejects invalid dimensions, short
frame data, and missing surfaces before SDL side effects.

## Narrow interoperability annotations

No whole-file unsafe-buffer suppression or arbitrary pointer/count indexing wrapper replaces the
check. Reviewed annotations cover exact owned allocations, object representations, aliases of an
existing bounded span, and external APIs that supply their allocation extents. SDL lock-derived
views require successful locks. A few annotations cover bounded C interoperability operations and
validated descriptor macros. GoogleTest death-test expansions have local annotations for the
framework's formatted diagnostics. Separate lifetime annotations document Clang's inability to trace
some references through standard-library span access.

## Final validation

Validated on the configured Linux Clang 23.1.2 build:

- Full configured clang-tidy plus the complete unsafe-buffer family: **952 units, zero findings**
  (385 RA, 312 TD, and 255 shared-library/test/header units). Source analysis includes project
  headers, including instantiated templates.
- Both games and all test targets build with strict checks and enforcement enabled. A subsequent
  default build reports no work remaining.
- **547/547 CTest tests pass**, including new codec, packet, profile, container, message, icon,
  rendering, and counter regressions.
- ASan/UBSan probes pass for queue decoding, network buffers, profiles/strings, and text formatting.
  Queue coverage includes every truncation of 1/2/3/255-unit compressed mission runs, variable
  payload claims through UINT32_MAX, full-queue cleanup, and 50,000 randomized packets. Leak
  detection is disabled for the sandbox's process-inspection restriction.
- Final save/load smoke checks match **240 RA object positions** and **5,742 TD game states**. RA's
  older save fixture also loads and runs through frame 120.
- A deliberately unsafe raw-pointer indexing sample fails under the actual configuration with
  `clang-diagnostic-unsafe-buffer-usage` treated as an error. The probe is not committed.
- Changed-file whitespace checks pass.

The optional `all_verify_interface_header_sets` build encountered reproducible **IWYU tool
segfaults** on eight generated headers: SDL `shape.h`; RA `const.h`, `interpal.h`, `netdlg.h`,
`taction.h`, and `tevent.h`; TD `const.h` and `interpal.h`. These are not compiler or clang-tidy
findings. Every other generated-header build step passed. All eight affected headers separately
passed direct compilation with the configured compiler flags and full enabled clang-tidy. No project
diagnostic was disabled to work around IWYU, and the ordinary strict build passes.
