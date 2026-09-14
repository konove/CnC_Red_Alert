# Plan: replace the FileClass inheritance chain with composition

## Resume checkpoint (2026-09-14)

- Step 0 is complete: `tech/rawfile_test.cc`, `tech/cdfile_test.cc` and the nested-mixfile,
  loose-override and missing-name handle tests in `ra/mix_aware_file_test.cc`. Baselines:
  `tools/ra_saveload_smoke.sh` → 240 object positions identical;
  `tools/td_saveload_smoke.sh ... SCG01EA --team` → 5951 game states identical.
- Pinned quirks worth knowing: a `RawFileClass` seek to before the start of the file leaves the
  position unchanged, whereas a resident `MixAwareFile` clamps to 0; the lowercase retry lowercases
  the whole path, so it only finds all-lowercase files.
- Step 1 is complete (`aadf673c` BufferIOFileClass removed, `1bc0ec62` dead interface removed,
  `d593a6e1` read-write open no longer truncates). Smoke baselines unchanged.
- Next: step 2.

## Context

This document rebuilds the file classes so that each concern is its own part instead of a level in
one chain:

```
FileClass → RawFileClass (FILE* + Bias) → BufferIOFileClass → CDFileClass (search paths) → MixAwareFile / CCFileClass
```

The chain forces a fixed layer order, needs qualified base calls to steer around overrides, hides
disk I/O inside `SetName`, and makes nested mixfiles depend on an undocumented
`SetName`-then-`Bias()` ordering plus `data_start_` absorbing `bias_start()`
(`src/tech/mixfile.h:252`). RA and TD each carry their own copy of the mixfile logic.

Requirements beyond the structural change: **typed I/O instead of `void*`**, **`base::ssize` for
sizes and positions**, and **better class names** (`FileClass`, `CCFileClass` say nothing).

## Facts verified against the code

| Finding                                                                                                                                                                                                                                                                                                                          | Effect                                                                                                  |
| -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------- |
| `RawFileClass::Open` always returns `true` (`rawfile.cc:187-231`); every `Open` in the chain does; only `RAMFileClass::Open` can fail. All 13 sites that check the result handle `false` sensibly (RecordFile flags, `Load_Uncompress` → 0, `MixFileVqaIo::Open` → 1, `FileStraw::Get` → 0). Unchecked sites rely on `IsOpen()`. | `Open` reports failure from the facade step on, for `DiskFile` too.                                     |
| Consumers of `FileClass&` (FileStraw, FilePipe, INI, jshell loaders, PCX writer) use only `IsOpen, IsAvailable, Open(FileAccess), Read, Write, Seek, Size, Close`. `MixFileClass<T>` also needs ctor from `string_view`, `FileName`, `Size`, `Bias/bias_start`, `Error(int)`.                                                    | Small interface; `Bias` is the only tie to `RawFileClass`.                                              |
| `IsAvailableStrict()` has zero callers; `kBlocking` path dead. `Read_Object`/`Write_Object` (`ra/saveload.h:29-32`) declared, never defined. `td/ccfile.cc` has `#ifdef NEVER` blocks. `Error()` is called by consumers only at `mixfile.h:312` (a no-op in every override).                                                     | Remove in step 1; `Error` leaves the interface in step 3.                                               |
| `BufferIOFileClass` users: `ra/saveload.cc:467,1273` (never `Cache()` → pass-through) and `Cache(200000)` at `ra/conquer.cc:1893`, `ra/gadget.cc:496` (PCX dumps). No `Free()`/`Commit()` callers.                                                                                                                               | Removal confirmed.                                                                                      |
| `kReadWrite` opens as `"w+b"` (`sdllib/file.cc:26`) and truncates; the one user is TD `SuperRecord` (`td/conquer.cc:3298` open then seek END = append).                                                                                                                                                                          | Real bug; fix early.                                                                                    |
| Lowercase retry lives in `RawFileClass::DoIsAvailable` and renames the object; 31 files use `RawFileClass` directly on loose names.                                                                                                                                                                                              | Case-insensitive lookup becomes a shared helper used by `DiskFile` and `SearchPaths`.                   |
| TD registers no nested mixfiles; RA finds `CONQUER/GENERAL/MOVIES1/MOVIES2/SCORES/SPEECH.MIX` inside `MAIN*.MIX` (`ra/init.cc:2384-2449`); `GENERAL`, `MOVIES*`, `SPEECH` never cached.                                                                                                                                          | TD's `CCFileClass` nesting bug is unreachable; RA movies/music are the production nested-uncached test. |
| `MixFileClass::Cache` uses `file.Bias(0); file.Bias(data_start_)` (`mixfile.h:305-306`).                                                                                                                                                                                                                                         | Replace with `Seek(data_start_, SeekOrigin::kBegin)`.                                                   |
| `sdllib/wsa.cc:122` never checks the handle and reads into an uninitialised header; TD's handle table rejects only `-1` (out-of-bounds reachable); RA returns `WWERROR`, TD `kInvalidHandle`.                                                                                                                                    | Fix `wsa.cc`; one bounds-checked handle API; `kInvalidHandle` everywhere.                               |
| Tests: `tech` = one aggregate (`src/tech/CMakeLists.txt:21-27`, append to `SOURCES`); RA = one executable per test with game-symbol stubs (`src/ra/CMakeLists.txt:181-186`, stubs `ra/mix_aware_file_test.cc:26-31`); RA test hand-builds an unencrypted mix image (`MixImage()` `:51-63`).                                      | New tests follow these patterns.                                                                        |
| Call-site volume: ~231 `.Read(`/`.Write(` on file objects, 58 of the `Read(&x, sizeof x)` form, 16 `.Seek(`; 52 `FileClass&`/`*` declarations. No existing symbols named `File`, `DiskFile`, `MemoryFile`, `GameFile`, `MixArchive`, `SeekOrigin`.                                                                               | Typed-I/O migration is a regex sweep for the 58 sites plus adapters for the rest; names are free.       |

