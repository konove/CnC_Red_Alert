# Plan: modernize the Pipe/Straw streams into ByteSink/ByteSource with shared codec cores

## Resume checkpoint (2026-09-14)

- Complete (2026-09-14). Every step passed the strict build, ctest (426 tests), both smoke scripts
  (240 RA object positions, 5951 TD game states identical) and the old-save fixture.
- Commits: `1bbefc04` stray IWYU tweak, A0 `d356e9be`, A1 `434f36a3`, A2 `2f11c15f`, A3 `41f7317a`,
  A4 `b25297a8`, A5 `b4b3386c`, A6 `482af0b3`, A7 `ceb86fbb`, B1 `9b59eb2f`, B2 `f6d877ef`, B3
  `7987ae53`.
- Where the result differs from the text below:
  - `BlowfishSink/BlowfishSource` and `Sha1Sink/Sha1Source` are thin subclasses of the adapters, not
    aliases, so that `Key()` and `digest()` stay on the link; the LZO, LCW, LZW and Base64 names are
    aliases. The sink and source headers keep their paths, so no include changed in Tier B; the
    per-codec `.cc` files are gone.
  - `codec_block.h` became `block_codec.h`, holding `CodecMode`, `BlockHeaderFits`, the
    `BlockBackend` concept and `BlockCodec`; the backends live in `tech/block_backends.{h,cc}`.
    `CipherMode` is in `tech/blowfish.h`, `Base64Mode` in `tech/base64.h`, the `ByteCodec` concept
    in `tech/byte_codec.h`, `VectorSink` in `tech/vector_sink.h`. `SpanSink/SpanSource` are
    header-only.
  - clang's lifetime analysis cannot verify `ABSL_ATTRIBUTE_LIFETIME_BOUND` on a derived constructor
    that forwards to `ChainedSink/ChainedSource`, nor through `make_unique` in the PK factories, so
    only the base constructors, `VectorSink`, `SpanSource` and `codec()` carry it.
  - `Save_Game` returns `pipe.Finish() && tee.ok()`: `Finish` runs first so the file is closed even
    when the tee has failed.
  - A3 used a transitional `tech/byte_view.h` for the codec internals; B3 deleted it.
  - Behaviour changes beyond the planned truncated-decode failure: `FileSource` fails when its file
    is unavailable or cannot be opened; `TransformSource` stops reading after its source's first
    short read (the old straws asked again); the old `Base64Straw` stopped at a group that decoded
    to no bytes, whereas the adapter carries on like the pipe did (malformed input only).
  - Test-local sinks named `ByteSink`/`VectorPipe` became `RecordingSink`.
- Left alone: `XMP_Randomize` (`tech/mp.cc`) reads `total_bits / 8 + 1` bytes into the digit array,
  one byte past its end when `total_bits == precision * 32`; it predates this plan and no caller in
  the game reaches that case.

## Context

`src/tech` carries Westwood's 1996 streaming layer: `Pipe` (push, `Put`) and `Straw` (pull, `Get`)
links chained on the stack — RA saves go `FilePipe ← SHAPipe ← BlowPipe ← LZOPipe ← TeePipe`,
mixfiles `FileStraw ← BlowStraw`, INI binary blocks `Base64`, map packs `LCW`. The composition idea
is good and `ArchiveWriter/ArchiveReader` already sit on it, but the API is legacy: `void*`+`int`
everywhere; `Put` returns "bytes that reached the far end" so codecs legitimately return short and
callers ignore it (an unchained `Pipe::Put` returns `length` as success; TD needed `SaveGamePipe` to
notice short writes); `Flush` vs `End` is fuzzy; links hold raw `sink_/source_` with no unlinking
(`ra/saveload.cc` does `sha.SetSource(nullptr)` by hand); raw `new[]`, `new BlowfishEngine`, LZO
allocates a 64 KB work buffer per 8 KB block; `SHAPipe::Result(void*)` is unsized; every codec
exists twice (Pipe + Straw = 14 classes for 7 transforms) and the halves drift. Dead: `ra/cstraw.*`,
`CRCPipe/CRCStraw`, `MixArchive::Register`'s unused `RandomStraw*`, and with it the global
`RandomStraw CryptRandom` (`ra/globals.cc:358`, `externs.h:168`): its only consumers are `Register`
and two `Seed_Byte` calls (`menus.cc:747`, `init.cc:1805`).

