# Plan: LCW in Rust, through cxx and Corrosion

Written 2026-09-30, on the tree at fa329aeb.

**Status: done** (2026-09-30, 20208375..HEAD of branch `rust-lcw`); the real-display check of Task 5
step 2 is pending.

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development
> (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use
> checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the three C++ LCW kernels in `engine/codec` with one Rust implementation, linked
through a `cxx` bridge that Corrosion builds, without changing a byte of any kernel's output or any
C++ caller.

**Architecture:** A single Rust staticlib crate, `engine_rs` in `src/rust/`, holds the kernels as
safe Rust (`#![forbid(unsafe_code)]` per kernel module). Corrosion imports it into CMake, and
`corrosion_add_cxxbridge()` generates `engine_rs_cxx`, the C++ side of the bridge. The C++ entry
points (`LCW_Uncompress()`, `LcwUncompBounded()`, `LCW_Comp()`) keep their signatures and become
thin shims, so no caller changes. The C++ kernels survive as a test-only reference that a
differential test holds the Rust ones to.

**Tech Stack:** Rust 1.98.1 (edition 2024), cxx 1.0.202, Corrosion v0.6.1, CMake + Ninja, Google
Test, cargo test, clippy, rustfmt.

**Spec:** this document. The request was "LCW in Rust via cxx and Corrosion"; the Findings below are
the facts about the tree it has to fit.

## Findings

### The three kernels

| Entry point                               | File                                | Behavior on bad input                                                           | Callers                                                                                                                                                                                                                                                           |
| ----------------------------------------- | ----------------------------------- | ------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `LCW_Uncompress(source, dest) -> int32_t` | `engine/codec/lcw_uncompress.cc:47` | Lenient: stops at the first malformed command, returns the bytes written so far | `engine/gfx/keyframe.cc:144,169` (shapes), `engine/gfx/wsa_animation.cc:195,299` (WSA), `engine/video/vqa/lcw_buffer.cc:61` (VQA), `engine/codec/compressed_block.cc:38` (CPS, via `ra/jshell.cc:233`, `td/jshell.cc:266`), `engine/window/hardware_cursor.cc:52` |
| `LcwUncompBounded(source, dest) -> int`   | `engine/codec/lcw.cc:51`            | Strict: returns -1 for any malformed stream, including a missing end marker     | `engine/codec/block_backends.cc:66` (`LcwBackend`, the map and overlay packs)                                                                                                                                                                                     |
| `LCW_Comp(in, out) -> int`                | `engine/codec/lcw.cc:200`           | Returns -1 if `out` is smaller than `LcwWorstCaseSize(in.size())`               | `engine/codec/block_backends.cc:61` (`LcwBackend`)                                                                                                                                                                                                                |

`LcwWorstCaseSize()` (`engine/codec/lcw.h:56`) is `constexpr` and stays in C++;
`LcwBackend::Capacity()` uses it at compile time.

Existing tests: `engine/codec/lcw_uncompress_test.cc` (lenient decoder, 8 tests),
`engine/codec/lcw_comp_test.cc` (encoder round trips through the strict decoder and the block codec,
12 tests), `engine/codec/codec_corrupt_test.cc` and `stream_golden_test.cc` (block codec, including
LCW). All of them stay and must keep passing unchanged: they test the public C++ API, which is
exactly what must not change.

### Three callers decode in place

The source span lies inside the destination's own storage:

- `WsaAnimation` (`engine/gfx/wsa_animation.cc:183-195`, `:290-299`) reads each compressed frame
  into the tail of `delta_buffer_` and decodes to its front.
- `LcwBuffer::Decompress()` (`engine/video/vqa/lcw_buffer.cc:58-61`) decodes `bytes_[offset_..]`
  into `bytes_`.
- `Load_Uncompress()` (`ra/jshell.cc:221`, `td/jshell.cc` likewise) puts the packet at the tail of
  the buffer when scratch and destination are the same buffer; `UncompressBlock()` then passes
  `dest.first(uncompressed_bytes)` and the payload, which may or may not overlap that prefix.
- `lcw_uncompress_test.cc`'s `SupportsInPlaceLiteralCopy` decodes with the source starting at the
  destination's first byte.

The lenient decoder reads each command after writing the previous one, so in place it sees its own
output where the two overlap; the encoder's streams rely on that being harmless. **In Rust, one
buffer seen as both `&[u8]` and `&mut [u8]` is undefined behavior**, so the bridge needs a separate
in-place entry point taking the whole buffer once, and the C++ shim must route overlapping spans to
it. No caller passes a source that starts _before_ the destination and overlaps it; the shim
`CHECK`-fails on that layout instead of supporting it. The strict decoder and the encoder are only
ever called on separate buffers (`BlockCodec` owns two vectors); their shims `CHECK` that too.

### Build facts that shape the integration

- No Rust toolchain is installed on this machine (`cargo`, `rustc`, `rustup` all missing).
- Every Rust staticlib embeds its own copy of `std`; two in one binary fail to link with duplicate
  symbols. Hence one crate for the whole tree, with LCW as its first module.
- `STRICT_CHECKS` runs clang-tidy on every compiled C++ file and `-Weverything` on every TU. The
  generated bridge `.cc` must be exempt from both, and the generated headers must be `SYSTEM` so
  their warnings never reach a project TU (and so `clang-tidy-cache` keeps seeing silent
  preprocesses; see CLAUDE.md).
- `tools/check_layers.py` walks only `src/base` and `src/engine` for `.h`/`.cc` files. A crate in
  `src/rust/` is outside both, so its `target/` directory (which holds generated `cxx.h` files after
  a manual `cargo build`) can never trip `folder_errors()`.
- `optimize_in_debug(lcw.cc lcw_uncompress.cc)` keeps LCW at `-O2` in Debug. Cargo's equivalent is
  `opt-level = 2` in `[profile.dev]`.
- Corrosion's cargo step is a custom target that always runs; cargo decides staleness itself (about
  0.2 s when nothing changed). After this plan, `cmake --build` prints a cargo line on every build
  and ninja never says `no work to do`. That is expected, not the rebuild loop CLAUDE.md describes.
- CI (`.github/workflows/cmake-multi-platform.yml`) builds on ubuntu gcc, ubuntu clang-20, macOS and
  Windows MSVC, and a separate `lint` job runs the strict build. Every job needs the Rust toolchain
  once the crate is linked. No CI job builds Emscripten; this plan does not verify it (see Task 5).
- Versions checked 2026-09-30: Rust stable 1.98.1, cxx 1.0.202, Corrosion v0.6.1.

## Global Constraints

- Rust toolchain: exactly `1.98.1`, pinned in `rust-toolchain.toml` at the repository root with the
  `clippy` and `rustfmt` components.
- Crate: `engine_rs`, `edition = "2024"`, `crate-type = ["staticlib"]`, `publish = false`, sole
  dependency `cxx = "1.0.202"`; `Cargo.lock` is committed and every cargo invocation passes
  `--locked`.
- Corrosion: `v0.6.1` via `FetchContent`, like abseil and googletest.
- `panic = "abort"` in both `[profile.dev]` and `[profile.release]`: a panic must never unwind into
  C++.
- Every kernel module starts with `#![forbid(unsafe_code)]`. Only the `cxx`-generated code in
  `lib.rs` contains `unsafe`.
- No kernel input may make Rust panic except a broken caller contract (the in-place ranges not
  fitting their buffer). A malformed stream returns, it never aborts.
- The Rust kernels produce exactly what the C++ kernels produce: the same return value, the same
  destination bytes (including the bytes they leave alone), and a byte-identical compressed stream.
- No C++ caller changes. The C++ headers `engine/codec/lcw.h` and `engine/codec/lcw_uncompress.h`
  keep every declaration as it is.
- C++ follows CLAUDE.md (Google style, Chromium-style includes, `base::ssize`, no C-style casts).
  The generated headers are the one exception to the include rule: they are included as
  `"engine_rs_cxx/lib.h"` and `"rust/cxx.h"`.
- Rust follows rustfmt defaults and clippy's default lints with `-D warnings`.
- Commits: one per task, messages in the style of `git log`, no `Co-Authored-By` or other trailers.

## Review Focus

1. **In-place decodes** (WSA, VQA `LcwBuffer`, `Load_Uncompress()`): overlapping spans must reach
   Rust as one buffer, and decode exactly as before. Tests: Task 1 `in_place_*` cargo tests; Task 3
   `InPlaceMatchesReference`; Task 4 `PublicApiDecodesInPlace`, `PublicApiDecodesTailLayoutDisjoint`
   and the death test for a source overlapping the front.
2. **Damaged or truncated streams** (a corrupt `.MIX` entry, a cut-off download): the game must keep
   the C++ behavior (a decoded prefix or -1), never abort. Tests: Task 3 `DamagedStreamsMatch*` and
   `RandomBytesMatchReference`, run on thousands of generated cases.
3. **Empty spans with null data** (`std::span<std::byte>{}`): Rust slices may not be null. Tests:
   Task 1 `empty_buffers`; Task 4 keeps `EmptyBuffersDoNotAccessStorage` passing through the shim.
4. **Inputs past 64 KiB** for the encoder: medium and long references switch off above position
   0xFFFF; the Rust encoder must make the same choices there. Tests: Task 2
   `round_trips_past_sixteen_bit_positions`; Task 3 `CompressMatchesReference` at 65 535, 65 536, 70
   000 and 200 000 bytes.
5. **Debug-build speed**: the Rust kernels replace two files that `optimize_in_debug()` compiled at
   `-O2`. A Debug build must still play the intro movie in real time. Check: Task 5, step 4.

---

## File Structure

| Path                                                 | Status        | Responsibility                                                                                |
| ---------------------------------------------------- | ------------- | --------------------------------------------------------------------------------------------- |
| `rust-toolchain.toml`                                | Create        | Pins Rust 1.98.1 with clippy and rustfmt for cargo, rustup and Corrosion                      |
| `.gitignore`                                         | Modify        | Ignores `src/rust/target/`                                                                    |
| `src/rust/Cargo.toml`, `src/rust/Cargo.lock`         | Create        | The `engine_rs` crate and its profiles                                                        |
| `src/rust/src/lib.rs`                                | Create        | Crate root: module list, the `#[cxx::bridge]`, and the FFI wrappers                           |
| `src/rust/src/lcw/mod.rs`                            | Create        | The LCW module's public functions                                                             |
| `src/rust/src/lcw/decode.rs`                         | Create        | Lenient decoder (separate and in place) and strict decoder, with their tests                  |
| `src/rust/src/lcw/encode.rs`                         | Create        | Encoder and `worst_case_size()`, with their tests                                             |
| `src/rust/CMakeLists.txt`                            | Create        | Corrosion import, the bridge target, cargo test, and clippy/rustfmt in strict builds          |
| `CMakeLists.txt`                                     | Modify        | Fetches Corrosion; adds `src/rust` before `src/engine`                                        |
| `src/engine/codec/rust_bytes.h`                      | Create        | `AsRustSlice()` and `Overlaps()`: byte spans as bridge slices                                 |
| `src/engine/codec/lcw_reference_test_util.h/.cc`     | Create        | The C++ kernels as they are today, test-only, in `namespace lcw_reference`                    |
| `src/engine/codec/lcw_differential_test.cc`          | Create        | Rust against the reference, byte for byte                                                     |
| `src/engine/codec/lcw.cc`, `lcw_uncompress.cc`       | Modify        | Become shims over the bridge                                                                  |
| `src/engine/codec/CMakeLists.txt`                    | Modify        | Links `engine_rs_cxx` and `absl::check`; adds the new test files; drops `optimize_in_debug()` |
| `src/tools/lcw_bench.cc`, `src/tools/CMakeLists.txt` | Create/Modify | Times Rust against the reference                                                              |
| `.github/workflows/cmake-multi-platform.yml`         | Modify        | Installs the toolchain in every job                                                           |
| `CLAUDE.md`, `TODO.md`, this document                | Modify        | Setup, architecture, Debug kernels, follow-ups, outcome                                       |

---

### Task 0: Commit this plan

- [ ] **Step 1: Commit the document on its own**

