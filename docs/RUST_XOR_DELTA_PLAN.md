# Plan: the XOR delta decoder in Rust

Written 2026-10-09. Phase 1 of `docs/RUST_MIGRATION_PLAN.md`, in the form of
`docs/RUST_LCW_PLAN.md`, whose build setup and global constraints it inherits.

**Status: done** (2026-10-09).

**Goal:** `ApplyXorDelta()` and `ApplyXorDeltaToView()` (`engine/codec/xor_delta.h`) decode in Rust,
with the same target bytes for every delta and no caller changed.

## Findings

- Both entry points run one kernel, `DecodeDelta(target, delta, width, stride, copy)`; the
  contiguous form passes the target's size as both width and stride. One Rust function and one
  bridge function serve both.
- Neither caller works in place. `BuildFrame()` (`engine/gfx/keyframe.cc`) XORs a delta from the
  shape file onto a vector it just decoded; `WsaAnimation` (`engine/gfx/wsa_animation.cc`) XORs its
  own delta buffer onto the view's pixels. The shim `CHECK`s that the spans do not overlap.
- The decoder is lenient: it stops at a command that is cut short, has a zero count or runs past the
  target, keeping what it has written. The Rust one returns at the same points and never panics.
- `xor_delta_test` has three cases, `keyframe_test` and `wsa_animation_test` decode real frames
  through it. They stay unchanged.

## Files

| Path                                                   | Change                                            |
| ------------------------------------------------------ | ------------------------------------------------- |
| `src/rust/src/xor_delta.rs`                            | New: `apply()` and its tests                      |
| `src/rust/src/lib.rs`                                  | `xor_delta_apply` on the bridge                   |
| `src/engine/codec/xor_delta.cc`                        | Shim over the bridge                              |
| `src/engine/codec/xor_delta_reference_test_util.h/.cc` | New: the C++ decoder as it was, test-only         |
| `src/engine/codec/xor_delta_differential_test.cc`      | New: Rust against the reference                   |
| `src/base/lcg_test_util.h`                             | New: the seeded generator differential tests use  |
| `src/tools/kernel_bench.cc`                            | New: times each Rust kernel against its reference |

## Tasks

- [x] `xor_delta.rs` with cargo tests: every command kind, every early stop, rows with padding, a
      partial last row, bad geometry
- [x] Bridge function, shim, reference copy
- [x] Differential test: 3,000 well-formed, 6,000 damaged and 6,000 random deltas, each applied
      contiguously and to rows (XOR and copy), plus empty spans and the overlap death test
- [x] `kernel_bench`
- [x] `ctest`, the strict build, `tools/ui_check.py` for both games

## Outcome

Every existing test passes unchanged and `tools/ui_check.py` reports no changed screenshot in either
game. `kernel_bench` at `RelWithDebInfo` (GCC 13 for the C++ reference), a 320x200 frame of mixed
commands:

| Case       | Rust        | C++        |
| ---------- | ----------- | ---------- |
| contiguous | 19,978 MB/s | 2,574 MB/s |
| rows       | 16,521 MB/s | 2,515 MB/s |

The C++ decoder XORed through `std::ranges::transform` with a `copy` test per byte; the Rust one
picks one of four loops per row segment, which the compiler vectorizes.