Save files on disk must stay byte-identical (LZO block format, Blowfish, SHA digest placement,
Base64).

## Facts that shape the design (verified)

- `Put`'s count is load-bearing in 4 places, all really asking "how many bytes landed in the
  `BufferPipe`": `ra/map.cc:1093` (`Write_Binary` sums `comp.Put`), `ra/display.cc:4467`,
  `ra/overlay.cc:366`, `ra/ini.cc:590` (`Get_UUBlock`), `tech/lzw.cc:53,101`. A `bytes_written()` on
  the terminator answers that. A fifth site, `INIClass::Save(Pipe&)` (`ra/ini.cc:342-367`, forwarded
  by `CCINIClass::Save`), sums `Put` into an `int` return with no `BufferPipe` under it; all four
  `Save(File&)` callers (`session.cc:911`, `scenario.cc:2299`, `startup.cc:327`, `options.cc:742`)
  ignore the value, so it becomes `bool`.
- RA `Save_Game` (`ra/saveload.cc:541-548`) calls `pipe.Flush()`, seeks, writes the SHA digest
  straight into `fpipe`, then `pipe.End()`, and returns `true` unconditionally. The digest covers
  the encrypted stream, so the Blowfish tail and the LZO partial block must be emitted by `Flush`
  (as today), and the later `End`/`Finish` must emit nothing. Section tags are written through the
  raw pipe next to the `ArchiveWriter` (`Put_Section`, `saveload.cc:226`), and `Put_All` is
  `static void`, so write failure has to be observable on the sink, not only on the writer.
- `TeePipe` takes a nullable copy sink (`saveload.cc:537` passes `nullptr` when `RA_SAVE_DUMP` is
  not open).
- Pull-side links read exactly what they need: `LZOStraw` reads the 4-byte header then `CompCount`
  bytes (`lzostraw.cc:168,181`), `BlowStraw` reads 8 at a time (`blwstraw.cc:107`). The source
  position after a decode is observable (`MakePKDecryptStraw` over the mixfile header,
  `mix_archive.cc:67`) and no golden covers it.
- `sha.h:74` already defines a `SHADigest` struct; `BufferPipe/BufferStraw` live in
  `tech/xpipe.h`/`xstraw.h` next to `FilePipe/FileStraw`.
- `Write_Binary`/overlay `Write_INI` never `Flush()` their `LCWPipe`; it works only because payloads
  are exact multiples of the 8192 block. Adding `Finish()` emits nothing → byte-identical.
- `SHAPipe` is used with no sink in `ra/ccini.cc:1520` (`Calculate_Message_Digest`) → needs
  `NullSink`.
- `MakePKEncryptPipe` has no production caller; `PKey::Generate`/`Generate_Prime`/`XMP_Randomize`
  are unreachable in the game (keep for the PK test).
- LZO/LCW/LZW pipe+straw bodies share one skeleton (accumulate → `BlockHeaderFits` → codec → emit);
  only the codec call and `SafetyMargin` differ. Blowfish and Base64 are symmetric at EOF (tail
  passes through unencrypted / short group padded). Only asymmetry: truncated decode input
  (`LZOPipe::Flush` dumps the partial block raw, `LZOStraw::Get` stops) — untested, corrupt files
  only.
- `File` (`tech/file.h`) already went through this migration: span virtuals, `base::ssize`,
  `ReadObject/WriteObject<T>`. Reuse the vocabulary.

## Target design

### Names (Tier A); in Tier B the codec ones become aliases, so B changes no call site