```bash
git add docs/RUST_LCW_PLAN.md
git commit -q -F - <<'EOF'
Plan moving the LCW kernels to Rust through cxx and Corrosion
EOF
```

---

### Task 1: Rust toolchain, crate and the two decoders

**Files:**

- Create: `rust-toolchain.toml`, `src/rust/Cargo.toml`, `src/rust/Cargo.lock` (generated),
  `src/rust/src/lib.rs`, `src/rust/src/lcw/mod.rs`, `src/rust/src/lcw/decode.rs`
- Modify: `.gitignore`

**Interfaces:**

- Consumes: nothing.
- Produces, in `engine_rs::lcw` (Rust):
  - `pub fn uncompress(source: &[u8], dest: &mut [u8]) -> usize`
  - `pub fn uncompress_in_place(buffer: &mut [u8], source_start: usize, source_len: usize, dest_len: usize) -> usize`
  - `pub fn uncompress_bounded(source: &[u8], dest: &mut [u8]) -> Option<usize>`

- [ ] **Step 1: Install the toolchain** (ask the user first: it writes `~/.rustup` and `~/.cargo`
      and adds `. "$HOME/.cargo/env"` to `~/.profile`, which CLion also needs to see)

```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain none
. "$HOME/.cargo/env"
```

- [ ] **Step 2: Pin it and create the crate**

`rust-toolchain.toml`:

```toml
# The Rust toolchain for src/rust. rustup installs it on first use; Corrosion
# builds with it (see src/rust/CMakeLists.txt).
[toolchain]
channel = "1.98.1"
components = ["clippy", "rustfmt"]
```

`src/rust/Cargo.toml`:

```toml
# engine_rs: the engine kernels written in Rust, linked into the C++ engine
# through cxx (src/lib.rs). One crate for the whole tree: each Rust staticlib
# carries its own copy of std, so two in one binary would clash at link time.
[package]
name = "engine_rs"
version = "0.1.0"
edition = "2024"
rust-version = "1.98"
publish = false

[lib]
crate-type = ["staticlib"]
# Doc tests need an rlib.
doctest = false

[dependencies]

# Debug builds keep the kernels fast, as optimize_in_debug() does for C++
# (cmake/OptimizeInDebug.cmake). A panic must not unwind into C++.
[profile.dev]
opt-level = 2
panic = "abort"

[profile.release]
debug = "line-tables-only"
panic = "abort"
```

`.gitignore`, append:

```
/src/rust/target
```

`src/rust/src/lib.rs`:

```rust
//! Engine kernels written in Rust. Task 3 adds the cxx bridge that the C++
//! engine calls them through.

pub mod lcw;
```

`src/rust/src/lcw/mod.rs`:

```rust
//! LCW, Westwood's LZ77-style compression, as used by shapes, key frames,
//! WSA animations, VQA movies, CPS pictures and the map and overlay packs.
//!
//! A stream is a sequence of commands, each an opcode byte and its operands:
//!
//! | Opcode              | Meaning                                                      |
//! | ------------------- | ------------------------------------------------------------ |
//! | `0cccdddd dddddddd` | copy `ccc + 3` bytes from `d` bytes back in the output       |
//! | `10cccccc`          | copy `c` literal bytes from the stream; `0x80` ends it       |
//! | `11cccccc pppp`     | copy `c + 3` bytes from output position `p` (`c` < 62)        |
//! | `0xfe cccc v`       | write `c` copies of byte `v`                                 |
//! | `0xff cccc pppp`    | copy `c` bytes from output position `p`                      |
//!
//! Multi-byte operands are little-endian 16-bit values. Copies run forward one
//! byte at a time, so a copy overlapping its own output repeats a pattern.

mod decode;

pub use decode::{uncompress, uncompress_bounded, uncompress_in_place};
```

- [ ] **Step 3: Write the failing decoder tests**

`src/rust/src/lcw/decode.rs` (tests first; the functions are stubs that return 0 / `None`, so the
tests compile and fail):

```rust
//! The two LCW decoders: the lenient one that pictures, shapes and movies
//! use, and the strict one behind the block codec.
#![forbid(unsafe_code)]

pub fn uncompress(_source: &[u8], _dest: &mut [u8]) -> usize {
    0
}

pub fn uncompress_in_place(
    _buffer: &mut [u8],
    _source_start: usize,
    _source_len: usize,
    _dest_len: usize,
) -> usize {
    0
}

pub fn uncompress_bounded(_source: &[u8], _dest: &mut [u8]) -> Option<usize> {
    None
}

#[cfg(test)]
mod tests {
    use super::*;

    // The C++ cases of engine/codec/lcw_uncompress_test.cc, one for one.

    #[test]
    fn bounds_every_command() {
        // Each stream emits abc, then a command longer than the two bytes left.
        let streams: [&[u8]; 5] = [
            &[0x83, b'a', b'b', b'c', 0x83, b'a', b'b', b'c', 0x80],
            &[0x83, b'a', b'b', b'c', 0x00, 3, 0x80],
            &[0x83, b'a', b'b', b'c', 0xc0, 0, 0, 0x80],
            &[0x83, b'a', b'b', b'c', 0xff, 3, 0, 0, 0, 0x80],
            &[0x83, b'a', b'b', b'c', 0xfe, 3, 0, b'a', 0x80],
        ];
        for stream in streams {
            let mut output = [0xcc_u8; 10];
            assert_eq!(uncompress(stream, &mut output[1..6]), 5, "{stream:x?}");
            assert_eq!(output[0], 0xcc);
            assert_eq!(&output[1..5], b"abca");
            assert!(output[6..].iter().all(|&byte| byte == 0xcc));
        }
    }

    #[test]
    fn rejects_references_outside_decoded_prefix() {
        let streams: [&[u8]; 6] = [
            &[0x81, b'a', 0x00, 0, 0x80],
            &[0x81, b'a', 0x00, 2, 0x80],
            &[0x81, b'a', 0xc0, 1, 0, 0x80],
            &[0x81, b'a', 0xc0, 0xff, 0xff, 0x80],
            &[0x81, b'a', 0xff, 3, 0, 1, 0, 0x80],
            &[0x81, b'a', 0xff, 3, 0, 0xff, 0xff, 0x80],
        ];
        for stream in streams {
            let mut output = [0xcc_u8; 8];
            assert_eq!(uncompress(stream, &mut output), 1, "{stream:x?}");
            assert_eq!(output[0], b'a');
            assert!(output[1..].iter().all(|&byte| byte == 0xcc));
        }
    }

    #[test]
    fn preserves_overlapping_back_references() {
        let streams: [&[u8]; 3] = [
            &[0x81, b'a', 0x20, 1, 0x80],
            &[0x81, b'a', 0xc2, 0, 0, 0x80],
            &[0x81, b'a', 0xff, 5, 0, 0, 0, 0x80],
        ];
        for stream in streams {
            let mut output = [0_u8; 7];
            assert_eq!(uncompress(stream, &mut output), 6, "{stream:x?}");
            assert_eq!(&output[..6], b"aaaaaa");
            assert_eq!(output[6], 0);
        }
    }

    #[test]
    fn accepts_empty_commands_and_end_marker() {
        let stream = [0xff, 0, 0, 0xff, 0xff, 0xfe, 0, 0, b'x', 0x80, 0];
        let mut output = [0xcc_u8; 1];
        assert_eq!(uncompress(&stream, &mut output), 0);
        assert_eq!(output[0], 0xcc);
    }

    #[test]
    fn empty_buffers() {
        assert_eq!(uncompress(&[], &mut []), 0);
        assert_eq!(uncompress_bounded(&[], &mut []), None);
        assert_eq!(uncompress_bounded(&[0x80], &mut []), Some(0));
    }

    #[test]
    fn stops_at_capacity_without_reading_next_command() {
        let mut output = [0_u8; 1];
        assert_eq!(uncompress(&[0x81, b'a'], &mut output), 1);
        assert_eq!(output[0], b'a');
    }

    #[test]
    fn truncated_commands_keep_decoded_prefix() {
        let streams: [&[u8]; 4] = [
            &[0x81, b'a', 0x00],
            &[0x81, b'a', 0xfe, 3],
            &[0x81, b'a', 0xff, 3, 0],
            &[0x81, b'a', 0xc0, 0],
        ];
        for stream in streams {
            let mut output = [0_u8; 8];
            assert_eq!(uncompress(stream, &mut output), 1, "{stream:x?}");
            assert_eq!(output[0], b'a');
            assert_eq!(output[1], 0);
        }
    }

    #[test]
    fn in_place_literal_copy() {
        // The stream starts at the destination's first byte.
        let mut buffer = [0x83, b'a', b'b', b'c', 0x80];
        assert_eq!(uncompress_in_place(&mut buffer, 0, 5, 3), 3);
        assert_eq!(&buffer[..3], b"abc");
    }

    #[test]
    fn in_place_stream_at_the_tail() {
        // WSA's layout: the stream fills the tail, the output grows from the
        // front and finally overwrites the stream's own end marker, which was
        // read before the copy that overwrites it.
        let mut buffer = [0_u8; 12];
        buffer[5..].copy_from_slice(&[0x83, b'a', b'b', b'c', 0x60, 3, 0x80]);
        assert_eq!(uncompress_in_place(&mut buffer, 5, 7, 12), 12);
        assert_eq!(&buffer, b"abcabcabcabc");
    }

    #[test]
    #[should_panic]
    fn in_place_rejects_ranges_outside_the_buffer() {
        let mut buffer = [0x80_u8; 4];
        uncompress_in_place(&mut buffer, 2, 3, 4);
    }

    #[test]
    fn bounded_decodes_every_command() {
        let cases: [(&[u8], &[u8]); 5] = [
            (&[0x83, b'a', b'b', b'c', 0x60, 3, 0x80], b"abcabcabcabc"),
            (&[0xfe, 5, 0, b'x', 0x80], b"xxxxx"),
            (&[0x81, b'a', 0xff, 5, 0, 0, 0, 0x80], b"aaaaaa"),
            (&[0x81, b'a', 0xc2, 0, 0, 0x80], b"aaaaaa"),
            (&[0x81, b'a', 0x20, 1, 0x80], b"aaaaaa"),
        ];
        for (stream, plain) in cases {
            let mut output = vec![0xcc_u8; plain.len() + 2];
            assert_eq!(uncompress_bounded(stream, &mut output), Some(plain.len()));
            assert_eq!(&output[..plain.len()], plain);
            assert_eq!(&output[plain.len()..], &[0xcc, 0xcc]);
        }
    }

    #[test]
    fn bounded_rejects_malformed_streams() {
        let streams: [&[u8]; 9] = [
            &[0x81, b'a'],                         // no end marker
            &[0x81, b'a', 0x00, 0, 0x80],          // short copy from zero bytes back
            &[0x81, b'a', 0x00, 2, 0x80],          // short copy before the start
            &[0x81, b'a', 0xc0, 1, 0, 0x80],       // medium copy from the write position
            &[0x81, b'a', 0xff, 0, 0, 1, 0, 0x80], // empty long copy, but from ahead
            &[0x84, b'a', b'b'],                   // literal longer than the stream
            &[0x81, b'a', 0xfe, 3],                // truncated fill
            &[0x81, b'a', 0xff, 3, 0, 0],          // truncated long copy
            &[0x81, b'a', 0xc0],                   // truncated medium copy
        ];
        for stream in streams {
            let mut output = [0_u8; 16];
            assert_eq!(uncompress_bounded(stream, &mut output), None, "{stream:x?}");
        }
    }

    #[test]
    fn bounded_rejects_output_overruns() {
        let streams: [&[u8]; 4] = [
            &[0x83, b'a', b'b', b'c', 0x80],
            &[0xfe, 3, 0, b'x', 0x80],
            &[0x81, b'a', 0x10, 1, 0x80],
            &[0x81, b'a', 0xff, 3, 0, 0, 0, 0x80],
        ];
        for stream in streams {
            let mut output = [0_u8; 2];
            assert_eq!(uncompress_bounded(stream, &mut output), None, "{stream:x?}");
        }
    }
}
```

- [ ] **Step 4: Run the tests to verify they fail**

