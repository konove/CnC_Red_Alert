# Plan: enable `cppcoreguidelines-pro-type-vararg`, `modernize-avoid-variadic-functions` and `cert-dcl50-cpp`

Written 2026-09-15 against `.clang-tidy` and clang-tidy 23.1.2. Companion to the three rows in
[CLANG_TIDY_PRIORITIES.md](CLANG_TIDY_PRIORITIES.md); `hicpp-vararg` is the same rule under a name
the installed toolchain does not ship and stays excluded as **Legacy**.

**Status: complete (2026-09-15).** The three checks are enabled; the results, including the eight
broken formats the compile-time check found, are in the "Variadic printer enablement" review in
CLANG_TIDY_PRIORITIES.md.

## Context

The rows were skipped on 2026-09-12 because most of the game's printers format a translated string
looked up at run time, so "a parameter-pack replacement still needs a runtime-typed formatter behind
it". Abseil ships that formatter: `absl::FormatUntyped` takes a run-time `absl::UntypedFormatSpec`
and a span of `absl::FormatArg`, checks every conversion against the argument it receives, and
returns `false` on a mismatch instead of reading the stack. Probed with the fetched Abseil on
2026-09-15: `%s` with an `int`, `%d` with a `char*`, a stray `%` and a missing argument all fail
cleanly; enums, `bool`, `int64_t`, `%*d`, `%.*s`, `%p`, `%c`, `%08X`, `%-5d` and the `l`/`h`/`z`
length modifiers all format as `printf` does. For literal formats `absl::FormatSpec<Args...>` checks
the string against the argument types at compile time under clang.

The isolated sweep of the 486 source translation units in the strict compile database on 2026-09-15
reports **1,599 sites** (both games, `tech`, `sdllib`, `winvq`):

| Check                                | Sites | What                                                                   |
| ------------------------------------ | ----- | ---------------------------------------------------------------------- |
| `cppcoreguidelines-pro-type-vararg`  | 1,558 | calls to a C-style variadic function                                   |
|                                      | 17    | `va_list` declarations inside the printers                             |
|                                      | 5     | `va_arg` in `Buffer_Frame_To_Page`                                     |
| `modernize-avoid-variadic-functions` | 19    | the printer definitions (`cert-dcl50-cpp` is its alias and prints too) |

The 1,558 calls by callee:

| Callee                                          | Calls | Format argument                                                              |
| ----------------------------------------------- | ----- | ---------------------------------------------------------------------------- |
| `Fancy_Text_Print` (both games)                 | 490   | 224 text IDs, 136 literals, 125 run-time strings; 465 pass no extra argument |
| `sprintf`                                       | 386   | all literals; 19 with no extra argument                                      |
| `MonoClass::Printf`                             | 203   | 200 literals, 1 text ID, 2 run-time strings with no extra argument           |
| `fprintf`                                       | 124   | all literals; 112 to a dump file `fp`, the rest `stderr`                     |
| `Format_Runtime_Text`                           | 95    | `Text_String(...)` results and WOL string macros                             |
| `Mono_Printf`                                   | 85    | all literals                                                                 |
| `printf`, `Smart_Printf`                        | 48+48 | all literals                                                                 |
| `snprintf`                                      | 41    | all literals                                                                 |
| `Plain_Text_Print` (RA)                         | 20    | 10 literals, 2 text IDs, 7 run-time strings                                  |
| `Buffer_Frame_To_Page`                          | 8     | shape-effect tables and counts read with `va_arg` by flag                    |
| `Fatal`, `OutputDebugString` (a `printf` macro) | 4+3   | literals                                                                     |
| `fcntl`                                         | 1     | POSIX; stays with a `NOLINT` naming it                                       |

The C-library calls are 601 sites in 97 files; `ra/queue.cc` (79) and `td/queue.cc` (58) are the
sync-debug dumps, then the two `netdlg.cc`, `nulldlg.cc` and the WOL files.

## Design

### `port/format.h`

```cpp
namespace port {
// Formats a printf-style string known only at run time (a translated game
// string) with `args`. Returns the string unformatted when it is not a valid
// format for those arguments, so a translated string that is not a format
// never prints as nothing; a mismatch with arguments present is DLOG'd.
std::string FormatRuntime(std::string_view format, absl::Span<const absl::FormatArg> args);
template <typename... Args>
std::string FormatRuntime(std::string_view format, const Args&... args);
// Packs `args` for a Span-taking printer; the array must outlive the call.
template <typename... Args>
std::array<absl::FormatArg, sizeof...(Args)> MakeFormatArgs(const Args&... args);
}
```

`port` links `absl::str_format`. Tests cover the fallback, `%%`, zero arguments, enum and `int64_t`
arguments, and truncation-free output.

### The printers

Each printer keeps its name and its call sites. The `...` becomes an
`absl::Span<const absl::FormatArg> args = {}` parameter on the out-of-line function, and a variadic
template overload constrained to at least one argument packs the arguments and calls it, so a call
with no arguments binds the empty default and a call with arguments deduces the pack:

