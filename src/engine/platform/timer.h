/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// File: The game's clocks and the one timer interface of both games.
//
// SystemTicks()        -- real time in 60 Hz ticks.
// SystemMilliseconds() -- real time in milliseconds.
// Stopwatch<T>         -- counts up with stop/start (pause/resume).
// Timer<T>             -- counts down toward zero with stop/start support.
//
// T is a tick source: a class with a static `Tick()` returning a monotonically
// increasing count. SystemTickSource (below) ticks at 60 Hz of real time; each
// game's FrameTickSource, in its game_clock.h, ticks once per game frame.
//
// Stopwatch and Timer serialize by value through Serialize().

#ifndef CNC_RED_ALERT_ENGINE_PLATFORM_TIMER_H_
#define CNC_RED_ALERT_ENGINE_PLATFORM_TIMER_H_

#include <algorithm>
#include <concepts>
#include <cstdint>

// The rate SystemTicks() counts at.
inline constexpr int kSystemTicksPerSecond = 60;

// Returns real time in milliseconds, read from the same monotonic clock as
// SystemTicks(). Like it, the zero point means nothing on its own; 64 bits
// because that clock counts from boot, so 32 bits would wrap after 49 days of
// machine uptime.
int64_t SystemMilliseconds();

// Returns real time in ticks of 1/60 second, read from a monotonic clock that
// needs no setup. The zero point means nothing on its own: callers compare two
// readings to measure how much time has passed.
int64_t SystemTicks();

// A clock the timers below can run on: a static Tick() that never decreases.
// The unit is the source's own (a 60 Hz tick, a game frame); the timers only
// subtract two readings, so they never need to know it.
template <typename T>
concept TickSource = requires {
  { T::Tick() } -> std::same_as<int64_t>;
};

// Ticks at 60 per second of real time, whether or not the game is running.
// For menus, animations and network timeouts.
struct SystemTickSource {
  static int64_t Tick() { return SystemTicks(); }
};

// A count-up timer with stop/start (pause/resume).
//
// When stopped, the timer freezes at its current value. When restarted, it
// resumes counting from where it left off.
template <TickSource T>
class Stopwatch {
 public:
  // Creates a running timer starting at zero elapsed ticks.
  Stopwatch() noexcept : start_tick_(T::Tick()) {}

  // Returns the current elapsed tick count, accounting for paused time.
  [[nodiscard]] int64_t Elapsed() const {
    return running_ ? accumulated_ticks_ + TicksSinceStart()
                    : accumulated_ticks_;
  }

  // Resets the timer to zero and activates it.
  void Reset() {
    start_tick_ = T::Tick();
    accumulated_ticks_ = 0;
    running_ = true;
  }

  // Stops (pauses) the timer. Further ticks do not accumulate until Start().
  void Stop() {
    if (running_) {
      accumulated_ticks_ += TicksSinceStart();
      running_ = false;
    }
  }

  // Starts (resumes) a stopped timer from where it left off.
  void Start() {
    if (!running_) {
      start_tick_ = T::Tick();
      running_ = true;
    }
  }

  // Returns false between Stop() and the next Start() or Reset().
  [[nodiscard]] bool IsRunning() const { return running_; }

  // Saved-game support. Writes the elapsed value and the running flag, not
  // the anchor tick, so the format does not depend on the tick source. On
  // read the stopwatch re-anchors to the current tick, which for a frame
  // tick source means the game clock must already be restored. A negative
  // value fails the archive and leaves the stopwatch unchanged.
  template <class Archive>
  void Serialize(Archive& ar);

 private:
  // Returns ticks elapsed since start_tick_ was last anchored.
  [[nodiscard]] int64_t TicksSinceStart() const {
    return T::Tick() - start_tick_;
  }

  // The tick the current running stretch began at. Only read while running;
  // Start() and Reset() re-anchor it.
  int64_t start_tick_;
  // Ticks accumulated by the stretches that have ended, i.e. up to the last
  // Stop(). Elapsed() adds the current stretch on top while running.
  int64_t accumulated_ticks_{0};
  bool running_{true};
};

template <TickSource T>
template <class Archive>
void Stopwatch<T>::Serialize(Archive& ar) {
  int64_t value = Elapsed();
  bool running = running_;
  ar(value, running);
  if constexpr (Archive::kIsReading) {
    if (value < 0) {
      ar.Fail("negative stopwatch value");
      return;
    }
    accumulated_ticks_ = value;
    start_tick_ = T::Tick();
    running_ = running;
  }
}

// A countdown timer that counts down from a set duration toward zero.
//
// When the value reaches zero the timer is "finished". The timer supports
// stop/start (pause/resume). Calling Set() with a new duration resets and
// restarts the countdown. It is a Stopwatch measured against a duration, so
// pausing works the same way in both.
template <TickSource T>
class Timer {
 public:
  // Starts counting down from `duration` ticks.
  explicit Timer(int64_t duration = 0) noexcept : duration_(duration) {}

  // Returns the ticks remaining, or 0 if the countdown has finished.
  [[nodiscard]] int64_t Remaining() const {
    return std::max<int64_t>(duration_ - watch_.Elapsed(), 0);
  }

  // Resets the countdown to `duration` ticks and restarts it.
  void Set(int64_t duration) {
    duration_ = duration;
    watch_.Reset();
  }

  // Stops (pauses) the countdown.
  void Stop() { watch_.Stop(); }

  // Starts (resumes) a stopped countdown from where it left off.
  void Start() { watch_.Start(); }

  // Returns false between Stop() and the next Start() or Set().
  // A countdown that reaches zero keeps running; it just reads 0.
  [[nodiscard]] bool IsRunning() const { return watch_.IsRunning(); }

  // Returns whether the countdown has reached zero. A stopped timer with time
  // left is not finished.
  [[nodiscard]] bool IsFinished() const { return Remaining() == 0; }

  // Returns the opposite of IsFinished().
  [[nodiscard]] bool HasTimeLeft() const { return Remaining() != 0; }

  // Saved-game support. Writes the remaining value and the running flag and
  // re-anchors on read; see Stopwatch::Serialize for the game clock
  // precondition.
  // A negative value fails the archive and leaves the timer unchanged.
  template <class Archive>
  void Serialize(Archive& ar);

 private:
  // Measures how much of duration_ has run down.
  Stopwatch<T> watch_;
  // The ticks to count down from, as of the last Set() or load. A load
  // stores what was remaining, so it restarts the stopwatch at zero too.
  int64_t duration_;
};

template <TickSource T>
template <class Archive>
void Timer<T>::Serialize(Archive& ar) {
  int64_t value = Remaining();
  bool running = IsRunning();
  ar(value, running);
  if constexpr (Archive::kIsReading) {
    if (value < 0) {
      ar.Fail("negative countdown duration");
      return;
    }
    duration_ = value;
    watch_.Reset();
    if (!running) {
      watch_.Stop();
    }
  }
}

#endif  // CNC_RED_ALERT_ENGINE_PLATFORM_TIMER_H_
