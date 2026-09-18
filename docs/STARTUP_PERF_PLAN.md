# Startup and Save/Load Performance Plan

Measured 2026-09-17 on `build/` (RelWithDebInfo), Red Alert, Steam game data,
`SDL_VIDEODRIVER=dummy`. Profiled with `valgrind --tool=callgrind` (`perf` is blocked:
`kernel.perf_event_paranoid=4`) and a temporary wall-clock log in `PaletteClass::Set` (reverted).

## Findings

**Save/load itself is not slow.** A `-LOADGAME5 -QUITFRAME61` run takes 1.53 s wall; the load and
everything else the game computes is about 80 ms of it. The rest is four palette fades:

| When                                  | Fade         | Wall              |
| ------------------------------------- | ------------ | ----------------- |
| Title page fades in (`Init_Game`)     | Slow, 30 t   | 467 ms            |
| Title fades to black (`Select_Game`)  | Medium, 15 t | 236 ms            |
| Map fades in (`RunGame`)              | Medium, 15 t | 239 ms            |
| Fade to black on quit (`EndScenario`) | Slow, 30 t   | 484 ms            |
| **Total**                             |              | **1426 ms (93%)** |

callgrind agrees: 97.3% of all instructions are inside `PaletteClass::Set`, almost all of it
`Video_End_Frame` → `SDL_RenderPresent`. The fade loop in `src/ra/palette.cc` spins with no pacing
and re-presents the whole window on every pass: about 1,150 full-screen presents per second (570 in
a half-second fade), which pins a core for the whole fade and gains nothing over 60 Hz.

**Starting to the main menu takes 11.3 s:**

| Phase                                                                                | Wall    |
| ------------------------------------------------------------------------------------ | ------- |
| Fade to black before the logo movie, when the screen is already black (`Play_Movie`) | 228 ms  |
| Westwood logo movie `VQ_REDINTRO` (`Play_Intro`, every launch)                       | ~10.5 s |
| Title page fade-in                                                                   | 482 ms  |
| Everything else: MIX caching with SHA-1, rules, INI, heaps                           | ~100 ms |

ESC already skips the movie (`BreakoutAllowed` defaults to true).

**The Debug build directories are the other "forever".** `cmake-build-debug-ra/` and
`cmake-build-strict-ra-clang/` are `Debug`. There the bounds checks in LCW, Blowfish, SHA-1 and the
blit are out-of-line calls: bootstrap 1836 ms instead of 45 ms, a save load 483 ms instead of 19 ms,
and the intro movie can't decode in real time (10.5 s of video takes 22 s). Launching from one of
those CLion profiles makes startup about 25 s.

## Plan

Ordered by how much time each step saves per launch.

### 1. Measurement first (small)

- Add a `ScopedPhaseTimer` (`DLOG(INFO)` of wall ms, compiled out in release) and wrap `Init_Game`'s
  phases, `Play_Movie`, `PaletteClass::Set`, `Load_Game`/`Save_Game` and `MixArchive::Cache`.
- `tools/ra_startup_bench.sh`: runs the headless load 5 times and reports the median wall time. It
  creates its own save slot first, because a missing slot hangs at the menu.
- Optional: `sudo sysctl kernel.perf_event_paranoid=1` makes `perf record -g` usable. It is about
  50x cheaper than callgrind and shows wall time, not instruction counts.

### 2. Fix the fade loop (`src/ra/palette.cc`)

- **Return early when the target palette equals `CurrentPalette`.** This drops the 228 ms
  black-to-black fade before every movie; there are similar no-op fades after movies and between
  menus.
- **Pace the loop to one present per display refresh** (`SDL_Delay` until the next 1/60 s, or rely
  on vsync where the renderer has it). The fade lasts just as long, but CPU use goes from 100% of a
  core to about 3%, and the fade looks the same.
- **Skip fades in automated runs.** Add a global `-NOFADE` (and make `-QUITFRAME`/`-LOADGAME` imply
  it) so `Set(fade)` becomes `Set()`. The headless load drops from 1.53 s to about 0.1 s, and the
  save/load smoke scripts and benchmarks get about 15x faster.
- **No fade-out when quitting the program.** The 484 ms `BlackPalette.Set(kFadePaletteSlow)` before
  exit only delays closing the window. Keep the fade when a mission ends and the game goes back to
  the menu.

Expected result: a headless load goes from 1.53 s to about 0.1 s. Interactive play keeps the same
fades at a fraction of the CPU.

**Done 2026-09-17.** As built: only `-QUITFRAME` implies `-NOFADE`, because `-LOADGAME` is also
useful interactively. The menu's Exit no longer fades; the rarely used "patch downloaded" exit still
does. Measured:

- Headless `-LOADGAME5 -QUITFRAME61`: 1.77 s → 0.29 s wall, 1.53 s → 0.08 s CPU.
  `tools/ra_saveload_smoke.sh` still passes (240 positions identical), in 12.5 s end to end.
- Three seconds of normal startup under `-NOMOVIES`: same wall time, 0.45 s less CPU (the title fade
  no longer spins).

New finding: once the main menu is up, it also runs a core at 100% (2.5 s CPU in a 3 s run). That
costs power, not startup time; pace it like the fade loop.

### 3. Intro movie policy (needs your decision)

The logo movie is 93% of launch-to-menu time. Options:

- a. Play it only on the first launch and remember that in `REDALERT.INI`
  (`[Options] SkipIntro=yes`). Later launches reach the menu in about 0.6 s.
- b. Keep it, and add `-NOMOVIES` to the CLion run configurations only.
- c. Keep the original behaviour (ESC skips it).

Recommended: (a), plus an options-menu toggle later.

### 4. Make Debug builds usable (`CMakeLists.txt`)

- Compile the hot kernels at `-O2` in every build type:
  `set_source_files_properties(tech/lcw.cc tech/sha.cc tech/blowfish*.cc sdllib/*blit*.cc ... PROPERTIES COMPILE_OPTIONS -O2)`.
  Stepping into game logic still works, and Debug bootstrap should get close to the 45 ms optimized
  figure.
- Or mark `base::At` `[[gnu::always_inline]]` and check whether `-Og` is enough.
- Switch the day-to-day CLion profile to `RelWithDebInfo`.

### 5. MIX SHA-1 verification (about 50 ms optimized; low priority)

`MixArchive::Cache()` hashes every archive it caches, about 13 MB at startup and ~1 MB per theater,
and it is the largest remaining CPU cost (callgrind: 1.5% of the load run, most of the non-fade
work). Options, from least to most change:

- Verify once per `(path, size, mtime)` and record the result in a small cache file.
- Hash on a worker thread while the title page is up.
- Hardware SHA-1 (SHA-NI) behind a CPU-feature check.

Only worth doing after steps 2–3, when it is a visible share of the time.

## Verification

- `tools/ra_startup_bench.sh` before and after every step. Record the medians here.
- `tools/ra_saveload_smoke.sh` after step 2, to show that skipping fades changes no game state.
- Interactive check on `DISPLAY=:0`: fades still look smooth, ESC still skips the movie, and the
  first-launch intro plays exactly once.