Run: `cargo test --manifest-path src/rust/Cargo.toml` Expected: compiles (the first build writes
`src/rust/Cargo.lock`); most tests FAIL with assertion mismatches;
`in_place_rejects_ranges_outside_the_buffer` FAILS with "test did not panic".

- [ ] **Step 5: Implement the decoders**

Replace the three stubs in `decode.rs` (keep the tests module) with:

```rust
/// Where the lenient decoder reads its stream and writes its output.
trait Buffers {
    fn source_len(&self) -> usize;
    fn source(&self, index: usize) -> u8;
    fn dest_len(&self) -> usize;
    fn dest(&self, index: usize) -> u8;
    fn set_dest(&mut self, index: usize, value: u8);
}

/// A stream and an output in separate storage.
struct Separate<'a> {
    source: &'a [u8],
    dest: &'a mut [u8],
}

impl Buffers for Separate<'_> {
    fn source_len(&self) -> usize {
        self.source.len()
    }
    fn source(&self, index: usize) -> u8 {
        self.source[index]
    }
    fn dest_len(&self) -> usize {
        self.dest.len()
    }
    fn dest(&self, index: usize) -> u8 {
        self.dest[index]
    }
    fn set_dest(&mut self, index: usize, value: u8) {
        self.dest[index] = value;
    }
}

/// One buffer holding the output at its front and the stream at
/// `source_start`. Reads see earlier writes, as the C++ decoder's did.
struct InPlace<'a> {
    buffer: &'a mut [u8],
    source_start: usize,
    source_len: usize,
    dest_len: usize,
}

impl Buffers for InPlace<'_> {
    fn source_len(&self) -> usize {
        self.source_len
    }
    fn source(&self, index: usize) -> u8 {
        self.buffer[self.source_start + index]
    }
    fn dest_len(&self) -> usize {
        self.dest_len
    }
    fn dest(&self, index: usize) -> u8 {
        self.buffer[index]
    }
    fn set_dest(&mut self, index: usize, value: u8) {
        self.buffer[index] = value;
    }
}

/// Decodes the LCW stream in `source` into `dest`. Stops at the end marker,
/// when `dest` is full, or at the first malformed command, and returns the
/// number of bytes written so far. Never panics.
pub fn uncompress(source: &[u8], dest: &mut [u8]) -> usize {
    decode_lenient(&mut Separate { source, dest })
}

/// As `uncompress`, for a stream stored inside its own output buffer:
/// `buffer[source_start..source_start + source_len]` is decoded into
/// `buffer[..dest_len]`. This is how WSA animations, VQA movies and CPS
/// pictures load; Rust may not see one buffer as both a shared and a mutable
/// slice, so it is passed once.
///
/// Panics if either range does not fit in `buffer`: that is a caller bug,
/// never a property of the stream.
pub fn uncompress_in_place(
    buffer: &mut [u8],
    source_start: usize,
    source_len: usize,
    dest_len: usize,
) -> usize {
    assert!(
        source_start
            .checked_add(source_len)
            .is_some_and(|end| end <= buffer.len()),
        "LCW stream outside its buffer"
    );
    assert!(dest_len <= buffer.len(), "LCW output outside its buffer");
    decode_lenient(&mut InPlace {
        buffer,
        source_start,
        source_len,
        dest_len,
    })
}

fn read_u16(buffers: &impl Buffers, index: usize) -> usize {
    usize::from(buffers.source(index)) | (usize::from(buffers.source(index + 1)) << 8)
}

// A port of the C++ LCW_Uncompress(), check for check: every bound is tested
// before the read or write it guards, so the indexing below cannot panic.
fn decode_lenient(buffers: &mut impl Buffers) -> usize {
    let source_len = buffers.source_len();
    let dest_len = buffers.dest_len();
    let mut read = 0;
    let mut written = 0;
    while written < dest_len && read < source_len {
        let opcode = buffers.source(read);
        read += 1;
        let (count, mut from) = if opcode & 0x80 == 0 {
            // Short copy, relative to the write position.
            if read == source_len {
                break;
            }
            let distance = usize::from(opcode & 0x0f) * 256 + usize::from(buffers.source(read));
            read += 1;
            if distance == 0 || distance > written {
                break;
            }
            (usize::from(opcode >> 4) + 3, written - distance)
        } else if opcode & 0x40 == 0 {
            if opcode == 0x80 {
                break; // The end marker.
            }
            // Literal bytes. Byte by byte, in order: in place, the stream
            // sits behind the output.
            let count = usize::from(opcode & 0x3f)
                .min(dest_len - written)
                .min(source_len - read);
            for i in 0..count {
                let value = buffers.source(read + i);
                buffers.set_dest(written + i, value);
            }
            read += count;
            written += count;
            continue;
        } else if opcode == 0xfe {
            // Long run of one byte value.
            if source_len - read < 3 {
                break;
            }
            let count = read_u16(buffers, read).min(dest_len - written);
            let value = buffers.source(read + 2);
            read += 3;
            for i in 0..count {
                buffers.set_dest(written + i, value);
            }
            written += count;
            continue;
        } else {
            // Long (0xff) or medium copy from an absolute output position.
            let count = if opcode == 0xff {
                if source_len - read < 2 {
                    break;
                }
                let count = read_u16(buffers, read);
                read += 2;
                count
            } else {
                usize::from(opcode & 0x3f) + 3
            };
            if source_len - read < 2 {
                break;
            }
            let from = read_u16(buffers, read);
            read += 2;
            // An empty copy never reads its position.
            if count != 0 && from >= written {
                break;
            }
            (count, from)
        };
        // Forward, byte by byte, so a copy overlapping its output repeats it.
        for _ in 0..count.min(dest_len - written) {
            let value = buffers.dest(from);
            buffers.set_dest(written, value);
            from += 1;
            written += 1;
        }
    }
    written
}

fn u16_at(bytes: &[u8], index: usize) -> usize {
    usize::from(u16::from_le_bytes([bytes[index], bytes[index + 1]]))
}

/// Copies `count` bytes forward from output position `from` to `written`.
/// Returns the new write position, or None if `from` is not behind `written`
/// or the copy would run past `dest`.
fn copy_back(dest: &mut [u8], written: usize, from: usize, count: usize) -> Option<usize> {
    if from >= written || count > dest.len() - written {
        return None;
    }
    for offset in 0..count {
        dest[written + offset] = dest[from + offset];
    }
    Some(written + count)
}

/// Decodes the LCW stream in `source` into `dest` and returns the number of
/// bytes written, or None if the stream is malformed: a command runs past
/// either buffer, a copy reads from at or beyond the write position, or the
/// end marker is missing. Never panics.
// A port of the C++ LcwUncompBounded(), check for check.
pub fn uncompress_bounded(source: &[u8], dest: &mut [u8]) -> Option<usize> {
    let mut read = 0;
    let mut written = 0;
    while read < source.len() {
        let opcode = source[read];
        read += 1;
        let remaining = source.len() - read;
        if opcode & 0x80 == 0 {
            // Short copy, relative to the write position.
            if remaining < 1 {
                return None;
            }
            let distance = usize::from(opcode & 0x0f) * 256 + usize::from(source[read]);
            read += 1;
            let from = written.checked_sub(distance)?;
            written = copy_back(dest, written, from, usize::from(opcode >> 4) + 3)?;
        } else if opcode & 0x40 == 0 {
            if opcode == 0x80 {
                return Some(written);
            }
            let count = usize::from(opcode & 0x3f);
            if count > remaining || count > dest.len() - written {
                return None;
            }
            dest[written..written + count].copy_from_slice(&source[read..read + count]);
            read += count;
            written += count;
        } else if opcode == 0xfe {
            if remaining < 3 {
                return None;
            }
            let count = u16_at(source, read);
            if count > dest.len() - written {
                return None;
            }
            dest[written..written + count].fill(source[read + 2]);
            read += 3;
            written += count;
        } else if opcode == 0xff {
            if remaining < 4 {
                return None;
            }
            let count = u16_at(source, read);
            let from = u16_at(source, read + 2);
            read += 4;
            written = copy_back(dest, written, from, count)?;
        } else {
            if remaining < 2 {
                return None;
            }
            let from = u16_at(source, read);
            read += 2;
            written = copy_back(dest, written, from, usize::from(opcode & 0x3f) + 3)?;
        }
    }
    None // The stream ran out before the end marker.
}
```

- [ ] **Step 6: Run the tests, clippy and rustfmt**

Run:

```bash
cargo test --manifest-path src/rust/Cargo.toml
cargo clippy --manifest-path src/rust/Cargo.toml --all-targets -- -D warnings
cargo fmt --manifest-path src/rust/Cargo.toml --check
```

Expected: 13 tests pass; clippy and fmt print nothing. If fmt reports a diff, run it without
`--check` and re-run the tests. If clippy suggests `copy_within` or `copy_from_slice` for the
back-reference loops (`manual_memcpy`), do not take it: those copy as `memmove` does, which breaks
the pattern repeat of a copy overlapping its own output. Put
`#[allow(clippy::manual_memcpy, reason = "a back-reference repeats its own output")]` on the
function instead.

- [ ] **Step 7: Commit**

```bash
git add rust-toolchain.toml .gitignore src/rust/Cargo.toml src/rust/Cargo.lock \
  src/rust/src/lib.rs src/rust/src/lcw/mod.rs src/rust/src/lcw/decode.rs
git commit -q -F - <<'EOF'
Port the two LCW decoders to Rust

The first module of engine_rs, a Rust crate in src/rust that the engine
will link through cxx. uncompress() and uncompress_bounded() are check
for check ports of LCW_Uncompress() and LcwUncompBounded(); a separate
uncompress_in_place() takes the whole buffer once for the WSA, VQA and
CPS loaders that decode a stream stored in its own output, since Rust
may not alias a shared and a mutable slice.

Nothing links the crate yet. The toolchain is pinned to 1.98.1.
EOF
```

---

### Task 2: The Rust encoder

**Files:**

- Create: `src/rust/src/lcw/encode.rs`
- Modify: `src/rust/src/lcw/mod.rs`

**Interfaces:**

- Consumes: `engine_rs::lcw::uncompress_bounded` (Task 1), in tests.
- Produces, in `engine_rs::lcw`:
  - `pub const fn worst_case_size(length: usize) -> usize`
  - `pub fn compress(input: &[u8], output: &mut [u8]) -> Option<usize>`

- [ ] **Step 1: Write the failing tests**

`src/rust/src/lcw/mod.rs`, replace the module list and `pub use` with:

```rust
mod decode;
mod encode;

pub use decode::{uncompress, uncompress_bounded, uncompress_in_place};
pub use encode::{compress, worst_case_size};
```

`src/rust/src/lcw/encode.rs`:

