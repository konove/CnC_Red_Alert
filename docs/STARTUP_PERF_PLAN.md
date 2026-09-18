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

**Done 2026-09-17: option (a).** The key is `[Intro] LogoPlayed`, next to the original game's
`[Intro] PlayIntro`. That flag already played the full cinematic once on the first launch, but the
logo still played on every launch after it. The first-launch cinematic now also counts as the one
showing, and a `-NOMOVIES` run never uses it up. Launch to menu: 10.88 s the first time, 0.29 s
after that (`-QUITFRAME0`, headless). Delete the key to see the logo again.

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

## perf profile after steps 2–3 (2026-09-17)

`perf` works now (`kernel.perf_event_paranoid=1`). Headless `-LOADGAME5 -QUITFRAME61` with the
default audio driver (PulseAudio on PipeWire): 0.31 s wall, 0.11 s CPU.

**Wall time: 0.19 s of it is waiting for audio to close.** At exit, `SDL_CloseAudioDevice` joins
SDL2's audio thread, which sleeps for two buffer lengths to let the sound drain. `Audio_Init`
(`src/sdllib/ww_audio.cc`) asks for 2048 samples at 22,050 Hz, which is 93 ms per buffer, so the
drain takes 186 ms. strace shows the 184 ms sleep, and the main thread blocked 0.27 s on the join
(slower under strace). SDL's own PipeWire driver doesn't sleep like that: the same run takes 0.12 s.
The same 2048-sample buffer also delays every sound effect by up to 93 ms.

**CPU on the main thread (perf, ~0.1 s):**

| Where                                            | Share |
| ------------------------------------------------ | ----- |
| `SHAEngine::Process_Block` (MIX digests)         | 48%   |
| SDL window creation plus udev device enumeration | ~18%  |
| memset/memmove (buffer clears)                   | ~11%  |
| `Update_Window_Surface`                          | ~4%   |

The other threads (PulseAudio main loop, the SDL audio and timer threads, hotplug) only wait and
talk to the sound server.

### 6. Audio buffer (new; ~0.19 s off every exit)

Ask for 512 samples, which is 23 ms at 22,050 Hz. The drain drops from 186 ms to 46 ms, and
sound-effect latency drops the same way. The mixing callback is cheap, so underruns are unlikely,
but check playback on the real device. Skipping the close on process exit would also remove the
drain, but it leaves shutdown to the OS; prefer the smaller buffer.

**Done 2026-09-17** (`src/sdllib/ww_audio.cc`, both games): 512 samples. Headless load 0.31 s → 0.16
s wall with the default PulseAudio driver. Also clamped `Fade_Sample`'s step count to at least 1: a
fade shorter than one callback divided by zero, which with 2048-sample buffers already happened for
fades under 6 ticks. RA and TD save/load smoke tests pass. Listening on real speakers is still to
do.

### Step 5 update

The CPU supports SHA-NI (`/proc/cpuinfo`), and SHA-1 is now the largest CPU cost left at about 50
ms. The SHA-NI option in step 5 would cut that to a few ms without changing what gets verified.

## Verification

- `tools/ra_startup_bench.sh` before and after every step. Record the medians here.
- `tools/ra_saveload_smoke.sh` after step 2, to show that skipping fades changes no game state.
- Interactive check on `DISPLAY=:0`: fades still look smooth, ESC still skips the movie, and the
  first-launch intro plays exactly once.
