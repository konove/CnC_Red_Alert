# Plan: one timer interface

Written 2026-09-27.

**Status: complete (2026-09-27).** Step 1 is cb9795d0, step 2 7aea7e85, step 3 51899177, step 4
b584c2eb (RA) and 4fa8a9cb (TD), step 5 8453f575, step 6 50e051b5 and the commit after it. Both
games now time everything with `Stopwatch<T>`/`Timer<T>` from `engine/platform/timer.h`.

**Renamed since (2026-10-11, T-86):** `Timer<T>` is now `Countdown<T>`, so that the two names say
which way each counts. The rest of this plan keeps the name it was written with.

## Context

The tree has three implementations of the same two ideas (count up with pause, count down to zero),
plus the clock they read:

| File                       | Types                                                          | Tick source                                                               | Users                                                               |
| -------------------------- | -------------------------------------------------------------- | ------------------------------------------------------------------------- | ------------------------------------------------------------------- |
| `engine/platform/ftimer.h` | `Stopwatch<T>`, `Timer<T>`                                     | template parameter; `FrameTickSource`/`SystemTickSource` in `ra/jshell.h` | all of RA (~80 declarations), `engine/gfx/glow_pulse.h`             |
| `engine/platform/timer.h`  | `TimerClass`, `CountDownTimerClass`                            | system ticks, hardwired                                                   | TD (~40), RA `stats.cc` `GameTimer`, `winstub.cc`, `nulldlg.cc` (2) |
| `td/ftimer.h`              | `TCountDownTimerClass`                                         | `CurrentFrame()`, hardwired                                               | TD game objects (34 declarations: house, team, foot, building, ...) |
| `engine/platform/timer.h`  | `TickTimer`, `SystemTicks()`, `Get_Time_Ms()`, `TimerSystemOn` | the clock itself                                                          | everyone                                                            |

RA already moved from its copies of the legacy classes to the templates; TD never did, and the
legacy classes stayed in the engine for it.

### How the legacy types differ from the templates

- `TimerClass::Time()` is non-const: it folds elapsed ticks into `Accumulated` on every read, and
  `Started` stores `tick + 1` so that 0 means stopped. `Stopwatch<T>::Value()` is const and keeps
  the anchor separately. Same observable values.
- `CountDownTimerClass` default-constructs stopped at 0, and `Set(v, start)` can leave it stopped.
  `Timer<T>` constructs running. A stopped and a running countdown at 0 both read 0, and no caller
  passes `start = false`, so the difference is unobservable.
- `TCountDownTimerClass` has an inactive state (`Clear()`, `Active()`, `Started == -1`), converts
  implicitly to `int64_t`, is assigned from one (`timer = 30`), and rejects a negative duration on
  load. `Active()`, `Get_Start()` and `Get_Delay()` are read only by `td/savevalues_test.cc`.
- `TCountDownTimerClass` serializes `(remaining, active)`; `Timer<T>` serializes `(value, running)`
  — the same two fields in the same order, so TD saves keep loading.
- `Timer<T>::Value()` returns `delay_time_` unclamped while stopped, but `Stop()` stores the clamped
  running value, so the only way to a negative stopped value is loading one from a save.
  `Stopwatch<T>` loads a negative elapsed value the same way.
- `TimerSystemOn` is read only by the `TimerClass(bool start)` constructor, and nobody passes
  `true`.
- `const CountDownTimerClass delay;` in `td/mplayer.cc` is never read. `send_timer` in `td/stats.cc`
  is `const` but calls `Set()` inside `#ifdef _WIN32`, so that branch does not compile; it loses the
  `const` in the migration.

## Decisions

- `Timer<T>` and `Stopwatch<T>` are the one interface. Rejected: aliasing the legacy names onto the
  templates (keeps two vocabularies with different semantics for `Set` and `Time`), and a runtime
  `Clock&` in place of the template (a pointer in every pooled game object for no caller benefit).
- TD's inactive state maps onto the existing running flag: `Clear()` becomes `Set(0); Stop();` (a
  helper is not added unless the call sites show a need) and `Active()` becomes `IsRunning()`. No
  new state in `Timer<T>`.
- `Get_Time_Ms()` (98 callers, a millisecond wall clock, not a timer) stays as it is; it only moves
  with the clock header.

## Target shape

