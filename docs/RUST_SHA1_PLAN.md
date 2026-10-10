# Plan: SHA-1 block compression in Rust

Written 2026-10-09. Phase 4 of `docs/RUST_MIGRATION_PLAN.md`, in the form of
`docs/RUST_LCW_PLAN.md`, whose build setup and global constraints it inherits.

**Status: done** (2026-10-09).

**Goal:** `Sha1Compress()` and its portable and hardware variants (`engine/crypto/sha1_compress.h`)
run in Rust. `SHAEngine` keeps padding and partial blocks and does not change.

## Decisions

- **The `sha1` crate, not a port** (decided 2026-10-09, after a port had been committed in
  199b1f83). `sha1::block_api::compress(state, blocks)` is the same operation as `Sha1Compress()`, a
  plain public function of version 0.11. Measured against the port on this machine: identical
  states, 2,701 against 2,775 MB/s with the SHA extensions. What the crate adds is the ARM SHA
  extensions, which neither the C++ nor the port had and Apple Silicon has; what it removes is the
  crate's only `unsafe` block, so `sha1.rs` is `#![forbid(unsafe_code)]` like the other kernels. It
  costs nine transitive crates (`digest`, `cpufeatures`, `hybrid-array`, `typenum`, ...), built
  without the default features, which only add object identifiers.

  The first version of this document said the crate's block function was behind an opt-in `compress`
  feature documented as hazardous. That was true of 0.10 and wrong for 0.11; the claim was not
  checked against the crate before the port was written.

- **`Sha1CompressPortable()`, `Sha1CompressHardware()` and `Sha1HardwareAvailable()` are gone.** The
  crate chooses its code path by itself and offers no way to force one, so the three could only have
  been three names for one call. Nothing but `sha1_compress_test`'s `HardwareMatchesPortable` used
  them; that test is deleted, and the differential test below does its job against the portable C++.
- **The state crosses as `&mut [u32; 5]`,** which cxx maps to `std::array<uint32_t, 5>&`, the type
  `Sha1State` already is.
- **`rust_bytes.h` moved to `engine/stream`** (its own commit), below both `engine/codec` and
  `engine/crypto`, which do not include each other.

## Files

| Path                                                    | Change                                            |
| ------------------------------------------------------- | ------------------------------------------------- |
| `src/rust/Cargo.toml`, `Cargo.lock`                     | The `sha1` dependency                             |
| `src/rust/src/sha1.rs`                                  | New: `compress()` and `digest()` over the crate   |
| `src/rust/src/lib.rs`                                   | One bridge function                               |
| `src/engine/crypto/sha1_compress.h`, `.cc`              | One shim; keeps the whole-blocks `CHECK`          |
| `src/engine/crypto/sha1_compress_test.cc`               | `HardwareMatchesPortable` deleted                 |
| `src/engine/crypto/sha1_compress_reference_test_util.*` | New: the C++ code with its three paths, test-only |
| `src/engine/crypto/sha1_compress_differential_test.cc`  | New                                               |
| `src/engine/crypto/CMakeLists.txt`                      | Links `engine_rs_cxx`; no `optimize_in_debug`     |
| `src/tools/kernel_bench.cc`                             | One row                                           |

## Tasks

- [x] `sha1.rs` with cargo tests: FIPS vectors, chaining, a partial block
- [x] Bridge, shim, reference copy
- [x] Differential test: random states and 0 to 8 blocks against the portable C++ reference, and 0
      to 39 blocks against the reference's own choice of path
- [x] `kernel_bench`, `ctest`, the strict build

## Outcome

`sha1_compress_test`'s other tests pass unchanged. `kernel_bench` at `RelWithDebInfo` (GCC 13 for
the C++ reference), 64 KiB of random blocks, both sides using the SHA extensions: 2,703 MB/s each.

The crate's portable code cannot be timed on a CPU with the extensions, and the ctests here never
run it. It was run once by hand, and passes the module's vectors:
`RUSTFLAGS='--cfg sha1_backend="soft"' cargo test --locked sha1`. The port's, which is no longer in
the tree, ran at 534 MB/s against the C++'s 291.

`sha1_compress.cc` no longer needs `optimize_in_debug()`: the Rust crate is built at `opt-level = 2`
in Debug.
