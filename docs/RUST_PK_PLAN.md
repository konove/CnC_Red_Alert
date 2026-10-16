# Plan: the bignum and `PKey` in Rust

Written 2026-10-10. Phase 6 of `docs/RUST_MIGRATION_PLAN.md`, in the form of
`docs/RUST_LCW_PLAN.md`, whose build setup and global constraints it inherits.

**Status: done** (2026-10-10).

**Goal:** `PKey` (`engine/crypto/pk.h`) raises its blocks to a power in Rust, and the C++ bignum
(`mp.cc`, `mp.h`, `int.h`, `digit_cursor.h`, 3,700 lines) leaves the engine.

## Decisions

- **The cut is at `PKey`.** A key is two 2048-bit numbers held by C++ as plain bytes, least
  significant first, which is the layout the C++ `BigInt` had. Five functions cross the bridge:
  decode a DER integer, encode one, count a number's bits, raise one block to a power, and make a
  key pair. `PKey` stays copyable and keeps its block loop, block sizes and public functions.
- **`num-bigint` does the arithmetic** (decided 2026-10-09 by building both). A port of the
  arithmetic (square and multiply over Knuth's long division) was written first and passed its
  tests. Timed against it, `num-bigint` gives the same results, takes 4.6 against 5.3 microseconds
  for the MIX key's operation and 390 against 2,746 for a 1024-bit private-key operation, and is 250
  lines the project does not own. `crypto-bigint` was ruled out: its modular form needs an odd
  modulus, and a malformed key can be even. The `rsa` crate was not built; only a release candidate
  of 0.10 exists.
- **The quirks are ours and are kept,** because a key and a key block are file formats
  (`src/rust/src/pk/mod.rs` documents each):
  - A DER integer with its top bit set is negative and is sign-extended over the register.
  - Malformed DER leaves the number as it was.
  - `Encode_Modulus()` and `Encode_Exponent()` encode only the low 512 bits. The C++ encoder began
    its search for the top byte at the register's length in digits, not bytes.
  - A zero exponent, a zero modulus, or a block or exponent that is not below the modulus gives 1.
- **Key generation is seeded.** `Generate()` used to pull bytes from its `ByteSource` as it went,
  which a leaf kernel cannot do. It now reads `PKey::kSeedBytes` (64) once and Rust derives
  everything from them with SHA-1 over the seed and a counter. The same source therefore gives a
  different pair than the C++ did; nothing depends on which pair, only on the pair working. Primes
  are random odd numbers of the asked size that pass trial division and 24 rounds of Miller-Rabin;
  the C++ looked for safe primes with a sieve.
- **`int.h` is gone, so two callers changed.** `IniFile::GetPublicKey()` and `tools/game_data.cc`
  built the fast exponent as a `BigInt` and DER-encoded it to feed `Decode_Exponent()`. They call
  the new `PKey::SetFastExponent()`.
- **Removed with their subject:** `PKey`'s two-argument constructor, which nothing called and which
  set the block size from the modulus before decoding it; `mp_test.cc` (its DER case moved to
  `pk_test.cc`); `codec_state_test`'s `ModularMultiplicationMatchesIndependentRemainder`, a test of
  `XMP_Mod_Mult`.

## Files

| Path                                                      | Change                                      |
| --------------------------------------------------------- | ------------------------------------------- |
| `src/rust/src/pk/mod.rs`                                  | New: DER, powers, key pairs, tests          |
| `src/rust/src/sha1.rs`                                    | `digest()`, for the seeded generator        |
| `src/rust/src/lib.rs`, `Cargo.toml`, `Cargo.lock`         | Five bridge functions; `num-bigint`         |
| `src/engine/crypto/pk.h`, `pk.cc`                         | The class over the bridge                   |
| `src/engine/crypto/mp_reference_test_util.h/.cc`          | Was `mp.h`, `mp.cc`; test-only              |
| `src/engine/crypto/int_reference_test_util.h`             | Was `int.h`                                 |
| `src/engine/crypto/digit_cursor_reference_test_util.h`    | Was `digit_cursor.h`                        |
| `src/engine/crypto/pk_reference_test_util.h/.cc`          | The old `PKey`, in `namespace pk_reference` |
| `src/engine/crypto/pk_test.cc`, `pk_differential_test.cc` | New                                         |
| `src/engine/crypto/mp_test.cc`                            | Deleted                                     |
| `src/engine/file/ini_file.cc`, `src/tools/game_data.cc`   | `SetFastExponent()`                         |
| `src/tools/kernel_bench.cc`                               | Two rows                                    |

## Tasks

- [x] `pk` module with cargo tests: every DER form and every malformed one, the 512-bit encoding,
      the refusals, a block with the real MIX key against Python's `pow()`, key pairs both ways
- [x] Bridge, `PKey`, reference files
- [x] Differential test (below)
- [x] `pk_stream_test` and `mix_archive_test` unchanged
- [x] Both games open their encrypted MIX files: `tools/ui_check.py` (every script of both games)
      and both save/load smoke scripts report no differences

## The differential test, and what it found

`pk_differential_test.cc` loads the same DER numbers into `PKey` and `pk_reference::PKey` and
requires the same block sizes, encodings and bytes from `Encrypt()` and `Decrypt()`:

- key pairs generated by the reference, used in Rust, and pairs generated by Rust, used in the
  reference (primes of 32 to 256 bits);
- 1,500 arbitrary moduli of 5 to 64 bytes, odd and even, with arbitrary exponents;
- 40 numbers of up to the whole register, about one in eight of them negative;
- 3,000 malformed DER inputs on top of a valid key.

**The C++ exponentiation is occasionally wrong.** Over the arbitrary moduli, 8 blocks of 9,000 come
out different (3 with the weaker generator the test first had). In each, Rust's result is the power:
for the first one found, Python's `pow()` agrees with it, and for every one so does squaring and
multiplying with the reference's own long division (`PowerByLongDivision()` in the test). The C++
result is not even congruent to the power. The fault is somewhere in `XMP_Mod_Mult()`'s reduction
(Smith's method over 16-bit half-digits with a correction step it calls rare); it was not tracked
down, as the code is leaving. One such block is kept as `TheReferenceGetsThisPowerWrong`.

So this phase is not byte-identical to the C++ on every input, by design: where the two differ the
test requires long division to side with Rust and allows at most 20 such blocks. No shipped file is
affected: every MIX archive of both games opens, as it did with a C++ that would have failed to open
one whose key block it computed wrongly.

Two more differences, both where the C++ stopped the program:

- A modulus of 32 bits or fewer made `XMP_Prepare_Modulus()` fail a bounds `CHECK`. Rust computes
  the power. The differential test keeps to moduli the reference can take.
- While writing the arbitration, the reference's `%` by a power of two disagreed with Python once.
  It is not used; the test compares bytes instead. Not investigated.

## Review

A fresh reviewer read the whole branch and fuzzed the kernels independently (860,000 rounds over the
XOR delta, VQ, movie ADPCM and `PKey` with moduli of 32 bits or fewer against a 64-bit model): no
mismatch and no bug in a shim or a kernel. What it did find, fixed in the commit after this phase:

- `base::Lcg` handed out the low bits of a 32-bit generator, so every "random" input of every
  differential test was a window of one 64 KiB string. It is now a 64-bit generator that hands out
  its high bits. Every comparison still holds; this test's count of reference errors went from 3
  to 8.
- `PKey::Generate(random, bits, key, key)` would have given Rust two mutable references to one
  array. It generates into numbers of its own now, and `CHECK`s that the random source filled the
  seed.
- The XOR delta test never used a long count above 255 and the VQ test never an entry above 255;
  both do now, and the VQ tests with sizes that are off count how many frames still draw.

Not fixed: about three quarters of this test's random blocks for `Decrypt()` are at or above the
modulus and compare the refusal value; powers are mostly exercised through `Encrypt()`.

## Outcome

The C++ bignum (`mp_reference_test_util`, `int_reference_test_util.h`,
`digit_cursor_reference_test_util.h`), the old `PKey` (`pk_reference_test_util`),
`PkDifferentialTest` and `kernel_bench` were deleted on 2026-10-15 (T-360). Their last run, that
day: fast exponent 7.03 against 1.19 MB/s, slow exponent 1.14 against 0.03. The block the C++ got
wrong is now a plain expected-value test in `pk_test.cc`, `EncryptsABlockTheOldBignumGotWrong`.

`kernel_bench` at `RelWithDebInfo` (GCC 13 for the C++ reference), a pair of 256-bit primes, in
megabytes of plain text:

| Operation                     | Rust      | C++       |
| ----------------------------- | --------- | --------- |
| fast exponent (opening a MIX) | 6.38 MB/s | 1.08 MB/s |
| slow exponent (making one)    | 1.02 MB/s | 0.02 MB/s |

The bignum was 24% of a Debug build's load CPU and the last crypto file under `optimize_in_debug()`;
`engine/crypto` no longer uses it.