```rust
//! The LCW encoder: greedy, finding earlier occurrences through hash chains
//! over 3-byte prefixes.
#![forbid(unsafe_code)]

/// Returns the largest stream `compress` produces for `length` input bytes:
/// the input as literal runs of at most 63 bytes, each behind one opcode,
/// plus the end marker. Matches the C++ LcwWorstCaseSize().
pub const fn worst_case_size(_length: usize) -> usize {
    0
}

/// Compresses `input` into `output` as one LCW stream ending in the 0x80
/// marker. Returns the stream size, or None if `output` is shorter than
/// `worst_case_size(input.len())`.
pub fn compress(_input: &[u8], _output: &mut [u8]) -> Option<usize> {
    None
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::lcw::uncompress_bounded;

    // The data generators of engine/codec/lcw_comp_test.cc.

    /// Deterministic pseudo-random bytes; incompressible for LCW.
    fn random(size: usize, mut seed: u32) -> Vec<u8> {
        (0..size)
            .map(|_| {
                seed = seed.wrapping_mul(1_664_525).wrapping_add(1_013_904_223);
                (seed >> 24) as u8
            })
            .collect()
    }

    /// Runs of one value, lengths cycling from 1 to 300.
    fn runs(size: usize) -> Vec<u8> {
        let mut bytes = Vec::with_capacity(size);
        let mut run = 1;
        while bytes.len() < size {
            bytes.extend(std::iter::repeat_n((run % 251) as u8, run));
            run = run % 300 + 1;
        }
        bytes.truncate(size);
        bytes
    }

    /// A 37-byte pattern repeated, with every 97th byte changed.
    fn repeats(size: usize) -> Vec<u8> {
        (0..size)
            .map(|i| if i % 97 == 0 { 0xee } else { (i % 37) as u8 })
            .collect()
    }

    fn compress_all(input: &[u8]) -> Vec<u8> {
        let mut output = vec![0_u8; worst_case_size(input.len())];
        let size = compress(input, &mut output).expect("output sized by worst_case_size");
        output.truncate(size);
        output
    }

    fn assert_round_trip(plain: &[u8]) {
        let stream = compress_all(plain);
        assert!(stream.len() <= worst_case_size(plain.len()));
        let mut decoded = vec![0_u8; plain.len()];
        assert_eq!(uncompress_bounded(&stream, &mut decoded), Some(plain.len()));
        assert!(decoded == plain, "round trip changed {} bytes of input", plain.len());
    }

    #[test]
    fn worst_case_size_matches_the_cpp_formula() {
        for length in [0, 1, 62, 63, 64, 126, 127, 4096, 65_535] {
            assert_eq!(worst_case_size(length), length + (length + 62) / 63 + 1);
        }
    }

    #[test]
    fn empty_input_is_just_the_end_marker() {
        assert_eq!(compress_all(&[]), [0x80]);
    }

    #[test]
    fn single_byte_is_one_literal() {
        assert_eq!(compress_all(&[42]), [0x81, 42, 0x80]);
    }

    #[test]
    fn repeated_pattern_becomes_a_short_copy() {
        // Three literals, then nine bytes from three back: 0x60 = (9 - 3) << 4.
        assert_eq!(
            compress_all(b"abcabcabcabc"),
            [0x83, b'a', b'b', b'c', 0x60, 3, 0x80]
        );
    }

    #[test]
    fn long_run_becomes_a_fill() {
        // 0xfe, count 1000 = 0x03e8 little-endian, value 7.
        assert_eq!(compress_all(&[7; 1000]), [0xfe, 0xe8, 0x03, 7, 0x80]);
    }

    #[test]
    fn rejects_a_small_output() {
        let input = random(100, 1);
        let mut output = vec![0_u8; worst_case_size(input.len()) - 1];
        assert_eq!(compress(&input, &mut output), None);
    }

    #[test]
    fn round_trips() {
        assert_round_trip(&random(8192, 12_345));
        assert_round_trip(&runs(8192));
        assert_round_trip(&repeats(8192));
        assert_round_trip(&runs(65_535));
    }

    #[test]
    fn round_trips_past_sixteen_bit_positions() {
        let mut plain = repeats(70_000);
        plain.extend(runs(70_000));
        plain.extend(random(10_000, 99));
        assert_round_trip(&plain);
    }

    #[test]
    fn compresses_redundant_data() {
        let plain = repeats(8192);
        assert!(compress_all(&plain).len() < plain.len() / 4);
    }
}
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `cargo test --manifest-path src/rust/Cargo.toml encode` Expected: FAIL (`worst_case_size`
returns 0; `compress` returns None).

- [ ] **Step 3: Implement the encoder**

Replace the two stubs in `encode.rs` with:

```rust
const MAX_LITERAL_RUN: u8 = 63; // 10xxxxxx holds 1..63 bytes.
const MAX_SHORT_COUNT: usize = 10; // 0xxx: 3..10 bytes.
const MAX_SHORT_DISTANCE: usize = 0xfff; // 12-bit relative distance.
const MAX_MEDIUM_COUNT: usize = 64; // 11xxxxxx below 0xfe: 3..64.
const MAX_LONG_COUNT: usize = 0xffff; // 16-bit count for 0xfe and 0xff.
const MAX_ABSOLUTE_POSITION: usize = 0xffff;
const HASH_SIZE: usize = 1 << 12;
const MAX_CHAIN_STEPS: usize = 256;

pub const fn worst_case_size(length: usize) -> usize {
    length + length.div_ceil(MAX_LITERAL_RUN as usize) + 1
}

/// One way to encode the bytes at the current position.
#[derive(Clone, Copy)]
enum Kind {
    Literal,
    Short,
    Medium,
    Long,
    Fill,
}

#[derive(Clone, Copy)]
struct Choice {
    kind: Kind,
    count: usize,   // Input bytes covered.
    from: usize,    // Distance (short) or absolute position (medium, long).
    savings: isize, // Input bytes covered minus encoded bytes.
}

const LITERAL: Choice = Choice {
    kind: Kind::Literal,
    count: 1,
    from: 0,
    savings: 0,
};

/// Bytes covered minus bytes spent; counts never exceed 0xffff.
fn savings(count: usize, cost: usize) -> isize {
    count as isize - cost as isize
}

/// Returns the cheapest back-reference for a match of `length` bytes,
/// `distance` bytes back, at absolute `position`. Ties keep the earlier,
/// shorter encoding, as the C++ BestReference() does.
fn best_reference(length: usize, distance: usize, position: usize) -> Choice {
    let mut best = LITERAL;
    if distance <= MAX_SHORT_DISTANCE {
        let count = length.min(MAX_SHORT_COUNT);
        best = Choice {
            kind: Kind::Short,
            count,
            from: distance,
            savings: savings(count, 2),
        };
    }
    if position <= MAX_ABSOLUTE_POSITION {
        let medium = length.min(MAX_MEDIUM_COUNT);
        if savings(medium, 3) > best.savings {
            best = Choice {
                kind: Kind::Medium,
                count: medium,
                from: position,
                savings: savings(medium, 3),
            };
        }
        let long = length.min(MAX_LONG_COUNT);
        if savings(long, 5) > best.savings {
            best = Choice {
                kind: Kind::Long,
                count: long,
                from: position,
                savings: savings(long, 5),
            };
        }
    }
    best
}

fn hash_at(input: &[u8], pos: usize) -> usize {
    let hash = (u32::from(input[pos]) << 8)
        ^ (u32::from(input[pos + 1]) << 4)
        ^ u32::from(input[pos + 2]);
    hash as usize % HASH_SIZE
}

/// Earlier positions by their 3-byte prefix, most recent first.
struct Chains {
    head: Vec<Option<usize>>,
    previous: Vec<Option<usize>>,
}

impl Chains {
    fn new(length: usize) -> Self {
        Self {
            head: vec![None; HASH_SIZE],
            previous: vec![None; length],
        }
    }

    fn remember(&mut self, input: &[u8], pos: usize) {
        if pos + 2 < input.len() {
            let bucket = hash_at(input, pos);
            self.previous[pos] = self.head[bucket];
            self.head[bucket] = Some(pos);
        }
    }
}

/// The output stream being written.
struct Writer<'a> {
    output: &'a mut [u8],
    written: usize,
}

impl Writer<'_> {
    fn put(&mut self, value: u8) {
        self.output[self.written] = value;
        self.written += 1;
    }

    fn put_u16(&mut self, value: usize) {
        let [low, high] = u16::try_from(value)
            .expect("LCW operands are 16 bits")
            .to_le_bytes();
        self.put(low);
        self.put(high);
    }
}

// A port of the C++ LCW_Comp(): a back-reference or fill is used only when it
// saves at least one byte over literals, which also pays for any literal
// opcode it forces afterwards, so the output never exceeds worst_case_size().
pub fn compress(input: &[u8], output: &mut [u8]) -> Option<usize> {
    let length = input.len();
    if output.len() < worst_case_size(length) {
        return None;
    }
    let mut chains = Chains::new(length);
    let mut out = Writer { output, written: 0 };
    let mut literal_opcode: Option<usize> = None; // The open literal run's opcode.

    let mut pos = 0;
    while pos < length {
        let limit = (length - pos).min(MAX_LONG_COUNT);
        let mut best = LITERAL;

        let run = 1 + input[pos + 1..pos + limit]
            .iter()
            .take_while(|&&byte| byte == input[pos])
            .count();
        if savings(run, 4) > best.savings {
            best = Choice {
                kind: Kind::Fill,
                count: run,
                from: 0,
                savings: savings(run, 4),
            };
        }

        if pos + 2 < length {
            let mut candidate = chains.head[hash_at(input, pos)];
            let mut steps = 0;
            while let Some(start) = candidate {
                if steps == MAX_CHAIN_STEPS {
                    break;
                }
                // Overlapping matches are fine: the decoder copies forward.
                let matched = input[start..]
                    .iter()
                    .zip(&input[pos..pos + limit])
                    .take_while(|(earlier, current)| earlier == current)
                    .count();
                if matched >= 3 {
                    let reference = best_reference(matched, pos - start, start);
                    if reference.savings > best.savings {
                        best = reference;
                    }
                }
                candidate = chains.previous[start];
                steps += 1;
            }
        }

        if best.savings < 1 {
            let opcode = match literal_opcode {
                Some(at) if out.output[at] != 0x80 + MAX_LITERAL_RUN => at,
                _ => {
                    let at = out.written;
                    out.put(0x80);
                    literal_opcode = Some(at);
                    at
                }
            };
            out.output[opcode] += 1;
            out.put(input[pos]);
            chains.remember(input, pos);
            pos += 1;
            continue;
        }

        match best.kind {
            Kind::Short => {
                // Both fit: count - 3 <= 7 and from <= 0xfff.
                out.put(((best.count - 3) * 16 + best.from / 256) as u8);
                out.put((best.from % 256) as u8);
            }
            Kind::Medium => {
                out.put(0xc0 + (best.count - 3) as u8);
                out.put_u16(best.from);
            }
            Kind::Long => {
                out.put(0xff);
                out.put_u16(best.count);
                out.put_u16(best.from);
            }
            Kind::Fill => {
                out.put(0xfe);
                out.put_u16(best.count);
                out.put(input[pos]);
            }
            Kind::Literal => unreachable!("a literal saves nothing"),
        }
        for i in 0..best.count {
            chains.remember(input, pos + i);
        }
        pos += best.count;
        literal_opcode = None;
    }

    out.put(0x80);
    Some(out.written)
}
```

`unreachable!` is not an input-driven panic: `best.savings >= 1` excludes `Kind::Literal`, whose
savings are 0.

- [ ] **Step 4: Run the tests, clippy and rustfmt**

Run:

```bash
cargo test --manifest-path src/rust/Cargo.toml
cargo clippy --manifest-path src/rust/Cargo.toml --all-targets -- -D warnings
cargo fmt --manifest-path src/rust/Cargo.toml --check
```

Expected: 22 tests pass; clippy and fmt silent. If clippy flags the `as` casts in tests
(`cast_possible_truncation` is pedantic and off by default, so it should not), use
`u8::try_from(...).unwrap()` there.

- [ ] **Step 5: Commit**

```bash
git add src/rust/src/lcw/mod.rs src/rust/src/lcw/encode.rs
git commit -q -F - <<'EOF'
Port the LCW encoder to Rust

compress() follows LCW_Comp() choice for choice: the same hash chains
over 3-byte prefixes, the same 256-step limit, the same tie-breaking in
favor of the shorter encoding. Task 3's differential test holds it to a
byte-identical stream, so the map and overlay packs it writes do not
change.
EOF
```

---

### Task 3: Link the crate through cxx and Corrosion, and test it against C++

**Files:**

- Create: `src/rust/CMakeLists.txt`, `src/engine/codec/rust_bytes.h`,
  `src/engine/codec/lcw_reference_test_util.h`, `src/engine/codec/lcw_reference_test_util.cc`,
  `src/engine/codec/lcw_differential_test.cc`
- Modify: `src/rust/Cargo.toml`, `src/rust/Cargo.lock`, `src/rust/src/lib.rs`, `CMakeLists.txt`,
  `src/engine/codec/CMakeLists.txt`, `.github/workflows/cmake-multi-platform.yml`

**Interfaces:**

- Consumes: `engine_rs::lcw::{uncompress, uncompress_in_place, uncompress_bounded, compress}`.
- Produces:
  - C++ (generated, `#include "engine_rs_cxx/lib.h"`), namespace `engine_rs`:
    `std::size_t lcw_uncompress(rust::Slice<const std::uint8_t> source, rust::Slice<std::uint8_t> dest)`,
    `std::size_t lcw_uncompress_in_place(rust::Slice<std::uint8_t> buffer, std::size_t source_start, std::size_t source_len, std::size_t dest_len)`,
    `std::int32_t lcw_uncompress_bounded(rust::Slice<const std::uint8_t> source, rust::Slice<std::uint8_t> dest)`,
    `std::int32_t lcw_compress(rust::Slice<const std::uint8_t> input, rust::Slice<std::uint8_t> output)`.
  - `engine/codec/rust_bytes.h`:
    `rust::Slice<const uint8_t> AsRustSlice(std::span<const std::byte>)`,
    `rust::Slice<uint8_t> AsRustSlice(std::span<std::byte>)`,
    `bool Overlaps(std::span<const std::byte>, std::span<const std::byte>)`.
  - `engine/codec/lcw_reference_test_util.h`, namespace `lcw_reference`:
    `int32_t Uncompress(std::span<const std::byte>, std::span<std::byte>)`,
    `int UncompressBounded(std::span<const std::byte>, std::span<std::byte>)`,
    `int Compress(std::span<const std::byte>, std::span<std::byte>)`.
  - CMake targets `engine_rs` (the staticlib) and `engine_rs_cxx` (the bridge).

