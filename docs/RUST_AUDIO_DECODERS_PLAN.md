# Plan: the audio decoders in Rust

Written 2026-10-09. Phase 2 of `docs/RUST_MIGRATION_PLAN.md`, in the form of
`docs/RUST_LCW_PLAN.md`, whose build setup and global constraints it inherits.

**Status: done in code** (2026-10-09). Listening to a sound and a movie on a real device is open.

**Goal:** `DecodeAdpcmBlock()` and `DecodeWestwoodBlock()` (`engine/codec/aud_decoder.h`) and
`ImaAdpcmDecoder::Decode()` (`engine/video/vqa/adpcm_decoders.h`) decode in Rust, sample for sample
as before, with no caller changed.

## Findings

- **One IMA kernel serves both shims.** The two C++ decoders computed the same thing
  (`(2 * magnitude + 1) * step / 8`, negated by bit 3, saturated to 16 bits) and differed only in
  the width of their state fields and in the output: a vector of samples for the mixer, bytes in the
  movie's sound buffer for the VQA loader. `adpcm.rs` has one `State` and three loops around it.
- **State crosses as a small POD,** `engine_rs::ImaAdpcmState { predictor, step_index }`, passed in
  and written back. `AdpcmState` and `ImaAdpcmDecoder`'s private fields keep their types; the shims
  convert. A step index outside the table, which no decode produces, reads as the last entry instead
  of failing `base::At`'s check.
- **`ImaAdpcmDecoder::Decode` works in place.** `MovieLoader::LoadAdpcmSound()` reads the chunk into
  the end of the ring or staging buffer and decodes to its front. The shim routes overlapping spans
  to `ima_adpcm_decode_bytes_in_place`, which takes the whole buffer once, and `CHECK`-fails on a
  stream that starts before its destination and overlaps it, as the LCW shim does.
  `DecodeAdpcmBlock` and `DecodeWestwoodBlock` return fresh vectors, so nothing can overlap.
- **cxx has no `Option` and a returned `Vec` would be copied,** so the Westwood decoder crosses as
  two calls: `westwood_audio_decoded_len` (-1 for a block whose command runs past its end) sizes the
  C++ vector, `westwood_audio_decode` fills it.

## Files

| Path                                                            | Change                                 |
| --------------------------------------------------------------- | -------------------------------------- |
| `src/rust/src/adpcm.rs`, `westwood_audio.rs`                    | New: the kernels and their tests       |
| `src/rust/src/lib.rs`                                           | Five bridge functions, `ImaAdpcmState` |
| `src/engine/codec/aud_decoder.cc`                               | Shims                                  |
| `src/engine/video/vqa/adpcm_decoders.cc`                        | Shim with the in-place route           |
| `src/engine/codec/aud_decoder_reference_test_util.h/.cc`        | New: the C++ decoders, test-only       |
| `src/engine/video/vqa/adpcm_decoders_reference_test_util.h/.cc` | New: likewise                          |
| `src/engine/codec/aud_decoder_differential_test.cc`             | New                                    |
| `src/engine/video/vqa/adpcm_decoders_differential_test.cc`      | New                                    |
| `src/engine/video/vqa/CMakeLists.txt`                           | `engine_vqa` links `engine_rs_cxx`     |
| `src/tools/kernel_bench.cc`                                     | Three audio rows                       |

## Tasks

- [x] `adpcm.rs`, `westwood_audio.rs` with cargo tests
- [x] Bridge, shims, reference copies
- [x] Differential tests: every code at every step index from five predictors; 4,000 random states
      with the state carried over three blocks; separate and in-place movie decodes over every
      layout of stream and destination, including ones where the output overtakes the stream;
      well-formed, truncated and random Westwood blocks
- [x] `kernel_bench`, `ctest`, the strict build
- [ ] Listen to one sound and one movie on a real device

## Outcome

Every existing test passes unchanged. `kernel_bench` at `RelWithDebInfo` (GCC 13 for the C++
reference), megabytes of decoded sound from 64 KiB of random input:

| Kernel                 | Rust     | C++      | Change |
| ---------------------- | -------- | -------- | ------ |
| IMA ADPCM, .AUD block  | 819 MB/s | 788 MB/s | +4%    |
| IMA ADPCM, movie chunk | 808 MB/s | 757 MB/s | +7%    |
| Westwood 8-bit block   | 924 MB/s | 931 MB/s | -1%    |

The Westwood row moves between -1% and +0% from run to run.

What it took to get there:

- The first IMA kernel, a straight port using `clamp`, ran at **a third** of the C++ speed. LLVM
  compiled both clamps as branches, and on sound that saturates now and then they mispredict; GCC
  had used conditional moves. `std::hint::select_unpredictable` did not help (the clamps are
  recognized as min/max first and the hint is lost). The predictor is now saturated with mask
  arithmetic (`saturate()`), and the next step index comes from a table built at compile time.
- Holding the step index to the table inside the loop (`min(88)`) put a compare and a conditional
  move on the path from one code's index to the next. The entry points now check the incoming state
  once (`State::checked()`); in the loop the index is only bounds-checked, which is a branch that
  never fires and so costs nothing on that path.
- The two-call Westwood bridge first cost 6%: the sizing pass branched on each command's kind, which
  random commands mispredict, and the decode then paid the same again. `decoded_len` now reads each
  command's payload and sample counts from a 256-entry table.
