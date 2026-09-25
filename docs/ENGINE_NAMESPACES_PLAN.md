# Engine namespaces plan

Decided 2026-09-25. Replaces the "no project prefix" row of `/migrate-namespaces`: the engine
libraries go in `engine::<lib>`, and `base` leaves `src/engine/` to become a top-level library with
a top-level `base::` namespace, the way Chromium lays out `base/`.

## Findings

- The games are to be split into their own namespaces and folders later (`ra::`, `td::`, perhaps
  `ra::audio`, `ra::net`). With the engine in top-level `audio::`, `net::`, `file::`, a game
  sub-namespace of the same name hides the engine's inside that game: in `namespace ra::audio`,
  `audio::TheAudio()` looks in `ra::audio` and fails. The style guide warns against exactly this
  ("avoid nested namespaces that match well-known top-level namespaces"). `ra/` already has
  `audio.cc`, `base.cc` and `file` sources, so the clash is likely, not hypothetical.
- `engine::` everywhere would cost `base::` most: it is written 9,398 times in `src/` (2026-09-25).
  The other namespaced libraries are cheap: `platform::` 108 uses in 36 files, `file::` 22 in 3,
  `crypto::` 12 in 3, `net::` 6 in 4. `gfx`, `stream`, `codec`, `window`, `audio`, `video` and `vqa`
  are still global.
- `base` is vocabulary (`base::ssize`, `SafeCopy`, `Installed`, `EnumArray`), used by the games and
  tools as much as by the engine, and depends on nothing in it. It is not an engine domain.
  `docs/ENGINE_FOLDERS_PLAN.md` put it under `src/engine/` when it merged the old `base`, `port` and
  `tech` vocabulary headers; the merge stays, only the folder moves.
- `#include "engine/base/..."` appears 1,221 times in 544 files. Header guards are
  `CNC_RED_ALERT_ENGINE_BASE_*`, which cpplint checks against the path.

## Plan

The top-level namespaces become `base`, `engine`, and later `ra` and `td`. No game may declare a
nested `base` or `engine` namespace (the games' AI `BaseClass` in `base.cc` gets another folder name
when the games are split).

1. **Move `src/engine/base/` to `src/base/`.** Folder, includes (`"base/types.h"`), header guards
   (`CNC_RED_ALERT_BASE_*`), the CMake target (`engine_base` -> `base`, test `base_test`), the
   clang-tidy header filter, `tools/engine_layout.py` and `tools/check_layers.py` (base is a root
   library outside `src/engine/` that every engine library may include and that includes no engine
   library), CLAUDE.md, `docs/TYPE_MIGRATION.md` and the `.claude/commands`. Namespace unchanged.
2. **Nest the namespaced engine libraries in `engine`.** `platform`, `crypto`, `file` and `net`
   become `engine::platform` and so on (`namespace engine::platform {`), callers write
   `engine::platform::`. Inside `engine`, other engine libraries may write `platform::` (unqualified
   lookup reaches `engine::platform` from any `engine::x`).
3. **Update `/migrate-namespaces`** so the remaining libraries go straight into `engine::<lib>`,
   then continue with them (`audio` first: its global-namespace pass is stashed as "audio namespace
   (pre engine:: decision)" and is redone as `engine::audio`).

## Verification

Per step: `cmake --build build --parallel 22 && ctest --test-dir build --output-on-failure`
(includes `engine_layers_test` and `cpplint_test`), then the strict build
(`cmake --build build-strict --parallel 14`, foreground) before committing. Step 1:
`git grep 'engine/base\|ENGINE_BASE\|engine_base' -- ':!docs/*_PLAN.md'` is empty. Step 2: `nm -C`
of each library shows no symbols outside `engine::`, `std::`, `absl::`.

## Progress

- [x] 1. `base` to `src/base/`
- [x] 2. `engine::` for platform, crypto, file, net
- [ ] 3. `/migrate-namespaces` policy; `engine::audio`