- [ ] **Step 1: Add the bridge to the crate**

`src/rust/Cargo.toml`, set the dependency:

```toml
[dependencies]
cxx = "1.0.202"
```

`src/rust/src/lib.rs`, replace the whole file:

```rust
//! Engine kernels written in Rust, and the cxx bridge the C++ engine calls
//! them through. CMake builds the C++ side of the bridge as engine_rs_cxx
//! (src/rust/CMakeLists.txt); C++ includes "engine_rs_cxx/lib.h".

pub mod lcw;

#[cxx::bridge(namespace = "engine_rs")]
mod ffi {
    extern "Rust" {
        /// Decodes an LCW stream leniently; see lcw::uncompress.
        fn lcw_uncompress(source: &[u8], dest: &mut [u8]) -> usize;

        /// Decodes an LCW stream stored in its own output buffer; see
        /// lcw::uncompress_in_place. Aborts if the ranges do not fit.
        fn lcw_uncompress_in_place(
            buffer: &mut [u8],
            source_start: usize,
            source_len: usize,
            dest_len: usize,
        ) -> usize;

        /// Decodes an LCW stream strictly; -1 if it is malformed.
        fn lcw_uncompress_bounded(source: &[u8], dest: &mut [u8]) -> i32;

        /// Compresses into one LCW stream; -1 if output is too small.
        fn lcw_compress(input: &[u8], output: &mut [u8]) -> i32;
    }
}

fn lcw_uncompress(source: &[u8], dest: &mut [u8]) -> usize {
    lcw::uncompress(source, dest)
}

fn lcw_uncompress_in_place(
    buffer: &mut [u8],
    source_start: usize,
    source_len: usize,
    dest_len: usize,
) -> usize {
    lcw::uncompress_in_place(buffer, source_start, source_len, dest_len)
}

/// A size as the C++ API's int result; -1 if it does not fit, which no LCW
/// buffer comes near.
fn size_or_minus_one(size: Option<usize>) -> i32 {
    size.and_then(|size| i32::try_from(size).ok()).unwrap_or(-1)
}

fn lcw_uncompress_bounded(source: &[u8], dest: &mut [u8]) -> i32 {
    size_or_minus_one(lcw::uncompress_bounded(source, dest))
}

fn lcw_compress(input: &[u8], output: &mut [u8]) -> i32 {
    size_or_minus_one(lcw::compress(input, output))
}
```

Run: `cargo test --manifest-path src/rust/Cargo.toml` (refreshes `Cargo.lock` with cxx; 22 pass)

- [ ] **Step 2: Fetch Corrosion and add the crate to CMake**

`CMakeLists.txt`, after the `magic_enum` block (`target_compile_definitions(magic_enum ...)`):

```cmake
# Corrosion: builds the Rust crate in src/rust with cargo and links it like
# any other library, with the C++ side of its cxx bridge as a target.
FetchContent_Declare(
        Corrosion
        GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
        GIT_TAG v0.6.1
)
FetchContent_MakeAvailable(Corrosion)
```

and in the `add_subdirectory` list, before `add_subdirectory(src/engine)`:

```cmake
add_subdirectory(src/rust)
```

`src/rust/CMakeLists.txt`:

```cmake
# engine_rs: the Rust kernels (Cargo.toml), and engine_rs_cxx, the C++ side of
# their cxx bridge (src/lib.rs). C++ links engine_rs_cxx and includes
# "engine_rs_cxx/lib.h" and "rust/cxx.h".

corrosion_import_crate(
        MANIFEST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml
        CRATES engine_rs
        CRATE_TYPES staticlib
        LOCKED
)

corrosion_add_cxxbridge(engine_rs_cxx
        CRATE engine_rs
        FILES lib.rs
)

# Generated code: the project's checks do not apply to it, and its headers are
# system headers to every includer, so -Weverything stays quiet and
# clang-tidy-cache keeps seeing silent preprocesses.
set_target_properties(engine_rs_cxx PROPERTIES
        CXX_CLANG_TIDY ""
        CXX_INCLUDE_WHAT_YOU_USE ""
        SYSTEM TRUE
)
target_compile_options(engine_rs_cxx PRIVATE $<IF:$<CXX_COMPILER_ID:MSVC>,/w,-w>)

set(ENGINE_RS_MANIFEST ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml)

# The crate's own tests, in a target directory of their own so they never wait
# on the lock of the build Corrosion runs.
add_test(NAME engine_rs_cargo_test
        COMMAND ${Rust_CARGO_CACHED} test --manifest-path ${ENGINE_RS_MANIFEST} --locked
            --target-dir ${CMAKE_CURRENT_BINARY_DIR}/cargo-test
)

# Strict builds hold the Rust code to clippy and rustfmt, as they hold C++ to
# clang-tidy. A stamp keeps it from re-running until a source changes.
if (STRICT_CHECKS)
    file(GLOB_RECURSE ENGINE_RS_SOURCES CONFIGURE_DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/src/*.rs)
    set(ENGINE_RS_LINT_STAMP ${CMAKE_CURRENT_BINARY_DIR}/lint.stamp)
    add_custom_command(
            OUTPUT ${ENGINE_RS_LINT_STAMP}
            COMMAND ${Rust_CARGO_CACHED} fmt --manifest-path ${ENGINE_RS_MANIFEST} --check
            COMMAND ${Rust_CARGO_CACHED} clippy --manifest-path ${ENGINE_RS_MANIFEST} --locked
                --all-targets --target-dir ${CMAKE_CURRENT_BINARY_DIR}/cargo-clippy
                -- -D warnings
            COMMAND ${CMAKE_COMMAND} -E touch ${ENGINE_RS_LINT_STAMP}
            DEPENDS ${ENGINE_RS_SOURCES} ${ENGINE_RS_MANIFEST}
                ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.lock
            COMMENT "rustfmt and clippy on src/rust"
            VERBATIM
    )
    add_custom_target(engine_rs_lint ALL DEPENDS ${ENGINE_RS_LINT_STAMP})
endif ()
```

- [ ] **Step 3: Configure and check what Corrosion found**

Run: `cmake -Bbuild -G Ninja 2>&1 | grep -iE "rust|corrosion|cxx"`

Expected: a `Rust Toolchain: 1.98.1-x86_64-unknown-linux-gnu` line (Corrosion prints the toolchain
it resolved), and `ctest --test-dir build -N | grep engine_rs` lists `engine_rs_cargo_test`. If
`ctest --test-dir build -R engine_rs_cargo -V` shows the command starting with `test` instead of a
path to cargo, the variable is not `Rust_CARGO_CACHED` in this Corrosion: look up the cargo variable
in `build/_deps/corrosion-src/cmake/FindRust.cmake` and use that name. If the toolchain line names
another toolchain, Corrosion did not pick up `rust-toolchain.toml`; add
`set(Rust_TOOLCHAIN "1.98.1-x86_64-unknown-linux-gnu" CACHE STRING "")` above the `FetchContent`
block, reconfigure, and record that in Findings. The first build also runs
`cargo install cxxbridge-cmd` at the version in `Cargo.lock` (network once, into the build dir).

Then find the generated header:

Run:
`cmake --build build --target engine_rs_cxx && find build -path '*engine_rs_cxx*' -name 'lib.h'`

Expected: one path ending in `include/engine_rs_cxx/lib.h`. If the directory layout differs, the
include below follows what is on disk (`#include "<dir under the include root>/lib.h"`); update this
plan's Interfaces to match.

- [ ] **Step 4: Add the byte-span helpers**

`src/engine/codec/rust_bytes.h`:

```cpp
// File: Byte spans as the slices the cxx bridge to src/rust takes.

#ifndef CNC_RED_ALERT_ENGINE_CODEC_RUST_BYTES_H_
#define CNC_RED_ALERT_ENGINE_CODEC_RUST_BYTES_H_

#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>

#include "rust/cxx.h"

// Returns `bytes` as a Rust slice. cxx gives an empty span's null data the
// dangling non-null pointer Rust slices require.
inline rust::Slice<const uint8_t> AsRustSlice(
    const std::span<const std::byte> bytes) {
  const void* const data = bytes.data();
  return {static_cast<const uint8_t*>(data), bytes.size()};
}
inline rust::Slice<uint8_t> AsRustSlice(const std::span<std::byte> bytes) {
  void* const data = bytes.data();
  return {static_cast<uint8_t*>(data), bytes.size()};
}

// Returns whether `a` and `b` share a byte. Rust may not see one byte through
// a shared and a mutable slice at once, so overlapping spans must not both
// cross the bridge.
inline bool Overlaps(const std::span<const std::byte> a,
                     const std::span<const std::byte> b) {
  if (a.empty() || b.empty()) {
    return false;
  }
  const auto a_begin = std::bit_cast<uintptr_t>(a.data());
  const auto b_begin = std::bit_cast<uintptr_t>(b.data());
  return a_begin < b_begin + b.size() && b_begin < a_begin + a.size();
}

#endif  // CNC_RED_ALERT_ENGINE_CODEC_RUST_BYTES_H_
```

- [ ] **Step 5: Copy the C++ kernels into a test-only reference**

`src/engine/codec/lcw_reference_test_util.h`:

```cpp
// File: The C++ LCW kernels as they were before src/rust replaced them, kept
// as the reference lcw_differential_test.cc holds the Rust ones to.

#ifndef CNC_RED_ALERT_ENGINE_CODEC_LCW_REFERENCE_TEST_UTIL_H_
#define CNC_RED_ALERT_ENGINE_CODEC_LCW_REFERENCE_TEST_UTIL_H_

#include <cstddef>
#include <cstdint>
#include <span>

namespace lcw_reference {

// The former LCW_Uncompress(): lenient, returns the bytes written. Works in
// place: `source` may lie in `dest`'s storage at or after its start.
int32_t Uncompress(std::span<const std::byte> source,
                   std::span<std::byte> dest);

// The former LcwUncompBounded(): returns the bytes written, or -1 for a
// malformed stream.
int UncompressBounded(std::span<const std::byte> source,
                      std::span<std::byte> dest);

// The former LCW_Comp(): returns the stream size, or -1 if `out` is smaller
// than LcwWorstCaseSize(in.size()).
int Compress(std::span<const std::byte> in, std::span<std::byte> out);

}  // namespace lcw_reference

#endif  // CNC_RED_ALERT_ENGINE_CODEC_LCW_REFERENCE_TEST_UTIL_H_
```

`src/engine/codec/lcw_reference_test_util.cc`: build it mechanically from the current sources, so
the reference is the shipped code and nothing else.

