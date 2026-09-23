# Plan: delete the `File` interface and put disk I/O on the C++ library

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development
> (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use
> checkbox (`- [ ]`) syntax for tracking. Record progress in the `## Progress` section at the end as
> each step lands.

**Goal:** `ByteStream` becomes the only I/O interface in the tree: `tech/file.h`, `File`,
`MemoryFile`, `DiskFile`, `GameFile` (the classes), `FileSource` and `FileSink` are deleted, and
`sdllib/file.cc` stops calling the C library (`fopen`/`fread`/`fseek`/`unlink`/`fnmatch`/`statvfs`/
`getcwd`, and the Win32 `FindFirstFile` branch).

**Architecture:** An open file is a `std::unique_ptr<ByteStream>`, obtained from a free function
that resolves a name (`OpenGameFile` for the game's lookup rules, `OpenDiskFile` for a plain path).
Name-level questions (does it exist, how big, delete it) are free functions too. Code that reads or
writes takes an open `ByteStream&`; the typed helpers (`ReadObject`, `ReadBytes`, ...) move from
`File` to `ByteStream`. `DiskStream` sits on `std::filebuf`; directory listing, deletion and free
space come from `std::filesystem`.

**Tech stack:** C++23, `std::filebuf`, `std::filesystem`, `std::chrono::clock_cast`, Abseil,
GoogleTest (`add_gtest` in `cmake/Testing.cmake`).

**Spec:** the design discussion of 2026-09-22 (user chose "Delete File" and "All of
sdllib/file.cc"). The user-level requirements are restated in _Context_ and _Global Constraints_
below.

## Context

`File` (`src/tech/file.h`) is the abstract base of `DiskFile`, `GameFile` and `MemoryFile`. Since
the file I/O refactor (`docs/FILE_IO_REFACTOR_PLAN.md`) `DiskFile` and `GameFile` are a name plus a
`std::unique_ptr<…Stream>` they forward to, so `File` duplicates `ByteStream`'s
`Read`/`Write`/`Seek`/`Size`/`ok` and adds three things:

1. a name and file-system operations (`FileName`, `SetName`, `Create`, `Delete`, `IsAvailable`);
2. an open/close lifecycle (`Open`, `Close`, `IsOpen`) and **auto-open**: `Read`/`Write` on a closed
   `DiskFile`/`GameFile` opens the file, does that one call, and closes it again — a `kWrite`
   auto-open truncates;
3. typed helpers (`ReadObject`/`WriteObject`, `ReadBytes`/`ReadString`, typed-span, count and
   char-array overloads).

Auto-open is the design flaw this plan removes: an object that silently reopens at offset 0 (or
truncates) on every call is correct only while every caller knows whether it is open. It has already
produced a real bug (Findings, TD super-record).

## Findings (surveyed 2026-09-22 at `ae978cf6`)

- **Uses.** ~200 production sites construct a `GameFile` or `DiskFile`: 46 existence checks, 39
  one-shot reads (`GameFile(name).ReadObject(table)`), 51 hand-offs to a consumer taking `File&`, 17
  explicit open/read/write sequences, 4 `Size()`-only, 7 long-lived holders (TD `record_file_`, RA
  `RecordFile`, the audio mixer's streamed score, TD `map.cc`'s heap `GameFile`, the two
  `BufferedFileReader`s, vqaview's `PaletteFile` which is not built).
- **Consumers of `File&`** and what they use:

  | Consumer                                                                                                       | Uses                                                           | Opens/closes itself                     |
  | -------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------- | --------------------------------------- |
  | `FileSource` / `FileSink` (`tech/file_source.*`, `file_sink.*`)                                                | `IsOpen`, `IsAvailable`, `Open`, `Read`/`Write`, `ok`, `Close` | yes, lazily, closes only what it opened |
  | `INIClass::Load/Save`, `CCINIClass::Load/Save` (`ra/ini.*`, `ra/ccini.*`)                                      | via FileSource/FileSink                                        | via the pipe                            |
  | `Load_Uncompress` (`ra/jshell.cc:201`, `td/jshell.cc:215`)                                                     | `ReadObject`, typed `Read`, `Seek`                             | yes                                     |
  | `LoadAllocData`, `Load_Alloc_Data` (`td/jshell.cc:279`, `:285`)                                                | `Size`, `Read`                                                 | no (auto-open)                          |
  | `WsaAnimation(File&)`, `Load(File&)` (`tech/wsa_animation.*`)                                                  | `ReadObject`, `Seek`, `Read`, `Size`                           | no                                      |
  | `Write_PCX_File(File&)` (`ra/writepcx.cc:101`) and `Write_Pcx_ScanLine` (`ra/writepcx.cc`, `tech/pcx_file.cc`) | `WriteObject`, `Write`                                         | RA one: yes                             |
  | `AudioMixer::Stream(std::unique_ptr<File>)` (`tech/audio_mixer.*`)                                             | `ReadObject`, `ReadBytes`                                      | no                                      |
  | `BufferedFileReader(GameFile&)` (`ra/nondosstub.cc:163`, `td/nondosstub.cc:188`)                               | `Read`                                                         | no                                      |
  | `Load/Save_Recording_Values(GameFile&)` (`ra/init.cc:2580`, `:2605`)                                           | via FileSource/FileSink                                        | no, file already open                   |
  | `MixArchive::Open/Cache` (`tech/mix_archive.cc:39`, `:146`)                                                    | FileSource + `Seek`/`Size` on the same `GameFile`              | via the pipe                            |

- **Dead code found by the survey** (deleted in Step 1):
  - `FuseClass::Fuse_Read/Fuse_Write(File&)` (`ra/fuse.cc:152`, `:173`): no callers.
  - `SessionClass::Save/Load(GameFile&)` (`ra/session.cc:485`, `:511`): no callers; the
    `ByteSink&`/`ByteSource&` overloads remain.
  - `Read_Line(File&, ...)` (`tech/readline.cc:51`): no callers; the `ByteSource&` overload remains.
  - TD `Load_Picture` (`td/jshell.cc:~270`): no callers (only a comment in `td/nullmgr.cc:1612`).
  - Unused locals: `td/udata.cc:1445`, `td/idata.cc:1815`, `td/idata.cc:1859`, `ra/udata.cc:1068`,
    `ra/idata.cc:1257` (`const GameFile file;`), `ra/tab.cc:295`
    (`const DiskFile file("tabs.shp");`), `ra/wol_gsup.cc:555` (`const GameFile loadfile(...)`).
  - Stale `#include "tech/file.h"`: `ra/anim.h:54`, `ra/saveload.h:12` (which needs `<span>`
    instead), `td/base.h:52`, `td/factory.h:54`, `td/layer.h:49`, `td/mouse.h:56`, `td/score.h:58`,
    `td/sidebar.h:59`, `td/trigger.h:57`, `tech/byte_stream.h:18` (an inverted dependency: the
    low-level stream header pulls in the file interface built on it).
  - `td/startup.cc:339-341` reads the whole config with `Load_Alloc_Data` and deletes the buffer
    unused.
- **Bug: TD super-record truncates RECORD.BIN.** In super-record mode `record_file_` is closed after
  the header (`td/init.cc:2928`) and after each frame's block (`td/conquer.cc:3273`), but
  `Queue_Record` (`td/queue.cc:3390`, `:3394`) then writes to the closed file; each `WriteObject`
  auto-opens with `kWrite`, which truncates. RA has no super-record mode.
- **Hazard: `Seek`/`Size` on a file only a pipe opened.** `ra/saveload.cc:525`/`:569`/`:676`/`:707`
  and `tech/mix_archive.cc:118-126` seek on a `DiskFile`/`GameFile` that `FileSink`/`FileSource`
  opened lazily; if that open failed, `Seek` silently returns 0. Sharing one open stream fixes it.
- **Dead macro blocks are left alone**, as in earlier sweeps: code under `DEMO`, `JAPANESE`,
  `ONHOLD`, `OBSOLETE`, `DENZIL_MIXEXTRACT` never compiles (`td/msgbox.cc:253`/`:305` and
  `td/init.cc:362` would not compile today), and `td/winstub.cc`, `ra/winstub.cc` and
  `winvq/vqaview` are not built. `NEWMENU` and `kCheatKeysEnabled` code **is** compiled.
- **`Extract()` is live** (`ra/init.cc:2241`, `:2249`, `:2250`): the Steam install path copies
  GENERAL.MIX/SCORES.MIX out of MAIN3/MAIN4.MIX on first run.
- **C library use** is confined to `src/sdllib/file.cc`:
  `IO_Open_File/Close/Read/Write/Seek/ Get_File_Size` (users: `DiskStream`, `FindExistingFile`,
  `sdllib/file_test.cc`), `IO_Delete_File` (`DiskFile::Delete`, `GameFile::Delete`),
  `Find_First_File/Next/End` over `fnmatch` + `stat` (RA `init.cc:1968`, `session.cc:990`/`:1073`,
  `loaddlg.cc:639`; TD `init.cc:393`, `loaddlg.cc:571`), `Disk_Space_Available` over `getcwd` +
  `statvfs` (RA `startup.cc:334`, `loaddlg.cc:465`, `session.cc:1341`; TD `startup.cc:327`,
  `loaddlg.cc:439`). `base/seek_origin.h`'s `StdioOrigin`/`SeekOriginFromStdio` exist only for
  `IO_Seek_File`.
- **What `std::filebuf` cannot do** that stdio did: report a read _error_ distinct from end of file.
  The one read error the tests exercise is reading a directory, which `DiskStream::Open` will refuse
  up front. Write errors remain detectable (a short `sputn`, a failed `pubsync`).
- Both toolchains accept `std::chrono::clock_cast<std::chrono::system_clock>` on
  `std::filesystem::file_time_type` (checked: GCC 13.3, clang 23 with libstdc++).

## Global Constraints

- Google C++ style and the project CLAUDE.md: `PascalCase` functions, `snake_case_` members,
  `base::ssize` for sizes and positions, `int64_t` for anything that may pass 2^31, no C-style
  casts, no printf family, `const` locals, Chromium-style include paths, no EA header on new files,
  `#ifndef CNC_RED_ALERT_<PATH>_<FILE>_H_` guards.
- No `void*` I/O; byte data is `std::span<std::byte>` (user preference from the file I/O refactor).
- `char` ↔ `std::byte` views go through `port::CharBytes` (`src/port/bytes_of.h`), which carries
  the one `reinterpret_cast` NOLINT.
- Every commit builds both `build/` and `build-strict/` (clang-tidy clean), passes `ctest`, and —
  from Step 5 on — both smoke scripts. Commit messages carry no `Co-Authored-By` or other trailer.
- Code under never-defined macros and in unbuilt files is not converted (see Findings).
- Save games, record files and mixfiles are byte-identical before and after: nothing here changes a
  format.

## Review Focus

Inputs no step's happy-path tests cover but a player will hit, most likely first. Each has a test in
the step that owns the code.

1. **Lowercase data on Linux** (`redalert.ini`, `conquer.ini`, `savegame.001`, `sc01.mix`): reads
   _and writes_ go to the existing lowercase file, not a new upper-case twin; `FindFiles` patterns
   match regardless of case. Tests: Step 4 (`MatchesPatternIgnoresCase`,
   `FindFilesMatchesLowercaseNames`), Step 5 (`OpenDiskFileWritesTheExistingLowercaseFile`).
2. **A missing optional file** (no `MPLAYER.INI`, no `CONQUER.INI`, no `HALLFAME.DAT`): the open
   returns null and the caller carries on as it did when the old `Load`/`Read` failed. Tests: Step 5
   (`OpenGameFileOfNameFoundNowhereIsNull`); each converted caller keeps its `if`.
3. **A disk that fills up while saving**: `DiskStream::ok()` turns false and `StreamSink` fails, so
   `Save_Game` reports failure instead of claiming success. Tests: Step 3
   (`WriteToFullDeviceFails`), Step 5 (`StreamSinkFailsOnShortWrite`).
4. **A directory where a file is expected** (a folder named `SAVEGAME.001` or `REDALERT.INI`):
   opening it fails, and listing skips it. Tests: Step 3 (`OpenRefusesDirectory`), Step 4
   (`FindFilesSkipsDirectories`).
5. **A crash mid-recording in TD super-record mode**: the frames written so far are on disk. Test:
   Step 8 (`FlushMakesWrittenBytesVisible`), plus the TD recording check in Step 8.

---

## Target API

```cpp
// src/tech/byte_stream.h — ByteStream gains File's helpers and a Flush.
class ByteStream {
 public:
  virtual base::ssize Read(std::span<std::byte> buffer) = 0;
  virtual base::ssize Write(std::span<const std::byte> buffer) = 0;
  virtual base::ssize Seek(base::ssize offset, SeekOrigin origin = SeekOrigin::kCurrent) = 0;
  virtual base::ssize Size() = 0;
  [[nodiscard]] virtual bool ok() const { return true; }
  virtual bool Flush() { return ok(); }        // Step 5
  base::ssize Tell();
  // Moved from File (Step 2):
  template <typename T> requires std::is_trivially_copyable_v<T> bool ReadObject(T& value);
  template <typename T> requires std::is_trivially_copyable_v<T> bool WriteObject(const T& value);
  std::vector<std::byte> ReadBytes(base::ssize count);
  std::string ReadString(base::ssize count);
  // + the typed-span, (span, count) and char-array (T(&)[N], count) Read/Write overloads.
};

// src/tech/game_file.h — free functions replace class GameFile (Step 5; class deleted in Step 10).
std::unique_ptr<ByteStream> OpenGameFile(std::string_view name,
                                         FileAccess access = FileAccess::kRead);
bool GameFileExists(std::string_view name);
base::ssize GameFileSize(std::string_view name);   // 0 if found nowhere
bool DeleteGameFile(std::string_view name);         // loose files only

// src/tech/disk_file.h — free functions replace class DiskFile.
std::optional<std::string> FindExistingFile(std::string_view path);   // unchanged
std::unique_ptr<DiskStream> OpenDiskFile(std::string_view path,
                                         FileAccess access = FileAccess::kRead);

// src/tech/stream_source.h, src/tech/stream_sink.h — replace FileSource/FileSink.
class StreamSource : public ByteSource { explicit StreamSource(ByteStream& stream); ... };
class StreamSink : public ByteSink { explicit StreamSink(ByteStream& stream); ... };

// src/sdllib/file_system.h — replaces src/sdllib/file.h.
struct FoundFile { std::string name; int64_t modified; };
bool MatchesPattern(std::string_view pattern, std::string_view name);
std::vector<FoundFile> FindFiles(std::string_view pattern,
                                 const std::filesystem::path& directory = ".");
int64_t FreeDiskSpace();
```

Why free functions rather than a slimmer `GameFile` class: once a handle cannot read, all it holds
is a name, and every use is "ask one question about this name" (`GameFileExists("TRAILER.VQA")`). A
class of one string whose methods each take no state is a namespace in disguise, which the Google
guide steers away from. The long-lived holders keep a `std::string` name next to their
`std::unique_ptr<ByteStream>`.

## Call-site patterns

These are the conversions Steps 6–9 apply. `OpenDiskFile` replaces `OpenGameFile` wherever the old
code used `DiskFile`.

| Old                                                       | New                                                                                                                                                                                   |
| --------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `GameFile(n).IsAvailable()`                               | `GameFileExists(n)`                                                                                                                                                                   |
| `DiskFile(n).IsAvailable()`                               | `FindExistingFile(n).has_value()`                                                                                                                                                     |
| `GameFile(n).Size()`                                      | `GameFileSize(n)`                                                                                                                                                                     |
| `GameFile(n).ReadObject(x);`                              | `if (const auto file = OpenGameFile(n)) { file->ReadObject(x); }`                                                                                                                     |
| `GameFile f(n); if (f.IsAvailable()) { f.Read(buf, k); }` | `if (const auto file = OpenGameFile(n)) { file->Read(buf, k); }`                                                                                                                      |
| `GameFile f(n); ini.Load(f);`                             | `if (const auto file = OpenGameFile(n)) { ini.Load(*file); }`                                                                                                                         |
| `if (!ini.Load(f)) { fail; }`                             | `const auto file = OpenGameFile(n); if (!file \|\| !ini.Load(*file)) { fail; }`                                                                                                       |
| Load then Save through one object                         | two scoped streams: read in one `if`, write in a later `if (const auto out = OpenGameFile(n, FileAccess::kWrite))` — the read stream must be closed before the write stream truncates |
| `f.Open(kWrite); ...; f.Close();`                         | `const auto out = OpenGameFile(n, FileAccess::kWrite); if (!out) {...}` — scope ends the file                                                                                         |
| `f.Delete()` (DiskFile)                                   | `std::error_code error; std::filesystem::remove(n, error);`                                                                                                                           |
| `f.Create()` before a load+save                           | dropped: the write open creates the file                                                                                                                                              |

`const auto file` above is `const std::unique_ptr<ByteStream> file` (or `DiskStream`); spell the
type out when the initializer does not name it, as clang-tidy's `modernize-use-auto` allows either.

---

## Step 0: Commit this plan

- [ ] `git add docs/FILE_INTERFACE_REMOVAL_PLAN.md && git commit -m "Plan the removal of the File interface"`
- [ ] Add a _Current Work_ line to `MEMORY.md` pointing at this document.

## Step 1: Delete the dead code the survey found

**Files:** `src/ra/fuse.{h,cc}`, `src/ra/session.{h,cc}`, `src/tech/readline.{h,cc}`,
`src/td/jshell.{h,cc}`, `src/td/udata.cc`, `src/td/idata.cc`, `src/ra/udata.cc`, `src/ra/idata.cc`,
`src/ra/tab.cc`, `src/ra/wol_gsup.cc`, the ten headers with a stale `tech/file.h` include
(Findings), `src/td/startup.cc:339-341`.

- [ ] Delete `FuseClass::Fuse_Read/Fuse_Write`, `SessionClass::Save/Load(GameFile&)`,
      `Read_Line(File&, ...)`, TD `Load_Picture`, and the seven unused locals.
- [ ] Delete the unused `Load_Alloc_Data(cfile)` + `delete[]` in `td/startup.cc:339-341`.
- [ ] Remove the ten stale `#include "tech/file.h"`; add `#include <span>` to `ra/saveload.h`; add
      whatever direct includes the build then asks for in the `.cc` files that leaned on them.
- [ ] Build both dirs (`cmake --build build --parallel 22`,
      `cmake --build build-strict --parallel 14`) and run
      `ctest --test-dir build-strict --output-on-failure`. Expected: clean, all tests pass.
- [ ] Commit: `Delete the File-taking functions nobody calls and stale file.h includes`.

## Step 2: Give ByteStream the typed helpers

`File` keeps its own copies until Step 10, so the compiler — not review — finds every caller that
still hands a `File` to code converted to `ByteStream&`: a `GameFile` is not a `ByteStream`.

**Files:** Modify `src/tech/byte_stream.{h,cc}`; test `src/tech/byte_stream_test.cc`.

**Produces:** `ByteStream::ReadObject`, `WriteObject`, `ReadBytes`, `ReadString`, and the typed
`Read`/`Write` overloads, with the signatures in `file.h:105-177`.

- [ ] **Write the failing tests** — port `src/tech/file_test.cc`'s six cases to
      `byte_stream_test.cc` over streams. Read-only cases use `MemoryStream`; the round trip uses
      the existing `DiskStreamTest` fixture:

```cpp
struct Header {
  int32_t magic;
  int16_t version;
  int16_t flags;
};

TEST_F(DiskStreamTest, ObjectsRoundTripThroughWriteObjectAndReadObject) {
  const Header written{.magic = 0x52415356, .version = 7, .flags = -2};
  {
    const std::unique_ptr<DiskStream> out =
        DiskStream::Open(path(), FileAccess::kWrite);
    ASSERT_NE(out, nullptr);
    EXPECT_TRUE(out->WriteObject(written));
  }
  const std::unique_ptr<DiskStream> in = DiskStream::Open(path(), FileAccess::kRead);
  ASSERT_NE(in, nullptr);
  Header read{};
  EXPECT_TRUE(in->ReadObject(read));
  EXPECT_EQ(read.magic, written.magic);
  EXPECT_EQ(read.version, written.version);
  EXPECT_EQ(read.flags, written.flags);
}

TEST(MemoryStreamTest, ReadObjectFailsOnShortRead) {
  const std::array<std::byte, 3> bytes{};
  MemoryStream stream(bytes);
  int32_t value = 0;
  EXPECT_FALSE(stream.ReadObject(value));
}

TEST(MemoryStreamTest, ReadBytesAndReadStringStopAtEndOfStream) {
  const std::string_view text = "abcde";
  MemoryStream stream(std::as_bytes(std::span(text)));
  EXPECT_EQ(stream.ReadString(3), "abc");
  EXPECT_EQ(std::ssize(stream.ReadBytes(10)), 2);
}

TEST(MemoryStreamTest, TypedViewCountIsBytesRatherThanElements) {
  const std::array<uint16_t, 2> words{0x0102, 0x0304};
  MemoryStream stream(std::as_bytes(std::span(words)));
  std::array<uint16_t, 2> read{};
  EXPECT_EQ(stream.Read(std::span(read), 2), 2);  // 2 bytes = 1 element
  EXPECT_EQ(read[0], words[0]);
  EXPECT_EQ(read[1], 0);
}
```

Port `ArraysAreObjectsToo` and `SpanAndRawPointerReadsAgree` the same way, from `file_test.cc:52`
and `:75`, reading from a `MemoryStream`.

- [ ] **Run to see them fail:**
      `cmake --build build-strict --target tech_test && ctest --test-dir     build-strict -R 'MemoryStreamTest|DiskStreamTest' --output-on-failure`.
      Expected: compile errors, `no member named 'ReadObject' in 'MemoryStream'`.
- [ ] **Implement:** copy `File`'s helper templates (`file.h:105-177`) into `ByteStream`'s public
      section verbatim, with `File` → `ByteStream` in the comment about `using` declarations, and
      `File::ReadBytes`/`ReadString` (`file.cc`) into `byte_stream.cc` as `ByteStream::` members.
      Add `using ByteStream::Read;` and `using ByteStream::Write;` to `DiskStream`, `MemoryStream`
      and `RangeStream`, whose overrides would otherwise hide the templates. Add the includes they
      need (`<string>`, `<type_traits>`, `<vector>`, `absl/log/check.h`, `base/buffer.h`,
      `base/numeric.h`).
- [ ] **Run the tests:** expected PASS, and the rest of `tech_test` still passes.
- [ ] Commit: `Give ByteStream the typed read and write helpers`.

## Step 3: DiskStream on std::filebuf; delete the IO\_\* functions

**Files:** Modify `src/tech/byte_stream.{h,cc}`, `src/tech/disk_file.cc`, `src/tech/game_file.cc`,
`src/base/seek_origin.h`, `src/sdllib/file.{h,cc}`, `src/tech/stream_error_test.cc`; move
`src/sdllib/file_test.cc`'s two tests into `src/tech/byte_stream_test.cc` and drop `file_test.cc`
from the sdllib test list (`src/sdllib/CMakeLists.txt:33`).

**Consumes:** nothing new. **Produces:** `DiskStream` with the same public interface.

- [ ] **Write the failing tests** in `byte_stream_test.cc`:

```cpp
TEST_F(DiskStreamTest, ReadWriteKeepsExistingContents) {
  {
    const std::unique_ptr<DiskStream> stream =
        DiskStream::Open(path(), FileAccess::kReadWrite);
    ASSERT_NE(stream, nullptr);
    EXPECT_EQ(stream->Seek(0, SeekOrigin::kEnd), 5);
    EXPECT_EQ(stream->Write(Bytes("!")), 1);
  }
  const std::unique_ptr<DiskStream> stream = DiskStream::Open(path(), FileAccess::kRead);
  EXPECT_EQ(ReadAll(*stream, 10), "xabcd!");
}

TEST_F(DiskStreamTest, ReadWriteCreatesMissingFile) {
  const std::string missing = path() + ".new";
  EXPECT_NE(DiskStream::Open(missing, FileAccess::kReadWrite), nullptr);
  EXPECT_TRUE(std::filesystem::exists(missing));
  std::filesystem::remove(missing);
}

TEST_F(DiskStreamTest, SeekBeforeTheStartKeepsThePosition) {
  const std::unique_ptr<DiskStream> stream = DiskStream::Open(path(), FileAccess::kRead);
  EXPECT_EQ(stream->Seek(2, SeekOrigin::kBegin), 2);
  EXPECT_EQ(stream->Seek(-10, SeekOrigin::kCurrent), 2);
  EXPECT_EQ(ReadAll(*stream, 1), "b");
}

TEST_F(DiskStreamTest, SizePreservesThePosition) {
  const std::unique_ptr<DiskStream> stream = DiskStream::Open(path(), FileAccess::kRead);
  EXPECT_EQ(ReadAll(*stream, 2), "xa");
  EXPECT_EQ(stream->Size(), 5);
  EXPECT_EQ(ReadAll(*stream, 1), "b");
}

TEST(DiskStreamErrorTest, OpenRefusesDirectory) {
  EXPECT_EQ(DiskStream::Open(std::filesystem::temp_directory_path().string(),
                             FileAccess::kRead),
            nullptr);
}

TEST(DiskStreamErrorTest, WriteToFullDeviceFails) {
  if (!std::filesystem::exists("/dev/full")) {
    GTEST_SKIP() << "no /dev/full on this platform";
  }
  const std::unique_ptr<DiskStream> stream =
      DiskStream::Open("/dev/full", FileAccess::kWrite);
  ASSERT_NE(stream, nullptr);
  // Larger than any filebuf buffer, so the failure surfaces in Write itself.
  const std::vector<std::byte> block(1 << 20);
  stream->Write(block);
  EXPECT_FALSE(stream->ok());
}
```

In `stream_error_test.cc:189-207` replace the directory test body: `DiskStream::Open(directory)` is
now expected to be `nullptr` (no `GTEST_SKIP`), `DiskFile(directory).Open()` to return false, and
the `FileSource` over that `DiskFile` still to fail (`IsAvailable` is false).

- [ ] **Run to see them fail:** `OpenRefusesDirectory` fails today (stdio opens directories).
- [ ] **Implement `DiskStream`** in `byte_stream.h`: replace `void* handle_` and the handle
      constructor with

```cpp
 private:
  DiskStream() = default;

  // The open file. A filebuf reports failed writes (a short sputn, a failed
  // pubsync) but not failed reads, which look like the end of the file.
  std::filebuf file_;

  // Set when a write or flush fails.
  bool failed_ = false;
```

and in `byte_stream.cc`:

```cpp
namespace {
std::ios_base::seekdir SeekDir(const SeekOrigin origin) {
  switch (origin) {
    case SeekOrigin::kBegin:
      return std::ios_base::beg;
    case SeekOrigin::kEnd:
      return std::ios_base::end;
    case SeekOrigin::kCurrent:
    default:
      return std::ios_base::cur;
  }
}

base::ssize ToPosition(const std::streampos position) {
  return static_cast<base::ssize>(std::streamoff(position));
}
}  // namespace

std::unique_ptr<DiskStream> DiskStream::Open(const std::string_view path,
                                             const FileAccess access) {
  const std::filesystem::path file_path(path);
  // A directory opens as a file on POSIX and then fails every read, which a
  // filebuf cannot report; refuse it here instead.
  std::error_code error;
  if (std::filesystem::is_directory(file_path, error)) {
    return nullptr;
  }
  auto stream = std::unique_ptr<DiskStream>(new DiskStream);
  std::filebuf& file = stream->file_;
  constexpr std::ios_base::openmode kBinary = std::ios_base::binary;
  bool opened = false;
  switch (access) {
    case FileAccess::kRead:
      opened = file.open(file_path, std::ios_base::in | kBinary) != nullptr;
      break;
    case FileAccess::kWrite:
      opened = file.open(file_path, std::ios_base::out | std::ios_base::trunc |
                                        kBinary) != nullptr;
      break;
    case FileAccess::kReadWrite:
      // in|out keeps the contents (the record file appends to itself) but
      // needs the file to exist; create it only when there is nothing to keep.
      opened =
          file.open(file_path, std::ios_base::in | std::ios_base::out | kBinary) !=
              nullptr ||
          file.open(file_path, std::ios_base::in | std::ios_base::out |
                                   std::ios_base::trunc | kBinary) != nullptr;
      break;
  }
  return opened ? std::move(stream) : nullptr;
}

base::ssize DiskStream::Read(const std::span<std::byte> buffer) {
  return file_.sgetn(port::CharBytes(buffer).data(), std::ssize(buffer));
}

base::ssize DiskStream::Write(const std::span<const std::byte> buffer) {
  const base::ssize written =
      file_.sputn(port::CharBytes(buffer).data(), std::ssize(buffer));
  if (written != std::ssize(buffer)) {
    failed_ = true;
  }
  return written;
}

base::ssize DiskStream::Seek(const base::ssize offset, const SeekOrigin origin) {
  const std::streampos moved = file_.pubseekoff(offset, SeekDir(origin));
  if (std::streamoff(moved) == -1) {
    // A seek to before the start fails and leaves the position alone, as
    // fseek did; report where the stream still is.
    return ToPosition(file_.pubseekoff(0, std::ios_base::cur));
  }
  return ToPosition(moved);
}

base::ssize DiskStream::Size() {
  const std::streampos here = file_.pubseekoff(0, std::ios_base::cur);
  const std::streampos end = file_.pubseekoff(0, std::ios_base::end);
  file_.pubseekpos(here);
  return ToPosition(end);
}
```

Delete `DiskStream::~DiskStream()` (the filebuf closes itself) and the
`ABSL_ATTRIBUTE_LIFETIME_BOUND` handle constructor. Fix the `DiskStream` comment in `byte_stream.h`
("A seek to before the start of the file leaves the position where it was, as stdio does" stays
true).

- [ ] **`FindExistingFile`** (`disk_file.cc`): test with
      `DiskStream::Open(name, FileAccess::kRead)     != nullptr` instead of
      `IO_Open_File`/`IO_Close_File`, keeping "opening is the existence test".
- [ ] **Deletes:** `DiskFile::Delete` and `GameFile::Delete` call
      `std::error_code error; return std::filesystem::remove(path, error);`.
- [ ] **Remove** `IO_Open_File`, `IO_Close_File`, `IO_Read_File`, `IO_Write_File`, `IO_Seek_File`,
      `IO_Get_File_Size`, `IO_Delete_File` from `sdllib/file.{h,cc}`, and `StdioOrigin` /
      `SeekOriginFromStdio` plus `<cstdio>` from `base/seek_origin.h`. Update the comment in
      `sdllib/file_access.h` ("used by File::Open() and IO_Open_File()" → "used by the stream
      openers").
- [ ] **Run:** full `tech_test`, `sdllib` tests, `disk_file_test`'s
      `SeekIsIgnoredBeforeTheStartOfTheFile`, `game_file_test`. Expected PASS.
- [ ] **Performance check** (the mixfile reads go through `RangeStream` → `DiskStream`): time a
      saved-game load in RelWithDebInfo before and after (`-LOADGAME0 -QUITFRAME120`, CLAUDE.md
      baseline 19 ms), and play the RA intro movie on a real display: it must still decode in real
      time.
- [ ] Both smoke scripts, passing the binary explicitly:
      `tools/ra_saveload_smoke.sh build-strict/src/ra/rasdl`,
      `tools/td_saveload_smoke.sh build-strict/src/td/tdsdl SCG01EA --team`.
- [ ] Commit: `Put DiskStream on std::filebuf and delete the C file functions`.

## Step 4: sdllib/file → sdllib/file_system on std::filesystem

**Files:** `git mv src/sdllib/file.h src/sdllib/file_system.h`,
`git mv src/sdllib/file.cc src/sdllib/file_system.cc`; create `src/sdllib/file_system_test.cc` and
list it in `src/sdllib/CMakeLists.txt:33`. Callers: RA `init.cc:1968-1983`, `session.cc:990-1010`,
`session.cc:1073-1088`, `session.cc:1341`, `loaddlg.cc:465`, `loaddlg.cc:639-701`, `startup.cc:334`;
TD `init.cc:393-408`, `loaddlg.cc:439`, `loaddlg.cc:571-622`, `startup.cc:327`.

**Produces:** `FoundFile`, `MatchesPattern`, `FindFiles`, `FreeDiskSpace` (Target API).

- [ ] **Write the failing tests** (`file_system_test.cc`):

```cpp
TEST(MatchesPatternTest, StarAndQuestionMark) {
  EXPECT_TRUE(MatchesPattern("SC*.MIX", "SC01.MIX"));
  EXPECT_TRUE(MatchesPattern("SC*.MIX", "SC.MIX"));
  EXPECT_TRUE(MatchesPattern("SAVEGAME.*", "SAVEGAME.001"));
  EXPECT_TRUE(MatchesPattern("*.PKT", "MISSIONS.PKT"));
  EXPECT_TRUE(MatchesPattern("A?C", "ABC"));
  EXPECT_FALSE(MatchesPattern("A?C", "AC"));
  EXPECT_FALSE(MatchesPattern("SC*.MIX", "SC01.MIXX"));
  EXPECT_FALSE(MatchesPattern("SC*.MIX", "XSC01.MIX"));
  EXPECT_TRUE(MatchesPattern("*", ""));
  EXPECT_FALSE(MatchesPattern("", "A"));
}

TEST(MatchesPatternTest, MatchesPatternIgnoresCase) {
  EXPECT_TRUE(MatchesPattern("SC*.MIX", "sc01.mix"));
  EXPECT_TRUE(MatchesPattern("savegame.*", "SAVEGAME.001"));
}

class FindFilesTest : public ::testing::Test {
 protected:
  void SetUp() override {
    dir_ = std::filesystem::temp_directory_path() /
           ("file_system_test_" +
            std::string(::testing::UnitTest::GetInstance()->current_test_info()->name()));
    std::filesystem::remove_all(dir_);
    std::filesystem::create_directory(dir_);
  }
  void TearDown() override { std::filesystem::remove_all(dir_); }
  void Touch(const std::string& name) const { std::ofstream(dir_ / name) << "x"; }
  [[nodiscard]] const std::filesystem::path& dir() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return dir_;
  }

 private:
  std::filesystem::path dir_;
};

TEST_F(FindFilesTest, FindFilesMatchesLowercaseNames) {
  Touch("sc02.mix");
  Touch("SC01.MIX");
  Touch("SS01.MIX");
  const std::vector<FoundFile> found = FindFiles("SC*.MIX", dir());
  ASSERT_EQ(found.size(), 2U);
  EXPECT_EQ(found[0].name, "SC01.MIX");  // sorted by name
  EXPECT_EQ(found[1].name, "sc02.mix");
}

TEST_F(FindFilesTest, FindFilesSkipsDirectories) {
  std::filesystem::create_directory(dir() / "SAVEGAME.002");
  Touch("SAVEGAME.001");
  const std::vector<FoundFile> found = FindFiles("SAVEGAME.*", dir());
  ASSERT_EQ(found.size(), 1U);
  EXPECT_EQ(found[0].name, "SAVEGAME.001");
}

TEST_F(FindFilesTest, ModifiedIsSecondsSinceTheEpoch) {
  Touch("A.BIN");
  const std::vector<FoundFile> found = FindFiles("A.BIN", dir());
  ASSERT_EQ(found.size(), 1U);
  const int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();
  EXPECT_NEAR(static_cast<double>(found[0].modified), static_cast<double>(now), 60.0);
}

TEST(FreeDiskSpaceTest, IsPositive) { EXPECT_GT(FreeDiskSpace(), 0); }
```

- [ ] **Run to see them fail** (`file_system.h` does not declare these yet).
- [ ] **Implement** `file_system.h` (file comment: "Directory listing and free space for the working
      directory, over std::filesystem.") and `file_system.cc`; delete the whole `_WIN32` branch,
      `fnmatch`, `stat`, `statvfs`, `getcwd` and their headers:

```cpp
bool MatchesPattern(std::string_view pattern, std::string_view name) {
  // Greedy matching with one backtrack point: on a mismatch, the last '*'
  // swallows one more character of name and matching resumes after it.
  std::string_view star_pattern;
  std::string_view star_name;
  bool have_star = false;
  const auto same = [](const char a, const char b) {
    return absl::ascii_tolower(static_cast<unsigned char>(a)) ==
           absl::ascii_tolower(static_cast<unsigned char>(b));
  };
  while (!name.empty()) {
    if (!pattern.empty() && pattern.front() == '*') {
      pattern.remove_prefix(1);
      star_pattern = pattern;
      star_name = name;
      have_star = true;
    } else if (!pattern.empty() &&
               (pattern.front() == '?' || same(pattern.front(), name.front()))) {
      pattern.remove_prefix(1);
      name.remove_prefix(1);
    } else if (have_star) {
      star_name.remove_prefix(1);
      pattern = star_pattern;
      name = star_name;
    } else {
      return false;
    }
  }
  return pattern.find_first_not_of('*') == std::string_view::npos;
}

std::vector<FoundFile> FindFiles(const std::string_view pattern,
                                 const std::filesystem::path& directory) {
  std::vector<FoundFile> found;
  std::error_code error;
  std::filesystem::directory_iterator entries(directory, error);
  // The iterator's own increment throws on error; step it by hand instead.
  for (; !error && entries != std::filesystem::directory_iterator();
       entries.increment(error)) {
    const std::filesystem::directory_entry& entry = *entries;
    std::string name = entry.path().filename().string();
    if (!MatchesPattern(pattern, name)) {
      continue;
    }
    // A directory, or a name that cannot be stat'ed (a broken symlink, no
    // permission), is skipped.
    std::error_code stat_error;
    if (!entry.is_regular_file(stat_error)) {
      continue;
    }
    const std::filesystem::file_time_type written = entry.last_write_time(stat_error);
    if (stat_error) {
      continue;
    }
    const auto system_time = std::chrono::clock_cast<std::chrono::system_clock>(written);
    found.push_back({.name = std::move(name),
                     .modified = std::chrono::duration_cast<std::chrono::seconds>(
                                     system_time.time_since_epoch())
                                     .count()});
  }
  std::ranges::sort(found, {}, &FoundFile::name);
  return found;
}

int64_t FreeDiskSpace() {
  std::error_code error;
  const std::filesystem::space_info space = std::filesystem::space(".", error);
  return error ? 0 : static_cast<int64_t>(space.available);
}
```

- [ ] **Convert the callers.** `do { ... } while (Find_Next_File(state))` loops become range-for
      loops; a `continue` keeps its meaning:

```cpp
// before (ra/init.cc:1968)
FindFileState state{};
if (Find_First_File("SC*.MIX", state)) {
  do {
    if (absl::EqualsIgnoreCase(state.name, "scores.mix")) {
      continue;
    }
    MixArchive::Register(state.name, &TheAssets().mix_key());
    MixArchive::Cache(state.name);
  } while (Find_Next_File(state));
}
// after
for (const FoundFile& found : FindFiles("SC*.MIX")) {
  if (absl::EqualsIgnoreCase(found.name, "scores.mix")) {
    continue;
  }
  MixArchive::Register(found.name, &TheAssets().mix_key());
  MixArchive::Cache(found.name);
}
```

The `while (found) { ...; found = Find_Next_File(state); }` loops in `session.cc` and the two
`loaddlg.cc` convert the same way; `fdata->DateTime = find_state.mod_time` becomes
`= found.modified` (`DateTime` is already `int64_t`). `Disk_Space_Available()` becomes
`FreeDiskSpace()`; in `ra/session.cc:1341` keep the CRC bit operations on
`const auto diskfree = static_cast<uint64_t>(FreeDiskSpace());`. Replace `#include "sdllib/file.h"`
with `#include "sdllib/file_system.h"` in each file.

- [ ] **Run:** the sdllib tests, both builds, both smoke scripts. Check on real data: the RA load
      dialog lists existing saves with their dates, and a TD start registers `SC*.MIX` expansions
      (log line "About to register addon mixfiles").
- [ ] Commit: `List files and query free space with std::filesystem`.

## Step 5: Stream openers, StreamSource and StreamSink

**Files:** Modify `src/tech/game_file.{h,cc}`, `src/tech/disk_file.{h,cc}`,
`src/tech/byte_stream.{h,cc}` (`Flush`), `src/tech/game_file_vqa_io.{h,cc}`; create
`src/tech/stream_source.h`, `src/tech/stream_sink.h`; tests in `src/tech/game_file_test.cc`,
`src/tech/disk_file_test.cc`, `src/tech/stream_error_test.cc`.

**Produces:** `OpenGameFile`, `GameFileExists`, `GameFileSize`, `DeleteGameFile`, `OpenDiskFile`,
`ByteStream::Flush`, `StreamSource`, `StreamSink` (Target API). The `GameFile`/`DiskFile` classes
stay and are reimplemented on these, so no caller changes yet.

- [ ] **Write the failing tests:**

```cpp
// game_file_test.cc — port every GameFile test to the free functions, e.g.
TEST_F(GameFileTest, OpenGameFileOfNameFoundNowhereIsNull) {
  EXPECT_EQ(OpenGameFile("GAME_FILE_TEST_MISSING.BIN"), nullptr);
  EXPECT_FALSE(GameFileExists("GAME_FILE_TEST_MISSING.BIN"));
  EXPECT_EQ(GameFileSize("GAME_FILE_TEST_MISSING.BIN"), 0);
}
TEST_F(GameFileTest, SizeOfPackedFileNeedsNoOpen) {
  EXPECT_EQ(GameFileSize(kPackedName), 4);  // as SizeOfUnopenedPackedFileIsItsOwnSize
}
TEST_F(GameFileTest, DeleteGameFileRefusesPackedFile) {
  EXPECT_FALSE(DeleteGameFile(kPackedName));
  EXPECT_TRUE(GameFileExists(kPackedName));
}

// disk_file_test.cc (WriteFile/ReadFile are the file's existing helpers)
TEST_F(DiskFileTest, OpenDiskFileWritesTheExistingLowercaseFile) {
  // FindExistingFile lowercases the whole path, so only an all-lowercase
  // file is found; temp_directory_path() is lowercase on Linux.
  const std::filesystem::path lower =
      std::filesystem::temp_directory_path() / "disk_file_test_lowercase_write.bin";
  const std::filesystem::path upper =
      lower.parent_path() / absl::AsciiStrToUpper(lower.filename().string());
  WriteFile(lower, "old");
  if (std::filesystem::exists(upper)) {
    std::filesystem::remove(lower);
    GTEST_SKIP() << "case-insensitive filesystem";
  }
  {
    const std::unique_ptr<DiskStream> out =
        OpenDiskFile(upper.string(), FileAccess::kWrite);
    ASSERT_NE(out, nullptr);
    out->Write(std::as_bytes(std::span(std::string_view("new"))));
  }
  EXPECT_FALSE(std::filesystem::exists(upper));
  EXPECT_EQ(ReadFile(lower), "new");
  std::filesystem::remove(lower);
}

TEST_F(DiskFileTest, OpenDiskFileForReadOfMissingFileIsNull) {
  EXPECT_EQ(OpenDiskFile(path() + ".missing"), nullptr);
  EXPECT_NE(OpenDiskFile(path() + ".new", FileAccess::kWrite), nullptr);
  std::filesystem::remove(path() + ".new");
}

// stream_error_test.cc — ScriptedFile becomes ScriptedStream : ByteStream; the three
// FileSource-over-ScriptedFile tests (:150, :159, :180) are repeated over StreamSource.
TEST(StreamSinkTest, StreamSinkFailsOnShortWrite) {
  ShortWriteStream stream(/*accept=*/3);   // a ByteStream whose Write stores at most 3 bytes
  StreamSink sink(stream);
  EXPECT_FALSE(sink.Write(std::as_bytes(std::span(std::string_view("abcdef")))));
  EXPECT_FALSE(sink.ok());
}
```

- [ ] **Run to see them fail.**
- [ ] **Implement.** `game_file.h`: move the static `GameFile::OpenStream` body to `OpenGameFile`;
      `GameFileExists` is `GameFile::IsAvailable`'s body (archive index, then
      `SearchPaths::Resolve`); `GameFileSize` is `GameFile::Size`'s closed-file branch;
      `DeleteGameFile` is `GameFile::Delete`'s body. The class's members then call them.
      `disk_file.h`:

```cpp
// Opens path, preferring an existing file under the lowercased name when the
// name as given does not exist (see FindExistingFile), for every access mode:
// a write replaces the file the game would read. Returns nullptr on failure.
std::unique_ptr<DiskStream> OpenDiskFile(std::string_view path,
                                         FileAccess access = FileAccess::kRead);
```

```cpp
std::unique_ptr<DiskStream> OpenDiskFile(const std::string_view path,
                                         const FileAccess access) {
  if (const std::optional<std::string> existing = FindExistingFile(path)) {
    return DiskStream::Open(*existing, access);
  }
  return access == FileAccess::kRead ? nullptr : DiskStream::Open(path, access);
}
```

`ByteStream`: `virtual bool Flush() { return ok(); }` ("Pushes buffered writes to the operating
system. Returns ok()."); `DiskStream::Flush` sets `failed_` when `file_.pubsync() != 0`.
`stream_source.h`:

```cpp
// A source that reads an open stream from its current position. It neither
// opens nor closes the stream; whoever owns the stream does.
//
// Example:
//   const std::unique_ptr<ByteStream> file = OpenGameFile("RULES.INI");
//   StreamSource source(*file);
class StreamSource : public ByteSource {
 public:
  // stream must outlive this source.
  explicit StreamSource(ByteStream& stream ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : stream_(stream) {}

  base::ssize Read(std::span<std::byte> buffer) override {
    if (!ok()) {
      return 0;
    }
    const base::ssize count = stream_.Read(buffer);
    if (!stream_.ok()) {
      Fail();
    }
    return count;
  }

 private:
  ByteStream& stream_;
};
```

`stream_sink.h` likewise: `Write` fails the sink on a short `stream_.Write`, `Flush` fails it when
`stream_.Flush()` returns false. Replace `GameFile::OpenStream` in `game_file_vqa_io.{h,cc}` (its
only user outside `game_file.*`) with `OpenGameFile`, then delete the static.

- [ ] **Run:** all tests; both builds.
- [ ] Commit: `Add free functions that open game and disk files as streams`.

## Step 6: INI files read and write streams

**Files:** `src/ra/ini.{h,cc}`, `src/ra/ccini.{h,cc}`, and every caller: RA `session.cc:557`, `:850`
(Load `:851` + Save `:916`), `:964`, `:996`, `:1022`, `:1049`, `:1076`; `startup.cc:330` (Load
`:357` + Save `:407`, drop the `Create`); `options.cc:469`, `:659` (Load `:668` + Save `:744`);
`scenario.cc:491`, `:1989`, `:2222`, `:2317`, `:2375` (CCINI Save, `DiskFile`); `assets.cc:63`;
`udpaddr.cc:136`; `init.cc:315`, `:323`, `:1224`, `:1238` (Load + Save), `:2521` (`MemoryFile` over
the keys); `nulldlg.cc:4438`; `saveload.cc:1080`, `:1147`.

- [ ] Change `INIClass::Load(File&)` → `Load(ByteStream&)` and `Save(File&) const` →
      `Save(ByteStream&) const`, each wrapping a `StreamSource`/`StreamSink`; the same for
      `CCINIClass::Load(ByteStream&, bool)`/`Save(ByteStream&, bool)`.
- [ ] Convert each caller with the Call-site patterns table. `ra/init.cc:2521` needs no stream:
      `SpanSource source(std::as_bytes(std::span(keys))); ini.Load(source);` (the `ByteSource&`
      overload). Where the old code checked `IsAvailable()` and then `Size()` (`nulldlg.cc:4438`),
      open once and ask the stream.
- [ ] Build both dirs: the compiler lists any caller still passing a `GameFile`/`DiskFile`.
- [ ] Tests, both smoke scripts. Check on real data: RA starts with `REDALERT.INI` present and with
      it deleted (it is recreated), options changed in the menu survive a restart, and the
      skirmish/multiplayer scenario list (`MISSIONS.PKT`, `*.PKT`, `*.MPR`) is populated.
- [ ] Commit: `Load and save INI files through open streams`.

## Step 7: Loaders, writers and the audio mixer take ByteStream

Two commits.

**7a — picture loaders.** Files: `src/ra/jshell.{h,cc}`, `src/td/jshell.{h,cc}`, `src/td/assets.cc`,
`src/td/assets_test.cc:16-19` (the stub becomes
`std::vector<std::byte> LoadAllocData(ByteStream& /*file*/) { return {}; }` with
`class ByteStream;`), and callers TD `ending.cc:111`, `:131`, `:182-185`, `:294`, `:314`,
`mapsel.cc:1045`, `:1049`, `init.cc:1388`, `:1413`, `:1438`, `:1462`, `intro.cc:136-141`,
`mouse.cc:289`, `bbdata.cc:595`, `adata.cc:2275`, `startup.cc:321`; RA `jshell.cc:261`.

- [ ] `Load_Uncompress(ByteStream& file, ...)` in both games: delete the `IsOpen`/`Open`/`Close` and
      the `opened` flag; the reads are unchanged (the helpers are on `ByteStream` now).
      `LoadAllocData(ByteStream&)`, `Load_Alloc_Data(ByteStream&)`.
- [ ] Callers open first:
      `if (const auto file = OpenGameFile("ATTRACT2.CPS")) {     Load_Uncompress(*file, ...); }`.
      Where the result is used as a count, a missing file yields 0 as before. `td/ending.cc:182` and
      `td/intro.cc:136` open one stream per file they read.
- [ ] `td/startup.cc:321`: `Read_Setup_Options(DiskFile*, ...)` becomes
      `Read_Setup_Options(ByteStream&, ...)`; open the config with
      `OpenDiskFile("CONQUER.INI",     FileAccess::kReadWrite)` (creates it if missing, replacing
      `Create()`), and give the profile read (`:383`) and write (`:413`) their own `OpenDiskFile`
      streams in that order.
- [ ] Both builds, tests, TD smoke. Real-display check (headless never loads palettes): TD title and
      ending pictures, map selection, the intro's STRUGGLE sequence, RA's `Load_Picture` screens.
- [ ] Commit: `Read pictures from open streams`.

**7b — WSA, PCX, audio, stub readers, mix archives.** Files: `src/tech/wsa_animation.{h,cc}`
(+test), `src/tech/pcx_file.cc`, `src/ra/writepcx.cc`, `src/ra/filepcx.h`, `src/ra/conquer.cc:619`,
`src/ra/gadget.cc:487`, `src/tech/audio_mixer.{h,cc}` (+test), `src/ra/nondosstub.cc`,
`src/td/nondosstub.cc`, `src/tech/mix_archive.cc`.

- [ ] `WsaAnimation(ByteStream&, span)` and `Load(ByteStream&, span)`; the name constructor uses
      `OpenGameFile` and stays closed when it returns null. The test uses a `MemoryStream` over the
      image instead of `MemoryFile`.
- [ ] TD `Write_PCX_File(const char*, ...)` opens `OpenGameFile(name, FileAccess::kWrite)`;
      `Write_Pcx_ScanLine(ByteStream&, ...)` in both games. RA `Write_PCX_File(ByteStream&, ...)`
      loses its open/close; `conquer.cc` and `gadget.cc` open
      `OpenDiskFile(name, FileAccess::kWrite)`, and `gadget.cc`'s search for an unused
      `scrshtNN.pcx` uses `FindExistingFile(name).has_value()`.
- [ ] `AudioMixer::Stream(std::unique_ptr<ByteStream>, int)`, `Channel::file` a
      `std::unique_ptr<ByteStream>`; the name overload uses `OpenGameFile`. In the test, `ScoreFile`
      becomes `ScoreStream : ByteStream` owning a `std::vector<std::byte>` behind a `MemoryStream`,
      keeping its live-instance counter; `std::unique_ptr<File>()` at `:312` becomes
      `std::unique_ptr<ByteStream>()`.
- [ ] `BufferedFileReader(ByteStream&)`; `Read_PCX_File` opens with `OpenGameFile` and seeks on that
      stream (`Seek(-768, SeekOrigin::kEnd)`).
- [ ] `MixArchive::Open`: `const auto file = OpenGameFile(filename); if (!file) return false;`, a
      `StreamSource` over it, `file->Tell()`/`file->Size()` in place of `Seek(0, kCurrent)`/
      `Size()`, `filename_ = filename`. `MixArchive::Cache`: open first, then source, then
      `Seek(data_start_, SeekOrigin::kBegin)` on the same stream.
- [ ] Both builds, tests (`wsa_animation_test`, `audio_mixer_test`, `mix_archive_test`), both smoke
      scripts. Real-display check: a WSA animation (TD map selection), a streamed score playing past
      its first block in both games, a TD and an RA screenshot written and opened.
- [ ] Commit: `Read animations, scores and archives from open streams`.

## Step 8: Save games and recordings own their streams

**Files:** `src/ra/saveload.cc`, `src/td/saveload.cc`, `src/ra/session.{h,cc}` (`RecordFile`),
`src/ra/init.cc` (`:197-198`, `:551-557`, `:1038-1045`, `:1083-1084`, `:2580-2610`),
`src/ra/conquer.cc:298`, `src/ra/record_playback.cc`, `src/ra/queue.cc`, `src/td/session.h`
(`record_file_`), `src/td/init.cc`, `src/td/conquer.cc`, `src/td/queue.cc`.

- [ ] **Write the failing test** (`byte_stream_test.cc`):

```cpp
TEST_F(DiskStreamTest, FlushMakesWrittenBytesVisible) {
  const std::unique_ptr<DiskStream> out = DiskStream::Open(path(), FileAccess::kWrite);
  ASSERT_NE(out, nullptr);
  out->Write(Bytes("frame"));
  EXPECT_TRUE(out->Flush());
  // Read back through a second handle while the first is still open.
  const std::unique_ptr<DiskStream> in = DiskStream::Open(path(), FileAccess::kRead);
  EXPECT_EQ(ReadAll(*in, 10), "frame");
}
```

- [ ] **RA save/load.** `Save_Game`: `const auto file = OpenDiskFile(name, FileAccess::kWrite)`,
      return false if null, `StreamSink` over `*file`, and the digest rewrite seeks `file` itself
      (`:525`, `:569`); the "Finish closes the file" comment (`:571`) goes — scope does it. The dump
      file is a nullable `std::unique_ptr<DiskStream>`. `Load_Game`: open first (a null stream is
      the old `IsAvailable` failure), `StreamSource`, seeks on the stream, `file.Close()` (`:964`)
      deleted. `Get_Savefile_Info`: the same.
- [ ] **TD save/load.** `Save_Game`, `Load_Game`, `Get_Savefile_Info` hold a
      `std::unique_ptr<DiskStream>`; every `file.Close(); return false;` becomes `return false;`.
      Char-array `Read(descr_buf, n)`/`Write` calls work unchanged on the stream.
- [ ] **RA recording.** `GameFile RecordFile` → `std::string record_file_name_` (set where
      `session.cc:187` calls `SetName`) and `std::unique_ptr<ByteStream> record_stream_`, null when
      not recording or playing back. `IsAvailable()` + `Open(kRead)` → assign `OpenGameFile(name)`
      and test for null; `Close()` → `reset()`. `Load/Save_Recording_Values(ByteStream&)` wrap
      `StreamSource`/`StreamSink`. `record_playback.cc` and `queue.cc` call through the stream
      (`->WriteObject`).
- [ ] **TD recording, fixing super-record.** The same shape for `record_file_`. Super-record keeps
      the stream open for the whole game and calls `Flush()` where it used to `Close()`
      (`td/init.cc:2928`, `td/conquer.cc:3273`); the `Open(kReadWrite)` + `Seek(0, kEnd)` at
      `td/conquer.cc:3235` goes. `Queue_Record` then appends to the open stream instead of
      truncating the file.
- [ ] Both builds, all tests, both smoke scripts (they exercise save/load end to end), and
      `tools/td_saveload_smoke.sh ... SCG01EA --team`. Recording check, both games (the switches
      need `config::kCheatKeysEnabled`): record a short skirmish with `-XX`, play it back with
      `-XY`, and confirm the playback matches. For TD super-record (`-XXS`), also confirm RECORD.BIN
      grows every frame and plays back (it was truncated before).
- [ ] Commit (two): `Save and load games through owned streams`,
      `Keep the recording stream open and flush it in super-record mode`.

## Step 9: The remaining GameFile and DiskFile users

Mechanical: existence checks, one-shot reads, explicit open/read/write sequences, `Size()` queries,
the TD `map.cc:1018` heap object and RA `Extract()`. Split into a TD commit and an RA commit; the
memory note _parallel forks for mechanical sweeps_ applies (disjoint file groups,
`git clang-format -f -- <own files>`).

**Sites** (from the survey; line numbers at `ae978cf6`):

- _Existence_ (→ `GameFileExists` / `FindExistingFile(...).has_value()`): TD `ending.cc:108`,
  `:291`, `ini.cc:220`, `theme.cc:364`, `:560`, `scenario.cc:177`, `:744`, `:745`,
  `goptions.cc:455`, `expand.cc:78`, `:344`, `:359`, `init.cc:428`, `:473`, `:1383`, `:1387`,
  `:1408`, `:1412`, `:1433`, `:1437`, `:1461`; RA `movie.cc:79`, `theme.cc:606`,
  `installation.cc:326`, `scenario.cc:407`, `:1233`, `:1751`, `init.cc:2170`, `:2177`, `:2186`,
  `:2232`, `:2237`, `:2238`, `:2245`, `:2246`, `:2294`, `:2299`, `sendfile.cc:335`,
  `nulldlg.cc:2816` (checked at `:2932`, `:3154`), `:5766`, `netdlg.cc:2538`, `:4278` (checked at
  `:4340`, `:4606`), `saveload.cc:751`.
- _One-shot reads_ (→ `if (const auto file = OpenGameFile(n)) { file->...; }`): TD
  `display.cc:424-452` (10), `sidebar.cc:1218`, `mapsel.cc:1158`, `:1170`, `ini.cc:348`, `:579`,
  `audio.cc:618`, `ending.cc:195`, `:197` (`ReadBytes(file->Size())`), `internet.cc:123`
  (`OpenDiskFile`), `:249`, `mplayer.cc:455`, `:971`, `options.cc:494`, `stats.cc:279`,
  `expand.cc:181`, `:200`, `mapeddlg.cc:3234`, `:3561`, `init.cc:334`, `:2856` (`OpenDiskFile`); RA
  `vortex.cc:1025`, `version.cc:332`, `:579` (`OpenDiskFile`), `egos.cc:419` (one stream for `Size`,
  `Read`, `Size`), `egos.cc:651`, `expand.cc:360`, `audio.cc:601`.
- _Read then write the same file_ (two scoped streams, read first): TD `mplayer.cc:812`,
  `options.cc:652`, `ini.cc:707` (cheat-keys path, compiled), `score.cc:655`; RA `score.cc:342`.
- _Explicit sequences_: TD `map.cc:924` (read loop), `map.cc:1018` (`new GameFile` → a
  `std::unique_ptr<ByteStream>` from `OpenGameFile(fname, FileAccess::kWrite)`; delete the
  `delete`); RA `sendfile.cc:344` (`std::filesystem::remove` then
  `OpenDiskFile(save_file_name, FileAccess::kWrite)`), `sendfile.cc:612`, and `Extract()`
  (`init.cc:2612`: `in = OpenGameFile(filename)`, `out = OpenGameFile(outname, kWrite)`, return if
  either is null, copy in 32 KiB blocks until `Read` returns 0).
- _Size only_ (→ `GameFileSize`): RA `wol_gsup.cc:2871`, `nulldlg.cc:3792`, `netdlg.cc:5069`,
  `saveload.cc:1027`/`:1044`.

- [ ] Convert TD's sites; both builds; tests; TD smoke; commit
      `Open Tiberian Dawn's files as streams`.
- [ ] Convert RA's sites; both builds; tests; RA smoke; first-run `Extract()` check on the Steam
      data (move GENERAL3.MIX/GENERAL4.MIX aside, start RA, confirm they are recreated
      byte-identical: `cmp`); commit `Open Red Alert's files as streams`.
- [ ] `grep -rnE '\b(GameFile|DiskFile|MemoryFile|FileSource|FileSink)\b' src --include='*.cc'     --include='*.h' | grep -v '_test\.cc' | grep -vE 'src/tech/(game_file|disk_file|memory_file|     file_source|file_sink|file)\.'`
      lists only comments and dead-macro/unbuilt code.

## Step 10: Delete File and its implementations

- [ ] Delete `src/tech/file.{h,cc}`, `src/tech/file_test.cc`, `src/tech/memory_file.{h,cc}`,
      `src/tech/file_source.{h,cc}`, `src/tech/file_sink.{h,cc}`, the `GameFile` and `DiskFile`
      classes (their headers keep the free functions), and `disk_file_test.cc`'s tests of auto-open
      and `IsAvailable` renaming (`ReadOpensAndClosesImplicitly`, `WriteOpensAndClosesImplicitly`,
      `IsAvailableRetriesLowercaseNameAndRenames` — the last is covered by
      `OpenDiskFileWritesTheExistingLowercaseFile` and a `FindExistingFile` test, which it becomes).
      Update `src/tech/CMakeLists.txt`'s test list.
- [ ] Fix every comment that names the deleted classes: `byte_source.h:99`, `byte_sink.h:104`,
      `mix_archive.h:48`, `search_paths.h:19`, `game_file_vqa_io.h:16`, `ra/assets_test.cc:12`,
      `ra/saveload.cc:571`. Leave history comments ("Originally RAWFILE.H ...") in the surviving
      headers.
- [ ] Update `docs/FILE_IO_REFACTOR_PLAN.md`'s header note (as the streams refactor did) and the
      memory notes `file-io-refactor-plan.md` and `file-api-modernization-preferences.md`, which
      describe `File` as current.
- [ ] Full verification (below); commit `Delete the File interface`.

## Verification

- Every commit: `build/` and `build-strict/` clean (`tools/strict_tu.py <files>` in the edit loop,
  the full strict build before committing), `ctest --test-dir build-strict --output-on-failure`.
- From Step 3: `tools/ra_saveload_smoke.sh build-strict/src/ra/rasdl` and
  `tools/td_saveload_smoke.sh build-strict/src/td/tdsdl SCG01EA --team`, binaries passed explicitly
  (the scripts' default binary may be stale).
- `-DRA_LANGUAGE=german` and `-DRA_LANGUAGE=french` still build (Step 6 and Step 10).
- ASan `-NEWGAMESCG01EA -QUITFRAME100` in both games after Steps 8 and 10: no new leaks (the streams
  are owned by `unique_ptr`; the old `new GameFile` in TD `map.cc` goes).
- A real-display pass after Steps 7 and 10 (headless never loads palettes): both intros and title
  screens, TD map selection and ending, a streamed score, a screenshot, save and load from the menu
  with the file list showing dates.
- After Step 10:
  `grep -rn 'tech/file.h\|IO_Open_File\|Find_First_File\|Disk_Space_Available\| fopen\|fread\|fwrite\|fseek\|ftell\|unlink\|fnmatch\|statvfs' src`
  finds nothing outside dead-macro and unbuilt code.

## Risks

- **A converted caller that used to rely on auto-open now reads nothing.** Mitigated by keeping
  `File` separate from `ByteStream` until Step 10, so the compiler refuses any `GameFile` passed
  where a stream is expected; the only remaining risk is a caller whose `if` drops a fallback, which
  the smoke scripts and real-display passes cover.
- **Write order on files read and rewritten** (config, high scores): the read stream must be closed
  before the `kWrite` open truncates. The table pattern scopes it; review each of the five
  read-then-write sites by eye.
- **Lazy truncation is gone:** `FileSink` opened for write only on the first non-empty write; the
  new callers truncate when they open. Every such caller writes immediately, so no file is left
  empty that was not before.
- **`std::filebuf` read errors are invisible.** Reads of a directory are refused at open; an I/O
  error on a real disk now looks like a short file. Accepted: the game treats a short read as
  corrupt data either way.
- **Performance of `std::filebuf` vs stdio** on the nested-mixfile path: measured in Step 3.

## Progress

- 2026-09-22: plan written from two surveys of the tree at `ae978cf6`.
- 2026-09-22: Step 1 landed. Deleted `FuseClass::Fuse_Read/Fuse_Write`,
  `SessionClass::Save/Load(GameFile&)`, `Read_Line(File&, ...)`, TD's `Load_Picture`, the seven
  unused-local sites (`const GameFile file;` in td/udata.cc, td/idata.cc x2, ra/udata.cc,
  ra/idata.cc; `const DiskFile file("tabs.shp");` in ra/tab.cc;
  `const GameFile loadfile("SAVEGAME.NET");` in ra/wol_gsup.cc), and the dead
  `Load_Alloc_Data(cfile)` + `delete[]` in td/startup.cc. Removed the ten stale `tech/file.h`
  includes (ra/anim.h, ra/saveload.h — replaced with `<span>`, td/base.h, td/factory.h, td/layer.h,
  td/mouse.h, td/score.h, td/sidebar.h, td/trigger.h, tech/byte_stream.h), plus includes that went
  unused as a side effect: `tech/file_sink.h`/`tech/file_source.h` in ra/session.cc,
  `tech/game_file.h` in td/udata.cc/td/idata.cc/ra/udata.cc/ra/idata.cc, `tech/disk_file.h` in
  ra/tab.cc, `td/jshell.h` and the `port/bytes_of.h` in td/startup.cc. Every deletion was confirmed
  dead first (grep across the tree, including tests). Both `build` and `build-strict` build clean;
  `ctest --test-dir build-strict` is 713/713 passed.
- 2026-09-22: Step 2 landed. `ByteStream` gained `File`'s typed helpers verbatim (`ReadObject`,
  `WriteObject`, `ReadBytes`, `ReadString`, the typed-span/count/char-array `Read`/`Write`
  overloads); `DiskStream`, `MemoryStream` and `RangeStream` each got `using ByteStream::Read;`/
  `using ByteStream::Write;` so their byte overrides don't hide the templates. `File` keeps its own
  copies untouched, on purpose, so the compiler still flags every caller that hands a `File` where a
  `ByteStream&` is now expected. Ported `file_test.cc`'s six cases to `byte_stream_test.cc` over
  `DiskStream`/`MemoryStream`; `ArraysAreObjectsToo` and `SpanAndRawPointerReadsAgree` moved to
  `MemoryStream` since it can't write, so `ArraysAreObjectsToo` builds its `MemoryStream` straight
  from the encoded bytes rather than round-tripping through `WriteObject`. Raw array/`std::array`
  indexing in the new tests uses `base::At`/`.at()` per the strict build's bounds-check rules, which
  the brief's snippets didn't need since `misc-const-correctness` only complained once the members
  existed. RED: `cmake --build build-strict --target tech_test` failed with
  `no member named 'ReadObject'/'ReadString'/'ReadBytes' in 'DiskStream'/'MemoryStream'`. GREEN:
  same build clean;
  `ctest --test-dir build-strict -R 'MemoryStreamTest|DiskStreamTest|RangeStreamTest'` 14/14 passed;
  full `tech_test` 250/250 passed. Both `build` and `build-strict` build clean;
  `ctest --test-dir build-strict` is 719/719 passed (up from 713 in Step 1, the six new
  `byte_stream_test.cc` cases).
- 2026-09-23: Step 3 landed. `DiskStream` sits on `std::filebuf` instead of the stdio `IO_*`
  wrappers; `Open()` refuses a directory up front (a filebuf cannot tell a read error from end of
  file), `Read`/`Write` go through `sgetn`/`sputn` over `port::CharBytes`, `Seek` reports the
  pre-seek position on a seek before the start (the `pubseekoff` failure case), and `Size()`
  round-trips the position through `pubseekoff`. `IO_Open_File`, `IO_Close_File`, `IO_Read_File`,
  `IO_Write_File`, `IO_Seek_File`, `IO_Get_File_Size` and `IO_Delete_File` are deleted from
  `sdllib/file.{h,cc}`, along with `StdioOrigin`/`SeekOriginFromStdio`/`<cstdio>` from
  `base/seek_origin.h`; `FindExistingFile` now probes with `DiskStream::Open` and
  `DiskFile::Delete`/ `GameFile::Delete` call `std::filesystem::remove`. `sdllib/file_test.cc`'s two
  `IO_Open_File` tests are gone (their coverage is `DiskStreamTest.ReadWriteKeepsExistingContents`/
  `ReadWriteCreatesMissingFile`, converted to `DiskStream::Open`), and `stream_error_test.cc`'s
  directory test now expects `DiskStream::Open`/`DiskFile::Open` to fail outright instead of opening
  and then failing the read. Per the controller: also strengthened
  `DiskStreamTest.ObjectsRoundTripThroughWriteObjectAndReadObject` to round-trip a `Header`, a
  `std::array<int16_t, 3>` and a trailer through one `kReadWrite` stream (mirroring
  `file_test.cc:25`'s three-object test), and added `MemoryStreamTest.TypedViewCountStopsMidElement`
  (the count=3-bytes-into-two-uint16_t-elements case from `file_test.cc:87`) alongside the existing
  count=2 case. RED: with the old stdio implementation still in place, `tech_test` failed
  `DiskStreamErrorTest.OpenRefusesDirectory` (fopen on `/tmp` succeeds on this glibc) and
  `StreamErrorTest.DiskReadErrorReachesFileAndStraw`. GREEN: same build clean; both suites pass.
  Strict findings fixed beyond the brief's snippet: an explicit `~DiskStream() override = default;`
  (rule-of-five), `<iosfwd>` for `std::streampos`, `static_cast<std::streamoff>(...)` instead of the
  functional-style `std::streamoff(...)` cast, and a `default: break;` in the access `switch`. Both
  `build` and `build-strict` build clean; `ctest --test-dir build-strict` is 724/724 passed (up from
  719: +7 `byte_stream_test.cc` cases — 4 `DiskStreamTest`, 2 `DiskStreamErrorTest`,
  `TypedViewCountStopsMidElement` — minus the 2 retired `sdllib_test` `IO_Open_File` cases). Both
  smoke scripts print OK against `build-strict/src/ra/rasdl` and `build-strict/src/td/tdsdl`.
  Performance: no clean isolated load-time number was practical (the game does not print one), so
  per the controller's fallback the whole headless run (`-NOMOVIES -LOADGAME0 -QUITFRAME120`) was
  timed 3x on the RA binary before and after, both in `build-strict`: before (stdio) 3.95/3.97/3.97
  s, after (`std::filebuf`) 3.87/3.88/3.93 s — no regression. The real-display intro-movie decode
  check from the brief was not run (headless environment); flagged as not covered by this step.
