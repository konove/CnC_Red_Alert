# Plan: the VQ frame decoder in Rust

Written 2026-10-09. Phase 3 of `docs/RUST_MIGRATION_PLAN.md`, in the form of
`docs/RUST_LCW_PLAN.md`, whose build setup and global constraints it inherits.

**Status: done** (2026-10-09). With LCW and the audio decoders, a VQA movie is now decoded in Rust
from its chunks to its pixels and samples.

**Goal:** `DecodeVqFrame()` (`engine/video/vqa/vq_decoder.h`) decodes in Rust, with the same image
bytes for every input and no caller changed.

## Findings

- One caller, `MovieDrawer` (`engine/video/vqa/movie_drawer.cc`), which passes a codebook and a
  pointer buffer (each an `LcwBuffer`) and its own image. Three separate buffers: nothing is decoded
  in place, and the shim `CHECK`s that the image shares no byte with the other two.
- The two block shapes differ in the block height (2 or 4) and in the high byte that marks a solid
  block (0x0F or 0xFF). The bridge takes both as plain integers, so `BlockShape` stays a C++ type.
- An entry past the end of the codebook stops the decode at that block and keeps the blocks before
  it. The Rust decoder writes a pixel row at a time, which the compiler turns into wider copies, so
  it first counts the blocks of a block row that come before such an entry and draws only those.

## Files

| Path                                                        | Change                              |
| ----------------------------------------------------------- | ----------------------------------- |
| `src/rust/src/vq.rs`                                        | New: `decode_frame()` and its tests |
| `src/rust/src/lib.rs`                                       | `vq_decode_frame` on the bridge     |
| `src/engine/video/vqa/vq_decoder.cc`                        | Shim; `BlockShapeFor()` stays C++   |
| `src/engine/video/vqa/vq_decoder_reference_test_util.h/.cc` | New: the C++ decoder, test-only     |
| `src/engine/video/vqa/vq_decoder_differential_test.cc`      | New                                 |
| `src/tools/kernel_bench.cc`                                 | One row                             |

## Tasks

- [x] `vq.rs` with cargo tests
- [x] Bridge, shim, reference copy
- [x] Differential test: 4,000 frames that fit (at least 3,900 of them must draw), 16,000 whose
      codebook, pointers, image, stride and block counts are off by a little or a lot, and the
      overlap death test
- [x] `kernel_bench`, `ctest`, the strict build
- [x] Real movies decode identically

## Outcome

The C++ reference (`vq_decoder_reference_test_util`), `VqDecoderDifferentialTest` and `kernel_bench`
were deleted on 2026-10-15 (T-360). Their last run, that day: Rust 4,698 MB/s, C++ 2,026 MB/s. The
death test moved to `vq_decoder_test.cc`, with a new case for an entry that needs its high byte and
one past the end of the codebook.

Every existing test passes unchanged. `kernel_bench` at `RelWithDebInfo` (GCC 13 for the C++
reference), a 320x200 frame of 4x2 blocks: Rust 4,551 MB/s, C++ 2,300 MB/s (+98%).

The frame-dump comparison the migration plan asks for was done in the decoder instead of on dumps: a
temporary build ran the C++ reference beside every Rust call and `CHECK`ed that the outputs were
equal, for this phase and for Phase 2's decoders. Played with `vqaplay` under SDL's offscreen video
and dummy audio drivers:

| Run                                 | VQ frames | Movie sound chunks | .AUD ADPCM blocks |
| ----------------------------------- | --------- | ------------------ | ----------------- |
| Red Alert `REDINTRO.VQA`            | 188       | 189                |                   |
| Red Alert `SOVIET1.VQA`             | 1,812     | 1,819              |                   |
| Tiberian Dawn `GDI1.VQA`            | 549       | 550                |                   |
| Red Alert `SCG01EA`, 600 frames     |           |                    | 1,383             |
| Tiberian Dawn `SCG01EA`, 600 frames |           |                    | 1,509             |

No call differed. None of these runs decodes a Westwood 8-bit block; that decoder is held to the
reference by its differential test alone.