| today                                               | new                                                                                                              | file                                               |
| --------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------- | -------------------------------------------------- |
| `Pipe` / `Straw`                                    | `ByteSink` / `ByteSource` (+ `ChainedSink` holding `ByteSink& next_`)                                            | `tech/byte_sink.{h,cc}`, `tech/byte_source.{h,cc}` |
| `BufferPipe` / `BufferStraw` (`xpipe.h`/`xstraw.h`) | `SpanSink` / `SpanSource`                                                                                        | `tech/span_sink.h`, `tech/span_source.h`           |
| `FilePipe` / `FileStraw` (`xpipe.*`/`xstraw.*`)     | `FileSink` / `FileSource`                                                                                        | `tech/file_sink.{h,cc}`, `tech/file_source.{h,cc}` |
| —                                                   | `NullSink`                                                                                                       | `tech/byte_sink.h`                                 |
| `TeePipe`                                           | `TeeSink(ByteSink& next, ByteSink* copy)` — copy stays nullable                                                  | `tech/tee_sink.h`                                  |
| `RandomStraw`                                       | `RandomSource` (tests + `PKey::Generate` only; `CryptRandom` global deleted)                                     | `tech/random_source.{h,cc}`                        |
| `SHAPipe` / `SHAStraw`                              | `Sha1Sink` / `Sha1Source`                                                                                        | `tech/sha1_sink.h`, `tech/sha1_source.h`           |
| `LZOPipe/LZOStraw`, `LCW…`, `LZW…`                  | `LzoSink/LzoSource`, `LcwSink/LcwSource`, `LzwSink/LzwSource`; `enum class CodecMode { kCompress, kDecompress }` | `tech/lzo_sink.*` …                                |
| `Base64Pipe/Straw`                                  | `Base64Sink/Base64Source`; `enum class Base64Mode { kEncode, kDecode }`                                          |                                                    |
| `BlowPipe/BlowStraw`                                | `BlowfishSink/BlowfishSource`; `enum class CipherMode { kEncrypt, kDecrypt }`                                    |                                                    |
| `MakePKEncryptPipe/MakePKDecryptStraw`              | `MakePkEncryptSink` / `MakePkDecryptSource`                                                                      | `tech/pk_sink.*`, `tech/pk_source.*`               |
| `SaveGamePipe` (td)                                 | deleted; `ArchiveWriter::ok()`                                                                                   |                                                    |

### Contract

```cpp
class ByteSink {
  // true = every byte accepted (buffered or forwarded). Once anything downstream fails,
  // ok() is false and every later Write returns false (sticky).
  virtual bool Write(std::span<const std::byte> bytes) = 0;
  virtual bool Flush();    // emit everything buffered (LZO partial block, Blowfish tail, Base64
                           // padding) all the way to the terminator; the chain stays usable and
                           // a second Flush with nothing buffered emits nothing
  virtual bool Finish();   // Flush + terminal cleanup (FileSink closes a file it opened); last call
  [[nodiscard]] bool ok() const;
  template <trivially copyable T> bool WriteObject(const T&);
};
class ByteSource {
  // Bytes stored; short only at end of data or after an error, which ok() distinguishes.
  virtual base::ssize Read(std::span<std::byte> buffer) = 0;
  [[nodiscard]] bool ok() const;
  template <trivially copyable T> bool ReadObject(T&);
};
```

- Links are **non-owning, constructor-injected**:
  `LzoSink(CodecMode, ByteSink& next ABSL_ATTRIBUTE_LIFETIME_BOUND, int block_size)`;
  `SetSink/SetSource` deleted. Chains are built inside-out on the stack as today. PK factories keep
  `unique_ptr` returns with a lifetimebound `next`.
- Supported sequence (RA `Save_Game`): `Flush()` a middle link, write directly to the terminator,
  then `Finish()` the chain. `Flush` guarantees every byte reached the terminator.
- Terminators expose position: `SpanSink::bytes_written()` (store what fits, then fail),
  `SpanSource::bytes_remaining()`.
- `FileSource::ok()` distinguishes clean EOF from read failure. Preserve `IO_Read_File`'s error
  result through `DiskStream`/`ByteStream` and `DiskFile`/`File` (including other implementations),
  then propagate it through source links; a byte count alone is insufficient.
- `Sha1Digest = std::array<std::byte, 20>` in `tech/sha.h` **replaces** the existing `SHADigest`
  struct (`sha.h:74`); `SHAEngine::Digest()`, `Sha1Sink::digest()`.
- Buffers: `std::vector<std::byte>` sized once in ctors; `std::array<std::byte, 8>` (Blowfish);
  `std::optional<BlowfishEngine>`; LZO work buffer one member.