## Target design

### Names

| Today                                              | New                                                                              | Why                                                                                                                                                         |
| -------------------------------------------------- | -------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `FileClass` (`tech/wwfile.h`)                      | `File` (`tech/file.h`)                                                           | The abstract interface; Google style, no `Class` suffix.                                                                                                    |
| `RawFileClass` (`tech/rawfile.*`)                  | `DiskFile` (`tech/disk_file.*`)                                                  | Says where the bytes live.                                                                                                                                  |
| `RAMFileClass` (`tech/ramfile.*`)                  | `MemoryFile` (`tech/memory_file.*`)                                              | Same.                                                                                                                                                       |
| `BufferIOFileClass`, `CDFileClass`                 | removed                                                                          | Buffering unused; search paths become `SearchPaths`.                                                                                                        |
| `MixAwareFile` (RA), `CCFileClass` (TD)            | `GameFile` (`tech/game_file.*`), shared                                          | One implementation: a `File` that resolves a name through search paths and mixfile archives. The CD prompt is an injected handler, so no per-game subclass. |
| `MixFileClass<T>` (`tech/mixfile.h`), alias `MFCD` | `MixArchive` (non-template, opens through `GameFile`)                            | Only `GameFile` will instantiate it; "archive" is what it is. `MFCD` alias kept until the last step.                                                        |
| `AvailabilityCheck`, `WWERROR`                     | removed (`kInvalidHandle` from `sdllib/wwstd.h:40`)                              | Dead / duplicate.                                                                                                                                           |
| new                                                | `ByteStream`, `DiskStream`, `MemoryStream`, `RangeStream` (`tech/byte_stream.*`) | The composable byte sources inside `GameFile`.                                                                                                              |
| new                                                | `SearchPaths` (`tech/search_paths.*`)                                            | Registry of directories + `Resolve(name)`.                                                                                                                  |
| new                                                | `enum class SeekOrigin { kBegin, kCurrent, kEnd }`                               | Replaces raw `SEEK_*` ints.                                                                                                                                 |

### Typed I/O and `base::ssize`