```bash
{
  printf '%s\n' '// The C++ LCW kernels as of fa329aeb, verbatim but for their names; see' \
                '// lcw_reference_test_util.h.' '' \
                '#include "engine/codec/lcw_reference_test_util.h"' ''
  sed -n '/^#include <algorithm>/,/^#include "base\/types.h"/p' src/engine/codec/lcw.cc
  printf '%s\n' '#include "engine/codec/lcw.h"' '' 'namespace lcw_reference {' ''
  sed -n '/^int32_t LCW_Uncompress(std::span<const std::byte>/,/^}/p' src/engine/codec/lcw_uncompress.cc
  echo
  sed -n '/^int LcwUncompBounded/,$p' src/engine/codec/lcw.cc
  printf '\n%s\n' '}  // namespace lcw_reference'
} > src/engine/codec/lcw_reference_test_util.cc
sed -i -e 's/^int32_t LCW_Uncompress(/int32_t Uncompress(/' \
       -e 's/^int LcwUncompBounded(/int UncompressBounded(/' \
       -e 's/^int LCW_Comp(/int Compress(/' src/engine/codec/lcw_reference_test_util.cc
```

Then open the file and check by hand: one `namespace lcw_reference {` around all three functions;
the copied anonymous namespace (the encoder's `Choice`, `BestReference`, `HashAt`) now sits inside
`lcw_reference`, which is fine; `LcwWorstCaseSize` still comes from `engine/codec/lcw.h`. Fix the
include list if `misc-include-cleaner` asks for it in step 8.

- [ ] **Step 6: Write the differential test**

`src/engine/codec/lcw_differential_test.cc`:

```cpp
// Holds the Rust LCW kernels in src/rust to the C++ ones they replace, byte
// for byte: the same result and the same destination contents for valid,
// damaged and random streams, and the same compressed stream.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "base/types.h"
#include "engine/codec/lcw.h"
#include "engine/codec/lcw_reference_test_util.h"
#include "engine/codec/rust_bytes.h"
#include "engine_rs_cxx/lib.h"
#include "gtest/gtest.h"

namespace {

constexpr std::byte kFill{0xcc};  // What untouched destination bytes hold.

// A deterministic generator, so every failure is a reproducible case.
class Lcg {
 public:
  explicit Lcg(const uint32_t seed) : state_(seed) {}

  uint32_t Next() {
    state_ = (state_ * 1664525U) + 1013904223U;
    return state_ >> 8;
  }
  // Returns a value in [0, bound); bound must be positive.
  int Below(const int bound) {
    return static_cast<int>(Next() % static_cast<uint32_t>(bound));
  }

 private:
  uint32_t state_;
};

std::vector<std::byte> RandomBytes(Lcg& lcg, const int size) {
  std::vector<std::byte> bytes(static_cast<std::size_t>(size));
  std::ranges::generate(
      bytes, [&] { return static_cast<std::byte>(lcg.Next() & 0xff); });
  return bytes;
}

// Game-like data: runs, repeated patterns and noise, so every command kind
// appears in its compressed form.
std::vector<std::byte> Corpus(Lcg& lcg, const int size) {
  std::vector<std::byte> bytes;
  while (std::ssize(bytes) < size) {
    const int piece = 1 + lcg.Below(300);
    switch (lcg.Below(3)) {
      case 0:
        bytes.insert(bytes.end(), static_cast<std::size_t>(piece),
                     static_cast<std::byte>(lcg.Next() & 0xff));
        break;
      case 1:
        if (!bytes.empty()) {
          const int from = lcg.Below(static_cast<int>(bytes.size()));
          for (int i = 0; i < piece; ++i) {
            bytes.push_back(bytes.at(static_cast<std::size_t>(from + i)));
          }
          break;
        }
        [[fallthrough]];
      default: {
        const auto noise = RandomBytes(lcg, piece);
        bytes.insert(bytes.end(), noise.begin(), noise.end());
        break;
      }
    }
  }
  bytes.resize(static_cast<std::size_t>(size));
  return bytes;
}

std::vector<std::byte> ReferenceCompress(const std::vector<std::byte>& plain) {
  std::vector<std::byte> stream(
      static_cast<std::size_t>(LcwWorstCaseSize(static_cast<int>(plain.size()))));
  const int size = lcw_reference::Compress(plain, stream);
  stream.resize(static_cast<std::size_t>(size));
  return stream;
}

// A valid stream with up to three bytes changed and, one time in four, its
// tail cut off.
std::vector<std::byte> Damage(Lcg& lcg, std::vector<std::byte> stream) {
  for (int flips = lcg.Below(4); flips > 0 && !stream.empty(); --flips) {
    const auto at = static_cast<std::size_t>(lcg.Below(static_cast<int>(stream.size())));
    stream.at(at) ^= static_cast<std::byte>(1 + lcg.Below(255));
  }
  if (lcg.Below(4) == 0 && !stream.empty()) {
    stream.resize(static_cast<std::size_t>(lcg.Below(static_cast<int>(stream.size()))));
  }
  return stream;
}

// Runs both lenient decoders on `stream` into `dest_size` bytes.
void ExpectLenientMatch(const std::vector<std::byte>& stream,
                        const int dest_size) {
  std::vector<std::byte> want(static_cast<std::size_t>(dest_size), kFill);
  std::vector<std::byte> got(want);
  const int32_t want_size = lcw_reference::Uncompress(stream, want);
  const auto got_size = engine_rs::lcw_uncompress(AsRustSlice(stream), AsRustSlice(std::span(got)));
  EXPECT_EQ(static_cast<int64_t>(got_size), int64_t{want_size});
  EXPECT_TRUE(got == want);
}

// Runs both strict decoders on `stream` into `dest_size` bytes.
void ExpectBoundedMatch(const std::vector<std::byte>& stream,
                        const int dest_size) {
  std::vector<std::byte> want(static_cast<std::size_t>(dest_size), kFill);
  std::vector<std::byte> got(want);
  const int want_size = lcw_reference::UncompressBounded(stream, want);
  const int got_size = engine_rs::lcw_uncompress_bounded(
      AsRustSlice(stream), AsRustSlice(std::span(got)));
  EXPECT_EQ(got_size, want_size);
  EXPECT_TRUE(got == want);
}

TEST(LcwDifferentialTest, CompressMatchesReference) {
  Lcg lcg(1);
  for (const int size : {0, 1, 2, 3, 100, 4096, 65535, 65536, 70000, 200000}) {
    SCOPED_TRACE(size);
    const auto plain = Corpus(lcg, size);
    const auto want = ReferenceCompress(plain);
    std::vector<std::byte> got(static_cast<std::size_t>(LcwWorstCaseSize(size)));
    const int got_size =
        engine_rs::lcw_compress(AsRustSlice(plain), AsRustSlice(std::span(got)));
    ASSERT_GE(got_size, 0);
    got.resize(static_cast<std::size_t>(got_size));
    EXPECT_TRUE(got == want);
  }
}

TEST(LcwDifferentialTest, CompressRejectsSmallOutputLikeReference) {
  Lcg lcg(2);
  const auto plain = RandomBytes(lcg, 500);
  std::vector<std::byte> output(static_cast<std::size_t>(LcwWorstCaseSize(500) - 1));
  EXPECT_EQ(lcw_reference::Compress(plain, output), -1);
  EXPECT_EQ(engine_rs::lcw_compress(AsRustSlice(plain), AsRustSlice(std::span(output))), -1);
}

TEST(LcwDifferentialTest, DamagedStreamsMatchLenientReference) {
  Lcg lcg(3);
  for (int iteration = 0; iteration < 2000; ++iteration) {
    SCOPED_TRACE(iteration);
    const auto plain = Corpus(lcg, lcg.Below(5000));
    const auto stream = Damage(lcg, ReferenceCompress(plain));
    // Exact, short and roomy destinations.
    const int size = static_cast<int>(plain.size());
    ExpectLenientMatch(stream, size);
    ExpectLenientMatch(stream, lcg.Below(size + 1));
    ExpectLenientMatch(stream, size + 64);
  }
}

TEST(LcwDifferentialTest, DamagedStreamsMatchBoundedReference) {
  Lcg lcg(4);
  for (int iteration = 0; iteration < 2000; ++iteration) {
    SCOPED_TRACE(iteration);
    const auto plain = Corpus(lcg, lcg.Below(5000));
    const auto stream = Damage(lcg, ReferenceCompress(plain));
    const int size = static_cast<int>(plain.size());
    ExpectBoundedMatch(stream, size);
    ExpectBoundedMatch(stream, lcg.Below(size + 1));
    ExpectBoundedMatch(stream, size + 64);
  }
}

TEST(LcwDifferentialTest, RandomBytesMatchReference) {
  Lcg lcg(5);
  for (int iteration = 0; iteration < 5000; ++iteration) {
    SCOPED_TRACE(iteration);
    const auto stream = RandomBytes(lcg, lcg.Below(400));
    ExpectLenientMatch(stream, lcg.Below(2000));
    ExpectBoundedMatch(stream, lcg.Below(2000));
  }
}

TEST(LcwDifferentialTest, InPlaceMatchesReference) {
  Lcg lcg(6);
  for (int iteration = 0; iteration < 1000; ++iteration) {
    SCOPED_TRACE(iteration);
    const auto plain = Corpus(lcg, 1 + lcg.Below(4000));
    const auto stream = Damage(lcg, ReferenceCompress(plain));
    // The WSA layout: output from the front, the stream at the very end, the
    // buffer as large as the larger of the two plus some slack.
    const auto dest_size = plain.size();
    const auto buffer_size =
        std::max(dest_size, stream.size()) + static_cast<std::size_t>(lcg.Below(64));
    std::vector<std::byte> want(buffer_size, kFill);
    const auto source_start = buffer_size - stream.size();
    std::ranges::copy(stream, want.begin() + static_cast<base::ssize>(source_start));
    std::vector<std::byte> got(want);

    const int32_t want_size = lcw_reference::Uncompress(
        std::span(want).subspan(source_start), std::span(want).first(dest_size));
    const auto got_size = engine_rs::lcw_uncompress_in_place(
        AsRustSlice(std::span(got)), source_start, stream.size(), dest_size);
    EXPECT_EQ(static_cast<int64_t>(got_size), int64_t{want_size});
    EXPECT_TRUE(got == want);
  }
}

}  // namespace
```

- [ ] **Step 7: Register the test and link the bridge**

`src/engine/codec/CMakeLists.txt`: add `lcw_differential_test.cc` and `lcw_reference_test_util.cc`
to the `add_gtest` `SOURCES`, and `engine_rs_cxx` to its `LIBS`:

```cmake
add_gtest(
        NAME engine_codec_test
        SOURCES aud_decoder_test.cc codec_corrupt_test.cc codec_state_test.cc
            compressed_block_test.cc lcw_comp_test.cc lcw_differential_test.cc
            lcw_reference_test_util.cc lcw_uncompress_test.cc stream_error_test.cc
            stream_golden_test.cc xor_delta_test.cc
        LIBS engine_codec engine_crypto engine_rs_cxx absl::strings
)
```

- [ ] **Step 8: Build, test, strict build**

Run:

```bash
cmake --build build --parallel 22 && ctest --test-dir build -R "LcwDifferential|engine_rs_cargo" --output-on-failure
```

Expected: the 6 `LcwDifferentialTest` cases and `engine_rs_cargo_test` pass. A differential failure
names its iteration; reproduce it from the seed, fix the Rust port (never the reference), and add
the case to the Rust unit tests of Task 1 or 2.

Then the strict build. `build-strict` must pick up the new `FetchContent` and subdirectory, which
needs a reconfigure; per CLAUDE.md the first build after a reconfigure re-populates `_deps` and can
print false "file not found" errors, so build twice before believing a finding:

```bash
cmake build-strict
cmake --build build-strict --parallel 14
```

Expected: no `error:` lines, `engine_rs_lint` runs rustfmt and clippy once, and no
`ERROR:clang-tidy-cache` lines (which would mean the generated headers are not `SYSTEM`).

- [ ] **Step 9: Install the toolchain in CI**

`.github/workflows/cmake-multi-platform.yml`: in the `build` job, after `actions/checkout`:

```yaml
    - uses: dtolnay/rust-toolchain@master
      with:
        toolchain: 1.98.1
```

and in the `lint` job, after its `actions/checkout`:

```yaml
    - uses: dtolnay/rust-toolchain@master
      with:
        toolchain: 1.98.1
        components: clippy, rustfmt
```

- [ ] **Step 10: Commit**

```bash
git add CMakeLists.txt src/rust/CMakeLists.txt src/rust/Cargo.toml src/rust/Cargo.lock \
  src/rust/src/lib.rs src/engine/codec/CMakeLists.txt src/engine/codec/rust_bytes.h \
  src/engine/codec/lcw_reference_test_util.h src/engine/codec/lcw_reference_test_util.cc \
  src/engine/codec/lcw_differential_test.cc .github/workflows/cmake-multi-platform.yml
git commit -q -F - <<'EOF'
Link the Rust LCW kernels through cxx and Corrosion

Corrosion builds src/rust with cargo and generates engine_rs_cxx, the
C++ side of the cxx bridge in lib.rs. Its generated code is exempt from
clang-tidy and the warning flags, and its headers are system headers to
every includer. Strict builds run rustfmt and clippy on the crate;
ctest runs its cargo tests.

LcwDifferentialTest holds the Rust kernels to the C++ ones, copied
verbatim into a test-only reference: identical compressed streams up to
200,000 bytes, and identical results and destination bytes for
thousands of damaged, truncated and random streams, in place and not.
The game still calls the C++ kernels.
EOF
```

---

### Task 4: Switch the C++ entry points to Rust

**Files:**

- Modify: `src/engine/codec/lcw.cc`, `src/engine/codec/lcw_uncompress.cc`,
  `src/engine/codec/CMakeLists.txt`, `src/engine/codec/lcw_differential_test.cc`, `CLAUDE.md`
- Create: `src/tools/lcw_bench.cc`; Modify: `src/tools/CMakeLists.txt`

**Interfaces:**

- Consumes: the bridge functions and `rust_bytes.h` (Task 3).
- Produces: unchanged `LCW_Uncompress()`, `LcwUncompBounded()`, `LCW_Comp()`; the `lcw_bench`
  executable.

- [ ] **Step 1: Write the failing public-API tests**

Append to `lcw_differential_test.cc`, inside the anonymous namespace, and add
`#include "engine/codec/lcw_uncompress.h"`:

```cpp
// The callers' in-place layouts, through the public entry point: the shim
// must route them to the in-place decoder and decode exactly as before.
TEST(LcwDifferentialTest, PublicApiDecodesInPlace) {
  Lcg lcg(7);
  for (int iteration = 0; iteration < 500; ++iteration) {
    SCOPED_TRACE(iteration);
    const auto plain = Corpus(lcg, 1 + lcg.Below(4000));
    const auto stream = ReferenceCompress(plain);
    const auto buffer_size = std::max(plain.size(), stream.size()) + 16;
    // At the tail (WSA, VQA), or from the destination's first byte.
    const auto source_start = lcg.Below(2) == 0 ? buffer_size - stream.size() : 0;
    std::vector<std::byte> want(buffer_size, kFill);
    std::ranges::copy(stream, want.begin() + static_cast<base::ssize>(source_start));
    std::vector<std::byte> got(want);

    const auto source = [&](std::vector<std::byte>& buffer) {
      return std::span<const std::byte>(buffer).subspan(source_start, stream.size());
    };
    const int32_t want_size = lcw_reference::Uncompress(
        source(want), std::span(want).first(plain.size()));
    const int32_t got_size =
        LCW_Uncompress(source(got), std::span(got).first(plain.size()));
    EXPECT_EQ(got_size, want_size);
    EXPECT_TRUE(got == want);
  }
}

// Load_Uncompress()'s layout: the packet at the tail of the buffer and the
// destination a prefix that may stop short of it, touch it, or run into it.
TEST(LcwDifferentialTest, PublicApiDecodesTailLayoutDisjoint) {
  Lcg lcg(8);
  const auto plain = Corpus(lcg, 3000);
  const auto stream = ReferenceCompress(plain);
  for (const int gap : {-100, -1, 0, 1, 100}) {
    SCOPED_TRACE(gap);
    const auto buffer_size = static_cast<std::size_t>(
        std::max<base::ssize>(std::ssize(plain) + gap, 0) + std::ssize(stream));
    std::vector<std::byte> want(buffer_size, kFill);
    const auto source_start = buffer_size - stream.size();
    std::ranges::copy(stream, want.begin() + static_cast<base::ssize>(source_start));
    std::vector<std::byte> got(want);
    const int32_t want_size = lcw_reference::Uncompress(
        std::span<const std::byte>(want).subspan(source_start),
        std::span(want).first(plain.size()));
    const int32_t got_size =
        LCW_Uncompress(std::span<const std::byte>(got).subspan(source_start),
                       std::span(got).first(plain.size()));
    EXPECT_EQ(got_size, want_size);
    EXPECT_TRUE(got == want);
  }
}

TEST(LcwDifferentialDeathTest, PublicApiRejectsSourceOverlappingTheFront) {
  std::vector<std::byte> buffer(16, std::byte{0x80});
  EXPECT_DEATH(LCW_Uncompress(std::span<const std::byte>(buffer).first(8),
                              std::span(buffer).subspan(4, 8)),
               "overlaps the front");
}

TEST(LcwDifferentialDeathTest, BoundedRejectsOverlappingSpans) {
  std::vector<std::byte> buffer(16, std::byte{0x80});
  EXPECT_DEATH(LcwUncompBounded(std::span<const std::byte>(buffer).subspan(8),
                                std::span(buffer).first(12)),
               "separate buffers");
}
```

`PublicApiDecodesTailLayoutDisjoint` with `gap = -100` overlaps; the others are disjoint or touch.

- [ ] **Step 2: Run them to verify they fail**

Run:
`cmake --build build --parallel 22 --target engine_codec_test && ctest --test-dir build -R LcwDifferential --output-on-failure`
Expected: the two death tests FAIL ("failed to die"): the C++ kernels accept both layouts. The two
in-place tests pass already (they still run the C++ kernel); they guard step 3.

- [ ] **Step 3: Replace the kernels with shims**

`src/engine/codec/lcw_uncompress.cc`: keep the EA header comment block, replace everything after it:

```cpp
#include "engine/codec/lcw_uncompress.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>

#include "absl/log/check.h"
#include "engine/codec/rust_bytes.h"
#include "engine_rs_cxx/lib.h"

// The decoder is engine_rs's lcw::uncompress (src/rust/src/lcw/decode.rs).
int32_t LCW_Uncompress(std::span<const std::byte> source,
                       std::span<std::byte> dest) {
  if (!Overlaps(source, dest)) {
    return static_cast<int32_t>(
        engine_rs::lcw_uncompress(AsRustSlice(source), AsRustSlice(dest)));
  }
  // The stream lies in the destination's own storage, as WSA animations, VQA
  // movies and Load_Uncompress() lay it out. Rust may not see one buffer as a
  // shared and a mutable slice at once, so the whole of it crosses once.
  const auto dest_begin = std::bit_cast<uintptr_t>(dest.data());
  const auto source_begin = std::bit_cast<uintptr_t>(source.data());
  CHECK_GE(source_begin, dest_begin)
      << "LCW stream overlaps the front of its destination";
  const std::size_t source_start = source_begin - dest_begin;
  const std::size_t buffer_size =
      std::max(dest.size(), source_start + source.size());
  // Both spans lie in one array, which runs at least this far.
  // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
  const std::span<std::byte> buffer(dest.data(), buffer_size);
  return static_cast<int32_t>(engine_rs::lcw_uncompress_in_place(
      AsRustSlice(buffer), source_start, source.size(), dest.size()));
}

int32_t LCW_Uncompress(std::span<const unsigned char> source,
                       std::span<unsigned char> dest) {
  return LCW_Uncompress(std::as_bytes(source), std::as_writable_bytes(dest));
}
```

`src/engine/codec/lcw.cc`: keep the EA header comment block, replace everything after it:

```cpp
#include "engine/codec/lcw.h"

#include <cstddef>
#include <span>

#include "absl/log/check.h"
#include "engine/codec/rust_bytes.h"
#include "engine_rs_cxx/lib.h"

// The kernels are engine_rs's lcw module (src/rust/src/lcw/).

int LcwUncompBounded(std::span<const std::byte> source,
                     std::span<std::byte> dest) {
  CHECK(!Overlaps(source, dest)) << "LCW needs separate buffers";
  return engine_rs::lcw_uncompress_bounded(AsRustSlice(source),
                                           AsRustSlice(dest));
}

int LCW_Comp(std::span<const std::byte> in, std::span<std::byte> out) {
  CHECK(!Overlaps(in, out)) << "LCW needs separate buffers";
  return engine_rs::lcw_compress(AsRustSlice(in), AsRustSlice(out));
}
```

`src/engine/codec/CMakeLists.txt`:

```cmake
target_link_libraries(engine_codec
        PUBLIC engine_stream
        PRIVATE absl::check absl::core_headers engine_rs_cxx lzo
)
```

and delete the `optimize_in_debug(lcw.cc lcw_uncompress.cc)` line with its two-line comment: the
kernels are Rust now, and `[profile.dev] opt-level = 2` keeps them fast in Debug.

- [ ] **Step 4: Run every test**

Run: `cmake --build build --parallel 22 && ctest --test-dir build --parallel 22 --output-on-failure`
Expected: everything passes, including the unchanged `LcwDestinationTest`, `LcwCompTest`,
`codec_corrupt_test`, `stream_golden_test`, and all 10 `LcwDifferential*` cases.

- [ ] **Step 5: Add the benchmark**

`src/tools/lcw_bench.cc`:

```cpp
// Times the LCW kernels the engine uses (Rust, through src/rust) against the
// C++ reference they replaced, on 8 MiB of game-like data.
//
// Usage: lcw_bench

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <utility>
#include <vector>

#include "absl/strings/str_format.h"
#include "engine/codec/lcw.h"
#include "engine/codec/lcw_reference_test_util.h"
#include "engine/codec/lcw_uncompress.h"

namespace {

constexpr int kBlockSize = 64 * 1024;
constexpr int kBlocks = 128;
constexpr auto kBlockBytes = static_cast<std::size_t>(kBlockSize);
constexpr auto kStreamBytes =
    static_cast<std::size_t>(LcwWorstCaseSize(kBlockSize));

// Runs, repeats and noise, as in lcw_differential_test.cc's Corpus().
std::vector<std::byte> GameLikeBlock(uint32_t seed) {
  std::vector<std::byte> bytes;
  const auto next = [&seed] {
    seed = (seed * 1664525U) + 1013904223U;
    return seed >> 8;
  };
  while (std::ssize(bytes) < kBlockSize) {
    const auto piece = static_cast<std::size_t>(1 + (next() % 300));
    switch (next() % 3) {
      case 0:
        bytes.insert(bytes.end(), piece, static_cast<std::byte>(next() & 0xff));
        break;
      case 1:
        if (bytes.size() > piece) {
          const auto from = next() % (bytes.size() - piece);
          for (std::size_t i = 0; i < piece; ++i) {
            bytes.push_back(bytes.at(from + i));
          }
          break;
        }
        [[fallthrough]];
      default:
        for (std::size_t i = 0; i < piece; ++i) {
          bytes.push_back(static_cast<std::byte>(next() & 0xff));
        }
        break;
    }
  }
  bytes.resize(kBlockBytes);
  return bytes;
}

template <typename Function>
double MegabytesPerSecond(const Function& function) {
  const auto start = std::chrono::steady_clock::now();
  function();
  const std::chrono::duration<double> seconds =
      std::chrono::steady_clock::now() - start;
  const double megabytes = static_cast<double>(kBlockSize) *
                           static_cast<double>(kBlocks) / (1024.0 * 1024.0);
  return megabytes / seconds.count();
}

}  // namespace

int main() {
  std::vector<std::vector<std::byte>> plain;
  std::vector<std::vector<std::byte>> streams;
  for (int block = 0; block < kBlocks; ++block) {
    plain.push_back(GameLikeBlock(static_cast<uint32_t>(block + 1)));
    std::vector<std::byte> stream(kStreamBytes);
    stream.resize(static_cast<std::size_t>(LCW_Comp(plain.back(), stream)));
    streams.push_back(std::move(stream));
  }
  std::vector<std::byte> output(kBlockBytes);
  std::vector<std::byte> scratch(kStreamBytes);

  const auto report = [](const char* what, double rust, double cpp) {
    absl::PrintF("%-18s Rust %8.1f MB/s   C++ %8.1f MB/s   %+.0f%%\n", what,
                 rust, cpp, ((rust / cpp) - 1.0) * 100.0);
  };
  report("lenient decode",
         MegabytesPerSecond([&] {
           for (const auto& stream : streams) {
             LCW_Uncompress(stream, output);
           }
         }),
         MegabytesPerSecond([&] {
           for (const auto& stream : streams) {
             lcw_reference::Uncompress(stream, output);
           }
         }));
  report("bounded decode",
         MegabytesPerSecond([&] {
           for (const auto& stream : streams) {
             LcwUncompBounded(stream, output);
           }
         }),
         MegabytesPerSecond([&] {
           for (const auto& stream : streams) {
             lcw_reference::UncompressBounded(stream, output);
           }
         }));
  report("compress",
         MegabytesPerSecond([&] {
           for (const auto& block : plain) {
             LCW_Comp(block, scratch);
           }
         }),
         MegabytesPerSecond([&] {
           for (const auto& block : plain) {
             lcw_reference::Compress(block, scratch);
           }
         }));
  return 0;
}
```

`src/tools/CMakeLists.txt`, append:

```cmake
# Times the Rust LCW kernels against the C++ reference they replaced.
add_executable(lcw_bench lcw_bench.cc
        ${CMAKE_SOURCE_DIR}/src/engine/codec/lcw_reference_test_util.cc)
target_link_libraries(lcw_bench PRIVATE engine_codec base absl::str_format)
```

Run: `cmake --build build --parallel 22 --target lcw_bench && build/src/tools/lcw_bench`

Expected: three lines. Record them in this document's Outcome. If Rust is more than 10% slower on
either decoder, profile before going on (`perf record build/src/tools/lcw_bench`); the usual cause
is a bounds check the optimizer could not hoist, fixed by iterating over a pre-checked subslice
instead of indexing, never by `unsafe`.

- [ ] **Step 6: Update CLAUDE.md**

In the Quick Reference paragraph on `Debug`, change the kernel list "(RSA bignum, Blowfish, SHA-1,
LCW, the fading table, the window blit) are compiled at `-O2` even in `Debug` via
`optimize_in_debug()`" to "(RSA bignum, Blowfish, SHA-1, the fading table, the window blit) are
compiled at `-O2` even in `Debug` via `optimize_in_debug()`, and the Rust kernels in `src/rust/`
(LCW) via `[profile.dev] opt-level = 2` in `src/rust/Cargo.toml`".