| Function                                                         | Format source         | New shape                                                                                                                                                                                                                                                                                |
| ---------------------------------------------------------------- | --------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `Fancy_Text_Print`, `Plain_Text_Print` (`const char*` and `int`) | run time              | Span parameter + template; every call formats through `port::FormatRuntime`, so a `%%` in a translated string still prints one `%` and a stray `%` prints verbatim                                                                                                                       |
| `MonoClass::Printf(int)`                                         | run time (text ID)    | Span parameter + template                                                                                                                                                                                                                                                                |
| `MonoClass::Printf(const char*)`, `Mono_Printf`, `Smart_Printf`  | literal at every site | `absl::FormatSpec<Args...>` template, compile-time checked; the two `Printf(run-time string)` calls with no arguments become `Print`                                                                                                                                                     |
| `Fatal`                                                          | literal               | `absl::FormatSpec<Args...>` template over a `[[noreturn]] FatalMessage(const std::string&)`                                                                                                                                                                                              |
| `Format_Runtime_Text(char*, size_t, const char*, ...)`           | run time              | template over `port::FormatRuntime`, still copying into the caller's buffer with truncation; the `va_list` overload goes                                                                                                                                                                 |
| `Buffer_Frame_To_Page(..., int flags, ...)`                      | n/a                   | a `ShapeEffects` struct (`ghost_table`, `fading_table`, `fading_count`, `predator_offset`, `partial_predator`) replaces the flag-driven `va_arg` reads; the four-way call in each `CC_Draw_Shape` collapses to one; the `extern "C"` on a function taking a `GraphicViewPortClass&` goes |
| `Dialog_Message(char*, ...)`, `Debug_Printf` in `vqaplay.h`      | n/a                   | declared, never defined or called: deleted                                                                                                                                                                                                                                               |

`td/savevalues_test.cc` stubs `MonoClass::Printf(const char*, ...)` for linking; the stub moves to
the out-of-line function the template calls.

### The C library

All 601 formats are literals, so each call becomes its Abseil equivalent with the same arguments and
compile-time checking on clang:

| Call                                                   | Replacement                                                                                                                                              |
| ------------------------------------------------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `sprintf(array, ...)`                                  | `absl::SNPrintF(array, sizeof(array), ...)`; this is also the CLAUDE.md safe-string fix for the 386 unbounded writes                                     |
| `sprintf(ptr, ...)` with no size in scope              | look at the buffer: `absl::SNPrintF` with the real size, or a `std::string` from `absl::StrFormat` when the buffer is a local only read as `const char*` |
| `sprintf(buf + strlen(buf), ...)`, `&buf[strlen(buf)]` | `absl::StrAppendFormat` on a `std::string`, or `absl::SNPrintF` on the remaining size                                                                    |
| `snprintf(buf, n, ...)`                                | `absl::SNPrintF(buf, n, ...)`                                                                                                                            |
| `fprintf(fp, ...)`, `fprintf(stderr, ...)`             | `absl::FPrintF(fp, ...)`; the sync dumps stay file dumps rather than becoming `LOG` lines                                                                |
| `printf(...)`                                          | `absl::PrintF(...)`                                                                                                                                      |
| `OutputDebugString(x)` (`#define` over `printf`)       | `absl::PrintF("%s", x)` at the three sites, macro deleted                                                                                                |
| `fcntl(fd, F_SETFL, flags)`                            | `// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)` with the reason: POSIX declares it variadic                                                       |

Abseil's `%s` takes `std::string` and `std::string_view` directly, so `.c_str()` on an argument can
go. A format that does not match its arguments is a compile error under clang; each one is a finding
to fix, not to paper over.

## Steps

Each step is one commit; each commit passes the isolated sweep on its files and the fast build of
both games, and the series ends with the strict build, CTest and both smoke scripts.

1. **Helper.** `port/format.h`/`.cc`, tests, CMake.
2. **Printers.** Both games' text printers, mono printers, `Fatal`, `Format_Runtime_Text`,
   `Smart_Printf`; the test stub; the dead declarations.
3. **Blitter.** `ShapeEffects` for `Buffer_Frame_To_Page` and its eight callers.
4. **C library.** The 601 sites, fanned out to fork agents on disjoint file groups with one recipe.
5. **Enable.** Remove the three names from `.clang-tidy`; full strict rebuild; rows to **Enabled**;
   a "Variadic printer enablement (date)" review in the priorities file.

## Verification

- **Isolated sweep** (~1.5 min) over the unique `src/*.cc` files in
  `cmake-build-strict-ra-clang/compile_commands.json`:

  ```sh
  xargs -P 10 -I{} sh -c 'clang-tidy -p cmake-build-strict-ra-clang --quiet \
      --checks="-*,cppcoreguidelines-pro-type-vararg,modernize-avoid-variadic-functions" "{}" 2>/dev/null' \
      < tus.txt > report.txt
  grep -c 'error:' report.txt   # target: 1, the fcntl NOLINT line does not count
  ```

- **Probe**: a scratch file calling `printf("%d", 1)` must fail under the enabled configuration.
- **Full strict build** in the foreground
  (`timeout 590 cmake --build cmake-build-strict-ra-clang --parallel 10 -- -k 0`, repeated until no
  work), then CTest.
- **Smoke**: `tools/ra_saveload_smoke.sh` and `tools/td_saveload_smoke.sh` with the `--team`,
  `--building`, `--mobile`, `--map` and `--globals` fixtures. The dialogs, map editors, mono debug
  pages and WOL code are not on the smoke path; those stay a manual check.