- `engine/platform/timer.h` (the renamed `ftimer.h`): `TickSource`, `Stopwatch<T>`, `Timer<T>`, and
  `SystemTickSource` (moved from `ra/jshell.h`; it only wraps `SystemTicks()`).
- `engine/platform/tick_clock.{h,cc}` (the rest of today's `timer.{h,cc}`): `TickTimer`,
  `InitTickTimer`, `ShutdownTickTimer`, `SystemTicks`, `Get_Time_Ms`.
- `ra/game_clock.h` and `td/game_clock.h` each define `FrameTickSource` next to `CurrentFrame()`.
- Deleted: `TimerClass`, `CountDownTimerClass`, `TimerSystemOn`, `td/ftimer.h`.

## Steps

Each step builds, passes ctest and the strict checks, and is its own commit.

1. **Harden the templates.** `Timer<T>::Serialize` fails the archive on a negative remaining value
   (ported from `TCountDownTimerClass`), `Stopwatch<T>::Serialize` on a negative elapsed value; both
   leave the object unchanged. Tests in `engine/platform/ftimer_test.cc`: countdown to zero,
   pause/resume for both templates, `Set` while stopped, stopped at zero (the new `Clear()`),
   `Reset`, negative-load rejection. Done.
2. **Move the tick sources.** `SystemTickSource` into `engine/platform/ftimer.h`; `FrameTickSource`
   into `ra/game_clock.h`, and a TD copy into `td/game_clock.h`. `ra/jshell.h` loses both. Done;
   `jshell.h` also dropped its `timer.h` and `game_clock.h` includes, and three `.cc` files that
   included `jshell.h` only for the tick sources no longer do.
3. **TD frame countdowns.** `TCountDownTimerClass` → `Timer<FrameTickSource>` in the 34 declarations
   and their uses, compiler-driven: `x = n` → `x.Set(n)`, implicit reads → `x.Value()`, `Time()` →
   `Value()`, `Expired()` → `IsFinished()`, `Clear()` → `Set(0); Stop();`. Rewrite the
   `savevalues_test.cc` cases against the new API, keeping what they prove (re-anchor on load, wide
   remaining values, cleared vs expired stay distinct, negative duration rejected). Delete
   `td/ftimer.h`. Save/load smoke test: `tools/td_saveload_smoke.sh`. Done: 76 assignments became
   `Set()`, five `Clear()` calls `Set(0); Stop();`, and `MissionClass::Timer` (private) became
   `MissionTimer`, as in RA. Every smoke fixture's state comparison passes; the `--building`,
   `--mobile`, `--globals` and `--map` rejection scripts fail on the pre-change binary too, because
   the messages they look for are `DLOG`ged and `NDEBUG` builds drop them. Those messages are
   `LOG(ERROR)` since the timer work, and the scripts pass.

4. **System-tick timers in both games.** `CountDownTimerClass` → `Timer<SystemTickSource>`,
   `TimerClass` → `Stopwatch<SystemTickSource>` (`Set(0, true)` → `Reset()`, `Time()` → `Value()`).
   Delete `delay` in `td/mplayer.cc`; drop the `const` from `send_timer`. TD members named `Timer`
   (`MouseClass::Timer`, `ScoreClass::Timer`) shadow the template and get renamed (`AnimTimer`, as
   RA did). Done: the `Start()` calls these timers needed after default construction are no-ops on a
   running `Timer<T>` and were dropped, and `send_timer` moved inside its `#ifdef _WIN32`.

5. **Delete the legacy classes.** `TimerClass`, `CountDownTimerClass`, `TimerSystemOn` go from
   `engine/platform/timer.{h,cc}`. Done.
6. **Rename the files.** `timer.{h,cc}` → `tick_clock.{h,cc}`, then `ftimer.h` → `timer.h` and
   `ftimer_test.cc` → `timer_test.cc`; update includers and CMake lists. Both save/load smoke tests,
   and one full strict build. Done: the layer checker's SDL-allowed file is now
   `platform/tick_clock.cc`. All eight TD fixtures and both RA runs (fresh and `--load-fixture`)
   pass.

## Risks

- A TD site that relied on the implicit `int64_t` conversion in arithmetic or a comparison. The
  conversion is gone, so the compiler lists every one.
- A TD site that reads a cleared countdown's value after `Clear()`: both old and new read 0.
- Name lookup: inside classes with a member named `Timer`, `Timer<...>` would not name the template
  until the member is renamed in step 4 (TD frame countdowns in step 3 are in other classes; check
  with the build).