```cpp
class File {
 public:
  virtual ~File() = default;
  [[nodiscard]] virtual std::string_view FileName() const = 0;
  virtual void SetName(std::string_view) = 0;
  virtual bool Open(FileAccess = FileAccess::kRead) = 0;
  bool Open(std::string_view name, FileAccess = FileAccess::kRead);   // non-virtual: SetName + Open
  [[nodiscard]] virtual bool IsOpen() const = 0;
  virtual bool IsAvailable() = 0;
  virtual bool Create() = 0;
  virtual bool Delete() = 0;
  virtual void Close() = 0;

  // The only I/O virtuals. Return the number of bytes transferred.
  virtual base::ssize Read(std::span<std::byte> buffer) = 0;
  virtual base::ssize Write(std::span<const std::byte> buffer) = 0;
  virtual base::ssize Seek(base::ssize offset, SeekOrigin origin = SeekOrigin::kCurrent) = 0;
  virtual base::ssize Size() = 0;

  // Typed helpers (non-virtual). T must be trivially copyable.
  template <typename T> bool ReadObject(T& value);            // false on short read
  template <typename T> bool WriteObject(const T& value);
  std::vector<std::byte> ReadBytes(base::ssize count);        // shorter on EOF
  std::string ReadString(base::ssize count);

  // Transitional adapters, removed in the last step.
  base::ssize Read(void* buffer, base::ssize size);
  base::ssize Write(const void* buffer, base::ssize size);
};
```

`ByteStream` has the same four I/O methods (span-based, `base::ssize`, `SeekOrigin`), no names.
`FileStraw`/`FilePipe` keep their `void*` `Get`/`Put` (Straw/Pipe are out of scope) and call the
span overloads internally.

### Composition

- `GameFile : File` holds `std::string name_` and `std::unique_ptr<ByteStream>`. `Open`: write
  access or loose file found → `DiskStream`; cached archive entry → `MemoryStream`; uncached →
  `RangeStream(OpenStream(parent archive name), offset, size)` recursively. `Open` returns whether a
  stream was obtained. Archive offsets are relative to the archive's own start.
- `GameFile::SetMissingMediaHandler(std::function<bool()>)`: called once when a read-open finds
  nothing; RA installs `[] { return Force_CD_Available(RequiredCD); }` and the open retries if it
  returns true. Replaces the `Error()` overrides.
- `DiskFile` and `MemoryFile` stay as concrete `File`s (31 and 1 direct users); in the last step
  they wrap `DiskStream`/`MemoryStream`.
- `SearchPaths`: today's `CDFileClass` statics +
  `std::optional<std::string> Resolve(std::string_view)`; a shared `FindCaseInsensitive(path)`
  helper is used by `Resolve` and `DiskFile::IsAvailable`.

## Steps

Each step is one or more commits; every commit passes: strict build
(`timeout 570 cmake --build cmake-build-strict-ra-clang --parallel 4 -- -k 0` until
`ninja: no work to do`), `ctest` in `build/`, `tools/ra_saveload_smoke.sh`,
`tools/td_saveload_smoke.sh`. Renames are compiler-driven and formatted with `git clang-format` (see
memory `renaming-shared-method-names`).

### Step 0 — Characterization tests

- `src/tech/rawfile_test.cc`, `src/tech/cdfile_test.cc` (append to `tech_test` SOURCES): implicit
  open on Read/Write; `Bias` clamp/size/`SetName` clears; lowercase retry; search order (cwd, then
  registration order, verbatim when absent, `SetSearchEnabled(false)`, empty name).
