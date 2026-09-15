# Plan: enable `concurrency-mt-unsafe`

Written 2026-09-15 against `.clang-tidy` and clang-tidy 23.1.2. Companion to the row in
[CLANG_TIDY_PRIORITIES.md](CLANG_TIDY_PRIORITIES.md).

## Context

`concurrency-mt-unsafe` reports calls to C library functions that keep hidden global state. The row
was skipped on 2026-09-12 because every call runs on the main game thread, which is true and is not
the reason to fix them: the hidden state is what makes the code fragile. `strtok` keeps its cursor
in a global, so a parser cannot call anything that might also tokenize (`ra/wol_gsup.cc` works
around this with "SetPlayerColor may call strtok" and hand-advanced offsets); `rand` is a second
global generator next to the game's own `RandomClass` (`ra/wol_gsup.cc` reads `Seed = rand()` before
the `srand` that was meant to seed it); `getenv` returns a pointer into the environment block;
`inet_ntoa` returns a static buffer; `gethostbyname` is the deprecated IPv4-only lookup on every
platform. Replacing them is the kind of modernization `CLAUDE.md` asks for ("use `std::string` with
Abseil functions").

A fresh isolated sweep on 2026-09-15 over the 920 compile-database entries reports **322 unique
sites**:

| Function        | Sites | Where                                                                               |
| --------------- | ----- | ----------------------------------------------------------------------------------- |
| `strtok`        | 268   | INI object, team, trigger and rules parsers; phone books; WOL game-setup messages   |
| `exit`          | 22    | Fatal-error and quit paths                                                          |
| `rand`          | 21    | Seed generation (8), WOL sound-effect picks (12), the public-key self-test (1)      |
| `getenv`        | 4     | `RA_SAVE_DUMP`, `TD_MAP_TRACE`, `CNC_TEST_CORE_DUMPS`, and `PATH` in the RA game ID |
| `inet_ntoa`     | 3     | WOL ping requests and the game-results IP                                           |
| `gethostbyname` | 2     | Local address enumeration in `ra/stats.cc` and `ra/wspudp.cc`                       |
| `glob`          | 2     | `Find_First_File` on POSIX (`sdllib/file.cc`)                                       |

Largest files: `ra/wol_gsup.cc` 37, `td/teamtype.cc` 32, `ra/teamtype.cc` 20, `ra/rawolapi.cc` 17,
`td/mplayer.cc` 14, `ra/session.cc` 13, `ra/tevent.cc` 11. The `strtok` calls in `winvq/vqm32` and
`winvq/vqaview` are not in any build and are out of scope.

## How the check classifies (read before fixing)

- The option `FunctionSet` selects the list: `posix` (the default `any` adds glibc's list). Probed
  on the installed tool: `strtok`, `rand`, `getenv`, `inet_ntoa`, `localtime`, `strerror` and
  `ctime` are in the POSIX list; `exit`, `glob` and `gethostbyname` are only in glibc's.
- The message never names the function; classify by reading the source column.
- The check has **no fix-its**.
- `WarningsAsErrors: '*'` makes the isolated sweep print `error:` lines.
- Enforced siblings that shape the fixes: `misc-include-cleaner` (every new `port/` include must be
  used directly), `misc-const-correctness` (tokens bound as `const char*` where not modified),
  `modernize-use-nodiscard`, `cppcoreguidelines-pro-type-reinterpret-cast` (no casts in the
  `getaddrinfo` loops: `sockaddr_in` comes back through `port::AlignedObject`/`ReadUnaligned`).

## Decisions

- **`exit` stays.** `std::exit` is the correct call on every fatal path; glibc lists it only because
  two threads must not exit at once. Set `concurrency-mt-unsafe.FunctionSet: posix` with a comment,
  rather than hide 22 calls behind a wrapper or a NOLINT each. `glob` and `gethostbyname` are then
  not reported either, but both are replaced anyway (below) because the replacements are better
  APIs, not to satisfy the check.
- **`strtok` becomes `port::Tokenizer`** (`port/tokenizer.h`): a cursor over a writable C string
  with the same splitting rules (runs of delimiters skipped, tokens terminated in place, `nullptr`
  at the end) so the 40 `From_Name`-style consumers keep their `const char*` parameters, plus
  `Remaining()` for the two length-prefixed WOL messages. The state lives in the object, so nested
  parsing is safe and the WOL offset arithmetic goes away.
- **`rand` goes away entirely.** Seeds come from `port::RandomSeed()` (`std::random_device`, one
  place); WOL sound picks use `Sim_Random_Pick`, the game's own non-synchronizing generator; the
  public-key self-test fills its block from a `std::minstd_rand`. `randomize()`, `IRandom()` and
  `Get_Random_Mask()` in `sdllib/misc.cc` are dead once the seed sites change and are deleted, with
  TD's `srand(Seed)` that fed them. This also clears the four `rand`-specific checks in P5
  (`misc-predictable-rand`, `cert-msc30-c`, `cert-msc50-cpp`,
  `clang-analyzer-security.insecureAPI.rand`), which are enabled in the same commit.
- **`getenv` becomes `port::GetEnv`** returning `std::optional<std::string>`: the one documented
  call, and callers hold a copy instead of a pointer into the environment.
- **`inet_ntoa` becomes `port::Ipv4Text`** (`inet_ntop` into a `std::string`).
- **`gethostbyname` becomes `getaddrinfo`** with an `AF_INET` hint; the two loops read `sockaddr_in`
  from `ai_addr`.
- **`glob` becomes `std::filesystem::directory_iterator` + `fnmatch(FNM_CASEFOLD)`**, which also
  replaces the "search again in lowercase" pass with a real case-insensitive match. `stat` still
  supplies the modification time.

## Fix strategy by group

| Group                                                                                                                                                                                                                                                                                                                                          | Sites | Fix                                                                                                                                                                                                                                                                          |
| ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **A. Sequential field parsers** over a local buffer: object `Read_INI` in both games, `base.cc`, `smudge.cc`, `terrain.cc`, `template.cc`, `overlay.cc`, `ccini.cc`, `rules.cc`, `warhead.cc`, `taction.cc`, `tevent.cc`, `trigtype.cc`, `teamtype.cc`, `trigger.cc`, `mapeddlg.cc`, `house.cc`, `init.cc`/`startup.cc` (dotted IPX addresses) | ~210  | `port::Tokenizer tokens(buf, ",");` then `tokens.Next()` for each `strtok` call; `tokens.Next(",:")` where the delimiter set changes mid-entry. A discarded field is `tokens.Next();` with a comment.                                                                        |
| **B. Loop parsers**: `Get_Owners`, `Get_Buildings`, TD allies, `iconlist.cc` line breaks, `wolapiob.cc` SKU list, `nullmgr.cc` init strings, `mplayer.cc` word shuffle                                                                                                                                                                         | ~20   | Same object; `while (char* token = tokens.Next())`. The SKU list no longer needs two buffer copies: count with one tokenizer, or size the array from the count and parse once.                                                                                               |
| **C. Phone books** (`ra/session.cc`, `td/mplayer.cc`)                                                                                                                                                                                                                                                                                          | 26    | One tokenizer per entry; nothing else changes.                                                                                                                                                                                                                               |
| **D. WOL messages** (`ra/wol_gsup.cc` game params, guest info, go message; `ra/rawolapi.cc` server list)                                                                                                                                                                                                                                       | 54    | Tokenizer per message. `Remaining()` replaces `szToken + 4` / `+ 3` for the length-prefixed strings; the "may call strtok" offset hops become plain `Next()` calls. The server-list parse copies `conndata` into a local buffer first (the follow-up left by the cast work). |
| **E. Seeds and picks**                                                                                                                                                                                                                                                                                                                         | 21    | `Seed = port::RandomSeed()`; `Scen.sync_rng_.set_seed(...)` likewise; `Sim_Random_Pick(0, n - 1)` for the WOL sounds; `std::minstd_rand` in `tech/pk.cc`.                                                                                                                    |
| **F. Environment, addresses, file search**                                                                                                                                                                                                                                                                                                     | 11    | `port::GetEnv`, `port::Ipv4Text`, `getaddrinfo`, `std::filesystem` as decided above.                                                                                                                                                                                         |

## Steps

Each step is one or more commits; each commit passes the strict build and CTest, and the parser
steps also pass both save/load smoke scripts.

1. **Helpers.** `port/tokenizer.{h,cc}` + test; `port/env.{h,cc}` + test; `port/random_seed.{h,cc}`
   - test; `port/inet_text.{h,cc}` + test. No call-site change.
2. **Parsers (A, B, C)**, one game per commit, then the WOL messages (D).
3. **Random (E)**, including the `sdllib/misc.cc` deletions and the four P5 `rand` checks.
4. **Environment, addresses, file search (F).**
5. **Enable.** Remove `-concurrency-mt-unsafe` from `.clang-tidy`, add the `FunctionSet` option;
   full strict rebuild; priorities rows to **Enabled**; a "Thread-unsafe function removal (date)"
   review section.

## Verification

- **Isolated sweep** per the tree-wide recipe, with `--checks="-*,concurrency-mt-unsafe"`; classify
  the `error:` lines by reading the source column; target zero.
- **Knock-on sweeps:** `misc-include-cleaner`, `misc-const-correctness`,
  `readability-make-member-function-const`, `modernize-use-nodiscard`.
- **Full strict rebuild** in foreground chunks:
  `timeout 590 cmake --build cmake-build-strict-ra-clang --parallel 10 -- -k 0` until
  `ninja: no work to do`.
- **Tests:** CTest, then `tools/ra_saveload_smoke.sh` and `tools/td_saveload_smoke.sh` with the
  `--team` and `--building` fixtures. Every INI object, team and trigger parser is on the smoke
  path; the phone books, WOL messages and address lookups are not and stay a manual check.
- **Probe:** a scratch file calling `strtok` must still be reported once the check is on.

## Risks

- A parser that reads past the last field relied on `strtok(nullptr, ...)` returning `nullptr`
  forever; `Tokenizer::Next()` does the same, and the unit test pins it.
- `fnmatch(FNM_CASEFOLD)` matches mixed-case names the two-pass glob did not; that is the intended
  direction for a case-insensitive game filesystem.
- `getaddrinfo` may return the same address more than once (one per socket type); the local address
  lists tolerate duplicates, and the hint restricts to `SOCK_DGRAM` anyway.