- [ ] **Step 7: Full verification**

Run, each as its own command:

```bash
git clang-format -f -- src/engine/codec/lcw.cc src/engine/codec/lcw_uncompress.cc \
  src/engine/codec/lcw_differential_test.cc src/engine/codec/rust_bytes.h \
  src/engine/codec/lcw_reference_test_util.h src/engine/codec/lcw_reference_test_util.cc \
  src/tools/lcw_bench.cc
cmake --build build-strict --parallel 14
tools/run_cpplint.py src/engine/codec/lcw.cc src/engine/codec/lcw_uncompress.cc \
  src/engine/codec/lcw_differential_test.cc src/engine/codec/rust_bytes.h \
  src/engine/codec/lcw_reference_test_util.h src/engine/codec/lcw_reference_test_util.cc \
  src/tools/lcw_bench.cc
ctest --test-dir build --parallel 22
```

Expected: no `error:` lines, cpplint silent, all tests pass. `engine_layers_test` passes (the
generated headers resolve outside `src/`, which `check_layers.py` treats as third-party).

- [ ] **Step 8: Commit**

```bash
git add src/engine/codec/lcw.cc src/engine/codec/lcw_uncompress.cc \
  src/engine/codec/CMakeLists.txt src/engine/codec/lcw_differential_test.cc \
  src/tools/lcw_bench.cc src/tools/CMakeLists.txt CLAUDE.md
git commit -q -F - <<'EOF'
Decode and encode LCW in Rust

LCW_Uncompress(), LcwUncompBounded() and LCW_Comp() keep their
signatures and become shims over the engine_rs bridge, so no caller
changes. The lenient decoder routes a stream stored in its own output,
as WSA, VQA and Load_Uncompress() lay it out, to the in-place entry
point; a stream overlapping the front of its output, which no caller
produces, now fails a CHECK, as do overlapping spans for the strict
decoder and the encoder.

lcw_bench times the Rust kernels against the C++ reference; the
numbers it printed in step 5 follow.
EOF
```

