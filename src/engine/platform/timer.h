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
// SystemTicks()  -- real time in 60 Hz ticks.
// SystemMilliseconds()  -- real time in milliseconds.
// Stopwatch<T>   -- counts up with stop/start (pause/resume).
// Timer<T>       -- counts down toward zero with stop/start support.
//
// T is a tick source: a class with a static `Tick()` returning a monotonically
// increasing count. SystemTickSource (below) ticks at 60 Hz of real time; each
// game's FrameTickSource, in its game_clock.h, ticks once per game frame.
//
// Stopwatch and Timer serialize by value through Serialize().

#ifndef CNC_RED_ALERT_ENGINE_PLATFORM_TIMER_H_
#define CNC_RED_ALERT_ENGINE_PLATFORM_TIMER_H_

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
  { T::Tick() } -> std::convertible_to<int64_t>;
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
  Stopwatch() noexcept;

  // Returns the current elapsed tick count, accounting for paused time.
  [[nodiscard]] int64_t Elapsed() const;

  // Resets the timer to zero and activates it.
  void Reset();

  // Stops (pauses) the timer. Further ticks do not accumulate until Start().
  void Stop();

  // Starts (resumes) a stopped timer from where it left off.
  void Start();

  // Returns false between Stop() and the next Start() or Reset().
  [[nodiscard]] bool IsRunning() const;

  // Saved-game support. Writes the elapsed value and the running flag, not
  // the anchor tick, so the format does not depend on the tick source. On
  // read the stopwatch re-anchors to the current tick, which for a frame
  // tick source means the game clock must already be restored. A negative
  // value fails the archive and leaves the stopwatch unchanged.
  template <class Archive>
  void Serialize(Archive& ar);

 private:
  // Returns ticks elapsed since start_tick_ was last anchored.
  [[nodiscard]] int64_t TicksSinceStart() const;

  // The tick the current running stretch began at. Only read while running;
  // Start() and Reset() re-anchor it.
  int64_t start_tick_;
  // Ticks accumulated by the stretches that have ended, i.e. up to the last
  // Stop(). Elapsed() adds the current stretch on top while running.
  int64_t accumulated_ticks_{0};
  bool running_{true};
};

template <TickSource T>
Stopwatch<T>::Stopwatch() noexcept
    : start_tick_(static_cast<int64_t>(T::Tick())) {}

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
    start_tick_ = static_cast<int64_t>(T::Tick());
    running_ = running;
  }
}

template <TickSource T>
int64_t Stopwatch<T>::Elapsed() const {
  int64_t value = accumulated_ticks_;
  if (running_) {
    value += TicksSinceStart();
  }
  return value;
}

template <TickSource T>
void Stopwatch<T>::Reset() {
  start_tick_ = static_cast<int64_t>(T::Tick());
  accumulated_ticks_ = 0;
  running_ = true;
}

template <TickSource T>
void Stopwatch<T>::Stop() {
  if (running_) {
    accumulated_ticks_ += TicksSinceStart();
    running_ = false;
  }
}

template <TickSource T>
void Stopwatch<T>::Start() {
  if (!running_) {
    start_tick_ = static_cast<int64_t>(T::Tick());
    running_ = true;
  }
}

template <TickSource T>
bool Stopwatch<T>::IsRunning() const {
  return running_;
}

template <TickSource T>
int64_t Stopwatch<T>::TicksSinceStart() const {
  return static_cast<int64_t>(T::Tick()) - start_tick_;
}

// A countdown timer that counts down from a set duration toward zero.
//
// When the value reaches zero the timer is "finished". The timer supports
// stop/start (pause/resume). Calling Set() with a new duration resets and
// restarts the countdown.
template <TickSource T>
class Timer {
 public:
  // Starts counting down from `duration` ticks.
  explicit Timer(int64_t duration = 0) noexcept;

  // Returns the ticks remaining, or 0 if the countdown has finished.
  [[nodiscard]] int64_t Remaining() const;

  // Resets the countdown to `duration` ticks and restarts it.
  void Set(int64_t duration);

  // Restarts the countdown from the remaining value it was last given: the
  // duration from construction or Set(), unless a Stop() or a load since
  // then replaced it with what was left at that point.
  void Restart();

  // Stops (pauses) the countdown.
  void Stop();

  // Starts (resumes) a stopped countdown from where it left off.
  void Start();

  // Returns false between Stop() and the next Start(), Set() or Restart().
  // A countdown that reaches zero keeps running; it just reads 0.
  [[nodiscard]] bool IsRunning() const;

  // Returns whether the countdown has reached zero. A stopped timer with time
  // left is not finished.
  [[nodiscard]] bool IsFinished() const;

  // Returns the opposite of IsFinished().
  [[nodiscard]] bool HasTimeLeft() const;

  // Saved-game support. Writes the remaining value and the running flag and
  // re-anchors on read; see Stopwatch::Serialize for the game clock
  // precondition.
  // A negative value fails the archive and leaves the timer unchanged.
  template <class Archive>
  void Serialize(Archive& ar);

 private:
  // Returns ticks elapsed since start_tick_ was last anchored.
  [[nodiscard]] int64_t TicksSinceStart() const;

  // The tick the countdown last started or resumed at. Only read while
  // running.
  int64_t start_tick_;
  // Ticks remaining as of the last anchor point (construction, Set(), Stop()
  // or a load). While running, Remaining() subtracts the ticks since
  // start_tick_.
  int64_t remaining_at_start_;
  bool running_{true};
};

template <TickSource T>
Timer<T>::Timer(int64_t duration) noexcept
    : start_tick_(static_cast<int64_t>(T::Tick())),
      remaining_at_start_(duration) {}

template <TickSource T>
template <class Archive>
void Timer<T>::Serialize(Archive& ar) {
  int64_t value = Remaining();
  bool running = running_;
  ar(value, running);
  if constexpr (Archive::kIsReading) {
    if (value < 0) {
      ar.Fail("negative countdown duration");
      return;
    }
    remaining_at_start_ = value;
    start_tick_ = static_cast<int64_t>(T::Tick());
    running_ = running;
  }
}

template <TickSource T>
int64_t Timer<T>::Remaining() const {
  const int64_t remaining = remaining_at_start_;
  if (running_) {
    const int64_t elapsed = TicksSinceStart();
    if (elapsed < remaining) {
      return remaining - elapsed;
    }
    return 0;
  }
  return remaining;
}

template <TickSource T>
void Timer<T>::Set(int64_t duration) {
  start_tick_ = static_cast<int64_t>(T::Tick());
  remaining_at_start_ = duration;
  running_ = true;
}

template <TickSource T>
void Timer<T>::Restart() {
  start_tick_ = static_cast<int64_t>(T::Tick());
  running_ = true;
}

template <TickSource T>
void Timer<T>::Stop() {
  if (running_) {
    remaining_at_start_ = Remaining();
    running_ = false;
  }
}

template <TickSource T>
void Timer<T>::Start() {
  if (!running_) {
    start_tick_ = static_cast<int64_t>(T::Tick());
    running_ = true;
  }
}

template <TickSource T>
bool Timer<T>::IsRunning() const {
  return running_;
}

template <TickSource T>
bool Timer<T>::IsFinished() const {
  return Remaining() == 0;
}

template <TickSource T>
bool Timer<T>::HasTimeLeft() const {
  return Remaining() != 0;
}

template <TickSource T>
int64_t Timer<T>::TicksSinceStart() const {
  return static_cast<int64_t>(T::Tick()) - start_tick_;
}

#endif  // CNC_RED_ALERT_ENGINE_PLATFORM_TIMER_H_