- `src/ra/mix_aware_file_test.cc`: uncached mix nested in uncached mix on disk (mix image inside a
  mix image's data section); cached nested in uncached; loose file overrides packed;
  `OpenFileHandle` on a missing name (today: returns a slot whose `IsOpen()` is false).
- Record baseline smoke output.

### Step 1 — Dead surface and small bugs (separate commits)

1. Remove `BufferIOFileClass`; `CDFileClass : RawFileClass`; `ra/saveload.cc` → `RawFileClass`; drop
   both `Cache(200000)` calls. Check a screenshot PCX is byte-identical.
2. Remove `IsAvailableStrict`, `AvailabilityCheck`, the `DoIsAvailable(mode)` indirection (make
   `IsAvailable()` the virtual); delete `Read_Object`/`Write_Object` declarations and the
   `#ifdef NEVER` blocks in `td/ccfile.cc`; drop `file.Error(EIO)` in `mixfile.h`.
3. `IO_Open_File`: `kReadWrite` → `"r+b"`, fall back to `"w+b"` when absent; test.

### Step 2 — Rename the classes that survive

- `FileClass`→`File` (`tech/wwfile.h`→`tech/file.h`), `RawFileClass`→`DiskFile`
  (`tech/disk_file.*`), `RAMFileClass`→`MemoryFile` (`tech/memory_file.*`); update includes,
  `.clang-tidy` allowlists, `mixfile_test.cc`. Mechanical, one commit per class. `CDFileClass`,
  `MixAwareFile`, `CCFileClass`, `MixFileClass` are renamed when replaced (steps 6–8).

### Step 3 — Modernize the `File` interface

- `base::ssize` for `Read`/`Write`/`Seek`/`Size` returns and size parameters in every override.
- `SeekOrigin` enum; `SEEK_*` ints go (16 `.Seek(` sites plus the internals).
- Span-based `Read`/`Write` become the virtuals; `void*` overloads become non-virtual adapters. Add
  `ReadObject`/`WriteObject`/`ReadBytes`/`ReadString`. Convert the 58 `Read(&x, sizeof x)` sites to
  `ReadObject(x)` (regex sweep, then compile). `Error()` leaves the interface (no callers after step
  1; internal `DiskFile` uses are dropped — `Open` failure is the signal).
- Tests for the typed helpers on `MemoryFile`.

### Step 4 — Extract `SearchPaths`

- New `tech/search_paths.{h,cc}` + tests (temp dirs; case-insensitive fallback; `?:` with stubbed
  `Get_CD_Index`). `CDFileClass::SetName` →
  `DiskFile::SetName(SearchPaths::Resolve(name).value_or(name))`.
- Move static callers (RA: `conquer.cc`, `wol_gsup.cc`, `startup.cc`, `init.cc`; TD: `startup.cc`,
  `conquer.cc`, `init.cc`) from `MixAwareFile::`/`CCFileClass::`/`CDFileClass::` to `SearchPaths::`.

### Step 5 — Byte streams (no users yet)

- `tech/byte_stream.{h,cc}` + `byte_stream_test.cc`: each stream; range of a range; seeks past both
  ends; `DiskStream::Open` failure → null.

### Step 6 — `GameFile` (core; replaces RA's `MixAwareFile`)

- New `tech/game_file.{h,cc}` per the design; RA switches (`using MixAwareFile = GameFile;` for one
  commit, then rename call sites). Delete `ra/mix_aware_file.*`; the test moves to
  `tech/game_file_test.cc` (needs no game stubs any more — the handler is injected).
- `mixfile.h`: `data_start_ = file.Seek(0, kCurrent)`; `Cache` seeks instead of biasing. Remove
  `Bias`, `bias_start`, bias fields, `RawSeek` from `DiskFile`.
- `DiskFile::Open` and `GameFile::Open` return real results; the handle API returns
  `kInvalidHandle`; fix `sdllib/wsa.cc:122` (check handle, zero-init header); update step-0 tests.
- Manual run with game data: intro movie, in-game music.

### Step 7 — TD onto `GameFile`

- Write the nested-uncached TD test first (fails on `CCFileClass`). TD installs its handler
  (`Force_CD_Available` + `Prog_End`); `td/externs.h` `MFCD`, `td/globals.h` `TheaterIcons`, ~34
  files: `CCFileClass`→`GameFile`. One handle API in `tech/` (RA's bounds-checked
  `OpenFileForHandle`); `WWERROR`→`kInvalidHandle`. Delete `td/ccfile.*`.

### Step 8 — Finish

- Remove `CDFileClass` (`ra/conquer.cc:1892`, `ra/gadget.cc:482` → `DiskFile`).
- `MixFileClass<T>`→`MixArchive` non-template over `GameFile`; `MFCD` alias removed.
- `DiskFile` over `DiskStream`, `MemoryFile` over a writable memory stream.
- Migrate the remaining `void*` `Read`/`Write` call sites (batches by directory) and delete the
  adapters. Add `std::unique_ptr<ByteStream> OpenGameFile(std::string_view)` for new code.

## Risks

| Risk                                                              | Mitigation                                                |
| ----------------------------------------------------------------- | --------------------------------------------------------- |
| Nested uncached archives read wrong bytes (RA movies/music)       | Step-0 nested fixture; manual run in step 6               |
| Case-insensitive lookup lost for direct `DiskFile` users on Linux | Shared helper; step-0 test                                |
| `Open` failing surprises a caller                                 | All checking sites audited; unchecked ones use `IsOpen()` |
| Wide renames/type changes break the build mid-way                 | Compiler-driven, one class or one type change per commit  |
| Save/load regressions via FilePipe/FileStraw                      | Both smoke scripts every commit                           |
| Strict rebuild OOM-killed                                         | Foreground `timeout 570`, `-j4`                           |

## Out of scope

Save format; `Straw`/`Pipe` signatures; the `sdllib/file.h` handle API beyond return codes.