- `ArchiveWriter(ByteSink&)` gains `ok()` that **returns the sink's sticky `ok()`** (RA writes
  section tags past the writer); `Scalar` → `WriteObject`; `Raw/Bytes` take spans;
  `ArchiveReader::Raw` → `Read(span) != ssize(span)`.
- RA `Save_Game` returns `tee.ok() && pipe.Finish()` instead of unconditional `true`.
- Decoders on truncated input at `Finish`: drop the partial block and clear `ok()` in both
  directions (deliberate change; corrupt files only; `ArchiveReader` already fails on the short
  read).

### Tier B shape

```cpp
template <class C> concept ByteCodec = requires(C c, std::span<const std::byte> in,
                                               ByteSink& out, base::ssize output_needed) {
  { c.Process(in, out) } -> std::same_as<bool>;   // consume input, push complete output
  { c.Flush(out) } -> std::same_as<bool>;          // emit what is still buffered; codec stays usable
  { c.ok() } -> std::same_as<bool>;
  { c.BytesWanted(output_needed) } -> std::convertible_to<base::ssize>;  // input needed before the next output
};
template <ByteCodec C> class TransformSink : public ChainedSink { ... };
template <ByteCodec C> class TransformSource : public ByteSource { ... };  // pending_ vector + cursor
using LzoSink = TransformSink<LzoCodec>; using LzoSource = TransformSource<LzoCodec>;
```

`TransformSink::Flush` calls `c.Flush(out)` then `next_.Flush()`; `Finish` is `Flush` +
`next_.Finish()`. After draining pending output, `TransformSource::Read` asks
`c.BytesWanted(output_needed)` with the caller's remaining demand (block size for compress; header
size, then `CompCount`, for decompress; 8 for keyed Blowfish; 4 for Base64 decode), reads exactly
that from its source, then `Process` into `pending_` via a `VectorSink`; on a short read calls
`c.Flush` once. A fixed chunk would over-read the source and change the file position after a
mixfile header decode. SHA-1 and unkeyed Blowfish request exactly `output_needed`, preserving
pass-through source position and SHA's digest after a partial read. LZO/LCW/LZW collapse further
into `BlockCodec<Backend>` (block accumulation, `{uint16 comp, uint16 uncomp}` header,
`BlockHeaderFits`, corrupt flag) with backends `Compress/Decompress/WorstCase`; `codec_block.h` →
`block_codec.h`. Blowfish: 8-byte accumulator, tail passthrough at `Flush`. Base64: 3↔4 grouping,
`Flush` pads. SHA-1: tap (`Process` = hash + forward).

## Steps (each commit: strict build, ctest, both smoke scripts green)

Verification per commit:
`timeout 590 cmake --build cmake-build-strict-ra-clang --parallel 10 -- -k 0` until `no work to do`;
`ctest --test-dir cmake-build-strict-ra-clang --output-on-failure`; `tools/ra_saveload_smoke.sh`;
`tools/td_saveload_smoke.sh cmake-build-strict-ra-clang/src/td/tdsdl SCG01EA --team`.

### A0 — Pin today's bytes (tests only)

- `tech/stream_golden_test.cc`: LCG-generated 100 KB input through (a)
  `LZOPipe(COMPRESS, SAVE_BLOCK_SIZE)`, (b) `BlowPipe` with a fixed key, (c) the RA chain
  `SHA←Blow←LZO`, (d) `Base64Pipe` + `Put_UUBlock`-style 70-char lines; assert hard-coded SHA-1
  digests captured from today's build. Also `Base64` decode of the golden output round-trips.
- `tech/blowfish_stream_test.cc`: pipe→straw round trip, 8-byte alignment, <8-byte tail passes
  through unchanged both ways, no-key pass-through, one byte at a time.
- `tech/codec_state_test.cc`: pin source consumption after partial reads through unkeyed `BlowStraw`
  and `SHAStraw` to the caller's requested count; assert SHA's digest covers only the returned
  bytes, with unread bytes still present in the source.
- `tech/pk_stream_test.cc`: `PKey::Generate` small key from seeded `RandomStraw`,
  `MakePKEncryptPipe` → `MakePKDecryptStraw` round trip, header too short → `nullptr`.
