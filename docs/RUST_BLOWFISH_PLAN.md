# Plan: Blowfish in Rust

Written 2026-10-09. Phase 5 of `docs/RUST_MIGRATION_PLAN.md`, in the form of
`docs/RUST_LCW_PLAN.md`, whose build setup and global constraints it inherits.

**Status: done** (2026-10-09). With Phase 4, opening an encrypted MIX archive hashes and decrypts in
Rust; the RSA step that unwraps the key is Phase 6.

**Goal:** `BlowfishEngine` (`engine/crypto/blowfish.h`) becomes a wrapper around a Rust cipher. Its
public functions, their return values and the copy-through behavior without a key do not change.

## Decisions

- **A port,** as decided in `docs/RUST_SHA1_PLAN.md`. The S-boxes and sub-keys in
  `src/rust/src/blowfish/tables.rs` were generated from the C++ tables by script, not retyped.
- **The opaque type is `engine_rs::Blowfish`,** the 4,168 bytes of tables a key expands to. C++
  holds it as `rust::Box<Blowfish>` inside `BlowfishEngine::Key`, a struct defined in `blowfish.cc`,
  so `blowfish.h` does not include the generated bridge header and its includers (`pk_sink.cc`, Red
  Alert's `saveload.cc`, ...) need not link `engine_rs_cxx`.
- **Without a key the engine copies,** and that stays in C++: a null `key_` and `base::MoveBytes`.
  An empty key removes the key, as `Submit_Key({})` did.
- **In place means one buffer.** The only caller, `BlowfishCodec::Process()`, passes its 8-byte
  block as both arguments. The shim sends spans that do not overlap to `blowfish_encrypt` /
  `blowfish_decrypt` and spans that start at the same byte to the `_in_place` forms; any other
  overlap `CHECK`-fails, as no caller has a use for it.
- **Key material is wiped on drop.** The C++ destructor reset the tables to their constants; the
  Rust type's `Drop` zeroes them. Submitting a second key re-keys the tables where they are
  (`blowfish_set_key`).
- **`Encrypt()` and `Decrypt()` stay non-const.** They could be const now, but clang-tidy then wants
  them `[[nodiscard]]`, which `BlowfishCodec` would fail; the suppression says so.

## Files

| Path                                               | Change                                         |
| -------------------------------------------------- | ---------------------------------------------- |
| `src/rust/src/blowfish/mod.rs`, `tables.rs`        | New: the cipher, its tables, tests             |
| `src/rust/src/lib.rs`                              | The opaque type and six bridge functions       |
| `src/engine/crypto/blowfish.h`, `blowfish.cc`      | The class as a wrapper; tables and rounds gone |
| `src/engine/crypto/blowfish_reference_test_util.*` | New: the C++ engine, test-only                 |
| `src/engine/crypto/blowfish_differential_test.cc`  | New                                            |
| `src/engine/crypto/CMakeLists.txt`                 | `blowfish.cc` no longer `optimize_in_debug`    |
| `src/tools/kernel_bench.cc`                        | Three rows                                     |

## Tasks

- [x] `blowfish` module with cargo tests: Eric Young's vectors both ways, variable key lengths,
      re-keying, partial blocks, in place
- [x] Bridge, wrapper, reference copy
- [x] Differential test: 8 random keys of every length from 1 to 56; every data length from 0 to 69
      into an exact and a longer destination and in place; 200 re-keyings, a quarter of them
      removing the key; death tests for a partial overlap and a 57-byte key
- [x] `blowfish_test`, `blowfish_stream_test`, `mix_archive_test`, `stream_golden_test` unchanged
- [x] Both games start from the real MIX files (`tools/ui_check.py`, every script of both games);
      `tools/ra_saveload_smoke.sh` and `tools/td_saveload_smoke.sh` report no differences

## Outcome

Every existing test passes unchanged. `kernel_bench` at `RelWithDebInfo` (GCC 13 for the C++
reference), 64 KiB in place with a 56-byte key:

| Operation  | Rust     | C++      | Change |
| ---------- | -------- | -------- | ------ |
| encrypt    | 380 MB/s | 304 MB/s | +25%   |
| decrypt    | 381 MB/s | 304 MB/s | +25%   |
| key set-up | 215 MB/s | 207 MB/s | +4%    |

Key set-up is measured as megabytes of table per second. It is slower than bulk encryption in both
languages because each block it encrypts is the input of the next, so nothing overlaps.

The first version took the S-box indices from `half.to_be_bytes()` and set a key up 9% slower than
C++: the byte swap sat on the path from one round to the next. Shifting each byte out of the word
made up 14% everywhere.
