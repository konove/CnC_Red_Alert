# Plan: Which engine code moves to Rust next

Written 2026-10-09, on the tree at aa739494. Answers T-210.

**Status: in progress.** Each phase below gets its own task-level plan (`docs/RUST_<NAME>_PLAN.md`,
in the form of `docs/RUST_LCW_PLAN.md`) before its code is written.

**Goal:** Move the remaining byte- and pixel-crunching kernels of `src/engine` into the `engine_rs`
crate, one kernel per phase, without changing a byte of any kernel's output or any C++ caller.

**Architecture:** Unchanged from the LCW migration. The one staticlib crate in `src/rust/` gains a
module per kernel (`#![forbid(unsafe_code)]` each, except where a phase says otherwise), `lib.rs`
gains the bridge functions, and the C++ entry point keeps its signature and becomes a shim that
converts spans with `engine/codec/rust_bytes.h`.

## What makes a kernel worth moving

LCW fitted because of five properties. A candidate is ranked by how many it has:

1. Its inputs and outputs are byte or integer slices and plain integers: no engine class crosses the
   bridge.
2. It is a leaf: it calls nothing in the engine.
3. It has few callers, all through one or two C++ functions that can stay as shims.
4. Tests of the public C++ API already exist, so the Rust version is held to them unchanged.
5. Its inner loop is hot, or its C++ is index arithmetic the bounds checks slow down
   (`cmake/OptimizeInDebug.cmake` lists the measured ones).

Code that fails 1 to 3 is not a candidate however old it is: cxx would bridge every call and nothing
would get simpler.

## Candidates

Lines are the `.cc` without its tests.

### Tier 1: the same shape as LCW

| Kernel                                                                   | Lines | Callers                                                         | Existing tests                                          |
| ------------------------------------------------------------------------ | ----- | --------------------------------------------------------------- | ------------------------------------------------------- |
| `engine/codec/xor_delta.cc`: `ApplyXorDelta`, `ApplyXorDeltaToView`      | 135   | `gfx/keyframe.cc`, `gfx/wsa_animation.cc`                       | `xor_delta_test`, `keyframe_test`, `wsa_animation_test` |
| `engine/codec/aud_decoder.cc`: `DecodeAdpcmBlock`, `DecodeWestwoodBlock` | 132   | `audio/audio_mixer.cc`, `video/vqa/movie_loader.cc`             | `aud_decoder_test`                                      |
| `engine/video/vqa/adpcm_decoders.cc`: `ImaAdpcmDecoder::Decode`          | 74    | `video/vqa/movie_loader.cc`                                     | `adpcm_decoders_test`                                   |
| `engine/video/vqa/vq_decoder.cc`: `DecodeVqFrame`                        | 85    | `video/vqa/movie_drawer.cc`                                     | `vq_decoder_test`, `movie_drawer_test`                  |
| `engine/crypto/sha1_compress.cc`: `Sha1Compress` and its two variants    | 226   | `crypto/sha.cc`                                                 | `sha1_compress_test`                                    |
| `engine/crypto/blowfish.cc`: `BlowfishEngine`                            | 656   | `pk_sink`, `pk_source`, `file/mix_archive.cc`, RA `saveload.cc` | `blowfish_test`, `blowfish_stream_test`                 |
| `engine/codec/lzw.cc`: `LZWEngine`                                       | 290   | `codec/compressed_block.cc` (LZW12/LZW14 blocks)                | `compressed_block_test`, `codec_state_test`             |
| `engine/codec/base64.cc`: `Base64_Encode`, `Base64_Decode`               | 445   | `file/ini_file.cc`, `ra/wol/rawolapi.cc`, `tools/game_data.cc`  | `stream_golden_test`, `codec_corrupt_test`              |

### Tier 2: worth it, but each needs preparation