- `tech/random_source_test.cc`: seeded generator deterministic, fills the requested count.
- Old-save fixture: save a `SAVEGAME.0NN` with today's `rasdl`
  (`-NEWGAMESCG01EA -QUITFRAME60 -SAVESLOT98`) into `src/ra/testdata/` (new directory; no fixture
  convention exists yet), and give `tools/ra_saveload_smoke.sh` a `--load-fixture` mode that loads
  it and checks it runs to frame 120 — the only proof of byte compatibility with existing saves. It
  needs the Steam game data, so it lives in the smoke script, not ctest. Regeneration rule, written
  next to the file: the fixture is valid only while `kSaveGameVersion` is unchanged; a version bump
  regenerates it with the first compatible binary for the new version. Record the generating commit
  and save version next to the fixture; retain the A0 baseline throughout this refactor while the
  version is unchanged.

### A1 — Delete dead code

`ra/cstraw.{h,cc}` + the two `REMOVE_ITEM` lines in `src/ra/CMakeLists.txt`; `tech/crcpipe.*`,
`crcstraw.*`; the `RandomStraw*` parameter of `MixArchive::Register` (defaulted, so only the ~33
callers passing `&CryptRandom` in `ra/init.cc`, `conquer.cc`, `display.cc` and td equivalents
change; none in tests); the `CryptRandom` global (`ra/globals.cc`, `externs.h`) and its two
`Seed_Byte` sites (`menus.cc:747`, `init.cc:1805`); commented-out LZW/LCW alternates in
`ra/saveload.cc`.

### A2 — Owned buffers, no API change

vectors/arrays/optional in
`lzopipe/lzostraw/lcwpipe/lcwstraw/lzwpipe/lzwstraw/blowpipe/blwstraw/b64pipe/b64straw`; LZO work
buffer becomes one member. Goldens prove identity.

### A3 — Typed signatures, same semantics, same names

`Pipe::Put(std::span<const std::byte>) -> base::ssize`,
`Straw::Get(std::span<std::byte>) -> base::ssize`, `WriteObject/ReadObject`;
`BufferPipe/BufferStraw` over spans (drops `Buffer` member and the const cast). Touches every
subclass, `archive.h`, and each call site once:
`ra/saveload.cc, ini.cc, ini.h, ccini.cc, ccini.h, map.cc, overlay.cc, display.cc, session.cc, iomap.cc, init.cc, heap.h`,
`tech/mix_archive.cc, lzw.cc, readline.cc, mp.cc, pkpipe.cc, pkstraw.cc`,
`td/saveload.cc, savepipe.h, conquer.cc` (two function-local pipes), 14 tests
(`tech/archive_test, codec_state_test, codec_corrupt_test, teepipe_test, lcw_comp_test, fixed_test, ftimer_test`,
`ra/heap_test, crate_test, saveglobals_test`,
`td/archive_roundtrip_test, savevalues_test, heap_test, saveload_test`). The `int total += Put(...)`
accumulators become `base::ssize` (else `-Wshorten-64-to-32` under strict checks); INI-layer `int`
returns get one `static_cast` at the boundary. No byte-sized `T*`+count adapters: spans at call
sites are short.

### A4 — Return contract, Flush/Finish

`Put` → `bool` + sticky `ok()`; `End` → `Finish` (`Flush` keeps today's emit-everything semantics,
see Contract); `BufferPipe::bytes_written()`; `ArchiveWriter::ok()` returning the sink's `ok()`;
delete `td/savepipe.h` and use `writer.ok()` in `td/saveload.cc` (move its test into
`archive_test.cc`); RA `Save_Game` returns `tee.ok() && pipe.Finish()`; rewrite the four
count-dependent sites to read `bytes_written()`; `INIClass::Save(Pipe&)` and `CCINIClass::Save` →
`bool` (callers ignore the count today); add `Finish()` to `Write_Binary`/overlay `Write_INI`;
`TeePipe::copy_ok()` via the copy sink's result; truncated-input decode behaviour unified + a
`codec_corrupt_test` case.

Add the file read-error propagation described in Contract before promising source `ok()`: preserve
the low-level error status through the file abstractions and `FileStraw`, then through each source
link. Test clean EOF versus read failure (including partial data followed by an error), and sticky
failure through a transform source.

