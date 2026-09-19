# Command Line as String Views Plan

## Findings

Both games hand `Parse_Command_Line` a `std::span<char*>` over the raw `argv`, which needs a
`clang-diagnostic-unsafe-buffer-usage-in-container` NOLINT at every entry point. Red Alert's parser
uppercases each argument in place (`port::MutableCString`), and both games tokenize the `-DESTNET`
address in place with `port::Tokenizer`. On Windows, `WinMain` rebuilds a DOS-style `char* argv[20]`
by hand, writing terminators into the command line and dropping everything past the 19th argument.
Tiberian Dawn also threads `argc`/`argv` through `Main_Game` into `Init_Game`, which ignores them.

**Why not Abseil flags.** `absl::ParseCommandLine` expects `--name=value`. The game's switches glue
their value on (`-LOADGAME3`, `-SEED1`, `-CD<path>`), `-X` packs letters (`-XQ`), and the cheat and
editor codes are bare phrases matched by `HashKeyPhrase`. Moving to flags would change the game's
command line and both smoke scripts; the syntax stays.

## Plan

1. **Entry points** (`src/ra/startup.cc`, `src/td/startup.cc`). Build a
   `std::vector<std::string_view>` of the arguments without the program name, and a
   `std::filesystem::path` for the executable. POSIX: one span over `argv`, `views::drop(1)`.
   Windows: `absl::StrSplit(command_line, absl::ByAnyChar(" \r"), absl::SkipEmpty())` and
   `GetModuleFileName`, with no argument cap.
2. **Parsers** (`src/ra/init.*`, `src/td/init.*`).
   `Parse_Command_Line(std::span<const std::string_view>)`; uppercase a local copy instead of
   `argv`; `ApplyDestNetArgument` takes a `std::string_view` and splits it with
   `absl::StrSplit(..., '.', SkipEmpty())`.
3. **TD dead parameters.** Drop the unused `argc`/`argv` from `Main_Game` and `Init_Game`.

One commit per game.

## Verification

- `build/` and the strict dir build with no new findings.
- `tools/ra_saveload_smoke.sh` and `tools/td_saveload_smoke.sh` pass (they use `-CD`, `-NOMOVIES`,
  `-SEED`, `-NEWGAME`, `-SAVESLOT`, `-LOADGAME`, `-QUITFRAME`).
- `rasdl -?` prints usage; `rasdl -XZ` prints the invalid-option message.

## Progress

- Done 2026-09-19: Red Alert (2f928d69), Tiberian Dawn (910a5bef). Both smoke scripts pass; strict
  builds clean.