Before running it, append the three lines `lcw_bench` printed in step 5 to the message, indented by
four spaces.

---

### Task 5: Check it in the games and record the outcome

**Files:**

- Modify: `docs/RUST_LCW_PLAN.md` (status, Outcome), `CLAUDE.md` (Setup, Architecture), `TODO.md`

- [ ] **Step 1: Save and load**

Run: `tools/ra_saveload_smoke.sh build/src/ra/rasdl` and
`tools/td_saveload_smoke.sh build/src/td/tdsdl` Expected: both report no differences. Saves carry
the map and overlay packs, written by `LCW_Comp()` and read by `LcwUncompBounded()`.

- [ ] **Step 2: Movies, animations and pictures** (needs the user's display; ask them)

```bash
cd "/home/konsto/.local/share/Steam/steamapps/common/Command & Conquer Red Alert"
SDL_AUDIODRIVER=dummy ~/projects/CnC_Red_Alert/build/src/ra/rasdl -NOMOUSEGRAB \
  -CD"/home/konsto/.local/share/Steam/steamapps/common/Command & Conquer Red Alert"
```

Watch for: the intro movie (VQA, `LcwBuffer` in place), the title screen and the briefing pictures
(CPS, `Load_Uncompress()` in place), the cursor and every shape (`hardware_cursor.cc`,
`keyframe.cc`). In Tiberian Dawn, the map selection screen's WSA animations (`WsaAnimation` in
place). Any garbled image means a layout the tests missed: capture the buffer and add it to
`PublicApiDecodesInPlace`.

- [ ] **Step 3: Release-build timings** (CLAUDE.md's figures: saved-game load 19 ms, startup
      bootstrap 45 ms at `RelWithDebInfo`)

Run the same measurement CLAUDE.md's figures came from (headless `-LOADGAME`, see
`docs/STARTUP_PERF_PLAN.md`) before and after (`git stash` is not enough across a crate; build the
commit before Task 4 in a worktree). Expected: within noise.

- [ ] **Step 4: Debug-build speed**

```bash
cmake -Bbuild-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DSTRICT_CHECKS=OFF
cmake --build build-debug --parallel 22 --target rasdl
```

Play the intro movie as in step 2 with `build-debug/src/ra/rasdl`. Expected: it plays in real time
(10.5 s), as it did with `optimize_in_debug()`. If it stretches, the dev profile is not reaching the
crate: check that Corrosion builds `Debug` with the `dev` profile
(`build-debug/cargo/build/*/debug/` exists).

- [ ] **Step 5: Documentation**

`CLAUDE.md`:

- Setup, "Required Dependencies", add: "**All platforms:** ... and the Rust toolchain through rustup
  (`curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh`); `rust-toolchain.toml` pins the
  version and rustup installs it on first use."
- Architecture tree, add after `engine/`:
  `rust/ → engine_rs, the engine kernels written in Rust (LCW), linked through the cxx bridge in src/lib.rs; CMake target engine_rs_cxx (Corrosion) [depends: nothing]`
- Under the Ninja notes: "Corrosion's cargo step runs on every build and lets cargo decide what is
  stale (about 0.2 s), so `cmake --build` never prints `ninja: no work to do`; that is not the
  rebuild loop described above."

`TODO.md`, under a suitable section:

```markdown
- Build the Rust crate for Emscripten
  - `src/rust` is linked into every target since `docs/RUST_LCW_PLAN.md`, and no CI job builds
    Emscripten; Corrosion needs `Rust_CARGO_TARGET=wasm32-unknown-emscripten` and that target
    installed through rustup
- Decide when to delete the C++ LCW reference
  - `engine/codec/lcw_reference_test_util.cc` exists only for `LcwDifferentialTest` and
    `lcw_bench`; once the Rust kernels have shipped without a report, both can go
```

This document: set `**Status: done** (<date>, <first>..<last commit>)` and add `## Outcome` with the
benchmark numbers, the timing comparison and anything that differed from the plan (the generated
header path, whether `Rust_TOOLCHAIN` had to be set).

- [ ] **Step 6: Commit**

```bash
git add CLAUDE.md TODO.md docs/RUST_LCW_PLAN.md
git commit -q -F - <<'EOF'
Record the Rust LCW outcome and the toolchain setup
EOF
```

## Verification

Done when all of these hold on the final tree:

- `ctest --test-dir build` passes, including `engine_rs_cargo_test` and every `LcwDifferential*`
  case; the pre-existing LCW, block codec and golden tests pass unmodified.
- `cmake --build build-strict --parallel 14` reports no errors, runs rustfmt and clippy clean, and
  prints no `ERROR:clang-tidy-cache` lines.
- CI is green on all four platforms and the lint job.
- Both save/load smoke scripts report no differences.
- The intro movie, title screen, cursor, shapes and TD's map-selection animations look right on a
  real display, in `RelWithDebInfo` and in `Debug`, and the Debug intro plays in real time.
- `lcw_bench` shows neither decoder more than 10% slower than the C++ reference.

## Outcome

Done 2026-09-30 on branch `rust-lcw`. Every C++ test passes unchanged, and `LcwDifferentialTest`
holds the Rust kernels to the C++ reference byte for byte.

`lcw_bench` at `RelWithDebInfo`, 8 MiB of game-like data (third of three runs; the encoder varies by
about ±8% from run to run in both directions):

| Kernel         | Rust        | C++         | Change |
| -------------- | ----------- | ----------- | ------ |
| lenient decode | 6120.3 MB/s | 3266.1 MB/s | +87%   |
| bounded decode | 9398.8 MB/s | 3081.4 MB/s | +205%  |
| compress       | 50.2 MB/s   | 51.0 MB/s   | -2%    |

In the game, eight interleaved headless runs (`SDL_VIDEODRIVER=offscreen`, `-NOMOVIES`) of the
commit before the switch (5f48e4f1) against the switch: `-NEWGAMESCG01EA -QUITFRAME1` took 0.42 s
and `-LOADGAME -QUITFRAME61` 0.24 s with either kernel, within noise. Both save/load smoke scripts
report no differences. `vqaplay REDINTRO.VQA` (188 frames at 15 fps) takes 12.54 s in `Debug` and in
`RelWithDebInfo`, so the Debug build still plays it in real time.

What differed from the plan:

- The generated headers are where the plan expected (`engine_rs_cxx/lib.h`, `rust/cxx.h`).
- Corrosion does not honor `rust-toolchain.toml` by itself: it asks rustup for the active toolchain
  from CMake's working directory, so a configure from outside the tree took the default toolchain,
  or found none and failed. The root `CMakeLists.txt` reads the channel from the file and forces
  `Rust_TOOLCHAIN` to it.
- A fresh configure failed to find the generated headers: Corrosion adds them to `engine_rs_cxx` as
  `PUBLIC` sources, and under `cmake_minimum_required(VERSION 3.14)` policy CMP0118 is OLD, so only
  `src/rust` knew they were generated. The root `CMakeLists.txt` sets CMP0118 to NEW.
- `Rust_CARGO_CACHED` is the toolchain's own `cargo`, which looks up `rustc` on `PATH`; the
  `engine_rs_cargo_test` and the lint run it under
  `cmake -E env --modify PATH=path_list_prepend:...` so they work without `~/.cargo/bin` on `PATH`.
- Corrosion's target directory is `<build>/cargo/rust_<hash>/`, not `<build>/cargo/build/`; a Debug
  build reports `Finished dev profile [optimized + debuginfo]`.
- The no-change cargo step takes under 0.1 s, not 0.2 s.
- The death tests needed the tree's usual `NOLINTNEXTLINE` for gtest's macro internals, and the
  differential test uses `base::ToSize()` instead of `static_cast<std::size_t>`.