| Code                                                                                                   | Lines        | What has to happen first                                                                                                                                                         |
| ------------------------------------------------------------------------------------------------------ | ------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `engine/crypto/mp.cc` + `pk.cc`, the RSA-style bignum                                                  | 3,014 + 389  | Decide the cut. Bridging 60-odd `XMP_*` functions is pointless; the bridge is `PKey`'s encrypt and decrypt of a block with a key, and `mp.cc` is deleted. 24% of Debug load CPU. |
| `third_party/lzo` (`lzo1x_c`, `lzo1x_d`)                                                               | 711          | Choose between a crate and a port. Decoding must stay byte-compatible with existing RA saves; encoding only has to round-trip.                                                   |
| `engine/gfx/fading_table.cc`: `BuildFadingTable`                                                       | 79           | Takes `Palette`; the shim passes its 768 bytes as a slice. 15% of Debug load CPU.                                                                                                |
| `engine/gfx/shape_blitter.cc`, and `BlitToLocked`/`ScaleLocked`/the glyph loop in `pixel_buffer.cc`    | 258 + ~500   | Split each into a clip step that stays in C++ and a kernel over `(pixels, stride, rect)`. Per-frame code, so measure the bridge call before committing to it.                    |
| Format parsers: `gfx/pcx_file.cc`, `cps_image.cc`, `ico_image.cc`, the header of `file/mix_archive.cc` | 319, 62, 184 | Separate the parse of a byte span from the file reading around it. Each then gets a `cargo fuzz` target cheaply.                                                                 |

### Not candidates

- `engine/stream` and every sink/source adapter, `engine/file` (`GameFile`, `SearchPaths`,
  `ini_file.cc` with 77 includers), `engine/net`, `engine/window`, `engine/audio/audio_mixer.cc`:
  virtual C++ interfaces, SDL or sockets, or a caller fan-out the shim cannot hide.
- `src/game`, `src/ra`, `src/td`: a virtual class hierarchy over pooled raw pointers and globals.
  There is no boundary to cut at.
- `base/fixed.h`, `base/strings`, `base/numeric.h`: vocabulary inlined into dozens of translation
  units. A bridge call per operation would be slower than the inline code.
- `base/random.cc`, `engine/crypto/key_phrase_hash.cc`, `random_source.cc`: small and cold, and
  `RandomClass` must stay bit-identical for multiplayer sync. Cheaper left alone.

## Global constraints

These hold for every phase; the first four are carried over from `docs/RUST_LCW_PLAN.md`.

- **Output is byte-identical.** The existing C++ tests pass unchanged, and each phase adds a
  differential test that feeds the same inputs (the test corpus plus seeded random and truncated
  streams) to the old C++ kernel, kept as a test-only reference, and to the Rust one.
- **No C++ caller changes.** Signatures, error returns and the lenient or strict behavior on bad
  input stay as the header documents them.
- **One buffer never crosses the bridge as both `&[u8]` and `&mut [u8]`.** Each phase's plan lists
  the callers that work in place and gives them a single-buffer entry point; the shim routes with
  `Overlaps()` and `CHECK`-fails on layouts no caller uses. Known in-place callers:
  `ImaAdpcmDecoder::Decode` ("source and dest may overlap, with source at the end"),
  `BlowfishEngine::Encrypt`/`Decrypt` ("pass the same buffer as both arguments"). `xor_delta` and
  `vq_decoder` are to be checked.
- **A panic never unwinds into C++.** `panic = "abort"` is already set; kernels return on bad input
  as the C++ did rather than panic.
- **Benchmark before and after** on `RelWithDebInfo`, never `Debug`. A phase that is slower than the
  C++ it replaces is not merged.