### A5 — Constructor injection

Remove `SetSink/SetSource`, add `NullSink`; `TeeSink` keeps its nullable copy pointer; reorder chain
construction in `ra/saveload.cc` (both directions; scope the digest pre-pass in its own block,
deleting the `SetSource(nullptr)` hack), `ccini.cc` (`Sha1Sink sha(null)`), `map.cc`, `overlay.cc`,
`display.cc`, `ini.cc`, `mix_archive.cc`, PK factories (lifetimebound `next`), `td/conquer.cc` (the
two local terminators derive from `ByteSink` directly), tests.

### A6 — Typed SHA digest

`Sha1Digest` replaces `SHADigest` in `sha.h/.cc`; `shapipe/shastraw`, `ccini.{h,cc}` (`Digest`
member), `saveload.cc` (`memcmp` → `==`), `mix_archive.cc`, `rndstraw.cc`.

### A7 — Rename

Classes, enums, files (`git mv` to snake*case;
`xpipe.*`/`xstraw._`split into`span_\_`and`file\_\_`), the `Pipe&/Straw&` signature-only sites (`mouse.h,
map.h, session.h, ccini.h, ini.h, saveload.h, heap.h, readline.h, mp.h, int.h,
pk.h`), `CLAUDE.md`Pipe/Straw row,`docs/SAVEGAME_MIGRATION_PLAN.md`and`docs/FILE_IO_REFACTOR_PLAN.md`mentions. Pure rename commit, compiler-driven,`git
clang-format`.

### B1 — Adapters + Blowfish

`tech/transform_sink.h`, `tech/transform_source.h`, `VectorSink`; `BlowfishCodec`; delete
`blowfish_sink/source.*` in favour of aliases. `blowfish_stream_test` and goldens unchanged. Add a
`codec_state_test` case that the pull adapter consumes exactly as many source bytes as the old straw
did (header + `CompCount`, 8 for keyed Blowfish, 4 for Base64), since no golden sees the source
position. Keep A0's unkeyed Blowfish consumption test unchanged apart from API migration.

### B2 — Block codecs

`BlockCodec<Backend>` + `LzoBackend/LcwBackend/LzwBackend` (LZW wraps the legacy
`LZW_Compress/Uncompress`); delete six files; `codec_state_test`/`codec_corrupt_test`/
`lcw_comp_test` unchanged apart from headers.

### B3 — Base64 and SHA-1 codecs

`Base64Codec`, `Sha1Codec`; delete four files. Keep A0's SHA partial-read consumption and digest
test unchanged apart from API migration.

### B4 — Docs

Update this checkpoint, `docs/FILE_IO_REFACTOR_PLAN.md` cross-reference, memory note.

## Verification

- Each commit: strict build, `ctest` (goldens, codec identity, corrupt-input, archive, smoke).
- A0's goldens prove LZO/Blowfish/SHA/Base64 output identity through A2–B3; the old-save fixture
  proves header/digest placement; `tools/*_saveload_smoke.sh` prove save→load round trips.
- After B: `codec_state_test` proves push == pull for the adapters; goldens prove new == old.

## Risks

| Risk                                                                 | Mitigation                                                                     |
| -------------------------------------------------------------------- | ------------------------------------------------------------------------------ |
| Byte identity of saves/mixfile decrypt/INI blocks                    | A0 goldens + old-save fixture + smoke scripts                                  |
| `Get_UUBlock`/`INIClass::Save` count/int returns are INI-layer API   | keep counts via `bytes_written()`; check `Save` callers                        |
| Truncated-input decode behaviour changes (intended)                  | own corrupt test; only affects corrupt files                                   |
| Pull adapter over-reads its source (Tier B)                          | `BytesWanted(output_needed)` + source-consumption and partial SHA digest tests |
| `Flush` semantics drift from today (digest placement in `Save_Game`) | Contract pins Flush = emit everything; old-save fixture                        |
| Fixture goes stale on a `kSaveGameVersion` bump                      | regeneration rule next to the file                                             |
| Template diagnostics under `-Weverything`                            | `ByteCodec` concept                                                            |
| Big mechanical commit (A3, ~35 files)                                | compiler-driven; separate from the rename (A7)                                 |
