# Plan: SHA-1 block compression in Rust

Written 2026-10-09. Phase 4 of `docs/RUST_MIGRATION_PLAN.md`, in the form of
`docs/RUST_LCW_PLAN.md`, whose build setup and global constraints it inherits.

**Status: done** (2026-10-09).

**Goal:** `Sha1Compress()` and its portable and hardware variants (`engine/crypto/sha1_compress.h`)
run in Rust. `SHAEngine` keeps padding and partial blocks and does not change.

## Decisions

- **A port, not the `sha1` crate.** The crate's block function is behind an opt-in `compress`
  feature it documents as hazardous, and using its `Sha1` hasher instead would move padding out of
  `SHAEngine`, which is outside this phase. A port is 80 lines for the portable code and 60 for the
  SHA extensions, and keeps the crate's dependencies at `cxx`. This is also the answer to the
  migration plan's open question for Phase 5: Blowfish is ported too.
- **`unsafe` is one call.** Since Rust 1.86 the `core::arch` intrinsics are safe to call inside a
  function compiled with their `#[target_feature]`; what is `unsafe` is calling that function from
  code compiled without the features. `sha1.rs` is `#![deny(unsafe_code)]` with a single
  `#[allow(unsafe_code)]` block in `compress_hardware()`, after it has asserted
  `is_x86_feature_detected!`. The block is loaded with `u32::from_be_bytes` instead of
  `_mm_loadu_si128`, which takes a raw pointer; the compiler emits the same `movdqu` and `pshufb`.
- **The state crosses as `&mut [u32; 5]`,** which cxx maps to `std::array<uint32_t, 5>&`, the type
  `Sha1State` already is.
- **`rust_bytes.h` moved to `engine/stream`** (its own commit), below both `engine/codec` and
  `engine/crypto`, which do not include each other.

## Files

| Path                                                    | Change                                        |
| ------------------------------------------------------- | --------------------------------------------- |
| `src/rust/src/sha1.rs`                                  | New: portable and SHA-extension code, tests   |
| `src/rust/src/lib.rs`                                   | Four bridge functions                         |
| `src/engine/crypto/sha1_compress.cc`                    | Shims; keeps the whole-blocks `CHECK`         |
| `src/engine/crypto/sha1_compress_reference_test_util.*` | New: the C++ code, test-only                  |
| `src/engine/crypto/sha1_compress_differential_test.cc`  | New                                           |
| `src/engine/crypto/CMakeLists.txt`                      | Links `engine_rs_cxx`; no `optimize_in_debug` |
| `src/tools/kernel_bench.cc`                             | Two rows                                      |

## Tasks

- [x] `sha1.rs` with cargo tests: FIPS vectors through every implementation, chaining, hardware
      against portable
- [x] Bridge, shims, reference copy
- [x] Differential test: random states and 0 to 8 blocks, portable and hardware against the portable
      reference; the dispatching entry point against the reference's
- [x] `sha1_compress_test` unchanged; `kernel_bench`, `ctest`, the strict build

## Outcome

Every existing test passes unchanged. `kernel_bench` at `RelWithDebInfo` (GCC 13 for the C++
reference), 64 KiB of random blocks:

| Path           | Rust       | C++        | Change |
| -------------- | ---------- | ---------- | ------ |
| portable       | 534 MB/s   | 291 MB/s   | +84%   |
| SHA extensions | 2,698 MB/s | 2,704 MB/s | 0%     |

The portable code is faster because it keeps a 16-word rolling schedule where the C++ filled 80
words through bounds-checked accessors.

The first hardware version looped over the 20 groups of four rounds and ran a third slower than C++:
the compiler unrolled three groups and kept the message and E registers of the other seventeen in
memory, indexed by the loop counter. The groups are now expanded by a macro, as the C++ expanded
them with a template parameter pack.

`sha1_compress.cc` no longer needs `optimize_in_debug()`: the Rust crate is built at `opt-level = 2`
in Debug.