- **State stays where the simplest bridge puts it.** A small POD (`AdpcmState`, `Sha1State`) is
  passed in and returned. A large keyed state (Blowfish's 4,168 bytes of tables) is a Rust type C++
  holds as `rust::Box`.

## Phases

Ordered by value over effort. Phases 1 to 3 complete a path that is already half in Rust.

### Phase 1: `xor_delta`

Every shape and WSA frame is an LCW decode followed by an XOR delta; after this phase the whole
decode is Rust. Two functions, two callers, three test files already covering them. Effort: s.

- [x] Task-level plan `docs/RUST_XOR_DELTA_PLAN.md`
- [x] `src/rust/src/xor_delta.rs` with the crate's own tests
- [x] Bridge functions and shims in `engine/codec/xor_delta.cc`
- [x] Differential test against the C++ reference
- [x] `tools/ui_check.py` for both games (shapes and WSA are on every screen)

### Phase 2: the audio decoders

`aud_decoder.cc` (IMA ADPCM and Westwood 8-bit, used by the mixer and the movies) and
`vqa/adpcm_decoders.cc`. The two IMA decoders differ in state width and output form; the plan
decides whether one Rust function serves both shims. Effort: s.

- [x] Task-level plan
- [x] `src/rust/src/adpcm.rs`, `westwood_audio.rs`
- [x] Shims; an in-place entry point for `ImaAdpcmDecoder::Decode`
- [x] Differential test
- [ ] Listen to one sound and one movie on a real device

### Phase 3: `vq_decoder`

With LCW and Phase 2 this makes the VQA decode path Rust end to end. Effort: s.

- [x] Task-level plan
- [x] `src/rust/src/vq.rs`, shim in `vq_decoder.cc`
- [x] Differential test; frame dumps of `REDINTRO` and `SOVIET1` identical before and after

### Phase 4: SHA-1 compression

`SHAEngine` keeps padding and partial blocks; only `Sha1Compress` and its portable and hardware
variants move. The hardware path needs `unsafe` for the `core::arch` SHA intrinsics behind
`is_x86_feature_detected!`, so this module is the exception to `forbid(unsafe_code)`: the `unsafe`
is confined to one function. Effort: s.

- [ ] Task-level plan, including whether a vetted crate (`sha1`) replaces a port
- [ ] `src/rust/src/sha1.rs`, shims in `sha1_compress.cc`
- [ ] `sha1_compress_test` unchanged; benchmark both paths

### Phase 5: Blowfish

The first kernel with keyed state. `BlowfishEngine` becomes a wrapper around a `rust::Box` of an
opaque Rust type; `Submit_Key`, `Encrypt` and `Decrypt` forward. Completes the MIX-open path with
Phase 4. Effort: m.

- [ ] Task-level plan: the opaque type, the in-place entry point, the unkeyed copy-through behavior
- [ ] `src/rust/src/blowfish.rs`, `BlowfishEngine` as a wrapper
- [ ] `blowfish_test`, `blowfish_stream_test`, `mix_archive_test` unchanged
- [ ] Both games start from the real MIX files; `tools/ra_saveload_smoke.sh`

### Phase 6: the bignum and `PKey`

The largest piece and its own project. The bridge is two or three functions (encrypt a block,
decrypt a block, key from its encoded form); `mp.cc`, `int.h` and most of `pk.cc` are deleted.
Effort: l.

- [ ] Task-level plan: the cut, and a crate (`num-bigint`, `crypto-bigint`) against a port
- [ ] Implementation, with `mp_test`'s vectors moved to the level that survives
- [ ] `pk_stream_test`, `mix_archive_test` unchanged; both games open their encrypted MIX files

### Later, in no fixed order

Each is picked up when something else touches the code, not for its own sake: `lzw.cc`, `base64.cc`,
the vendored LZO, `fading_table.cc`, the format parsers, and the blit kernels once `PixelView`'s
loops are split into clip and kernel.

## Open questions

- **Crates or ports.** SHA-1, Blowfish, base64, LZO and the bignum all have maintained crates. A
  crate is less code to own; a port is the point of a learning project and keeps the dependency list
  at `cxx` alone. Decide per phase, starting with Phase 4.
- **Where a blit kernel's bridge call stops paying.** Phases 1 to 5 are called per frame, block or
  file. A blit runs per object per frame; measure one before planning the pixel loops.
- ~~Whether `vq_decoder` and `xor_delta` have in-place callers.~~ Neither has
  (`docs/RUST_XOR_DELTA_PLAN.md`, `docs/RUST_VQ_DECODER_PLAN.md`).
