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

// Timer templates parameterized on a tick-source class.
//
// Stopwatch<T> -- counts up with stop/start (pause/resume).
// Timer<T>     -- counts down toward zero with stop/start support.
//
// The tick-source class T must provide `T::Tick() const` returning a
// monotonically increasing tick count (typically int64_t or uint64_t).
//
// These classes are serialized via raw memcpy (see heap.cc), so all members
// must be trivially copyable. Do not use std::optional or other non-trivial
// types as members.

#ifndef CNC_RED_ALERT_TECH_FTIMER_H_
#define CNC_RED_ALERT_TECH_FTIMER_H_

#include <concepts>
#include <cstdint>

#include "tech/noinit.h"

template <typename T>
concept TickSource = requires {
  { T::Tick() } -> std::convertible_to<int64_t>;
};

// A count-up timer with stop/start (pause/resume).
//
// When stopped, the timer freezes at its current value. When restarted, it
// resumes counting from where it left off.
template <TickSource T>
class Stopwatch {
 public:
  // Creates a running timer starting at zero elapsed ticks.
  Stopwatch();

  // No-init constructor for save/load serialization.
  Stopwatch(const NoInitClass&);

  // Returns the current elapsed tick count, accounting for paused time.
  int64_t Value() const;

  // Resets the timer to zero and activates it.
  void Reset();

  // Stops (pauses) the timer. Further ticks do not accumulate until Start().
  void Stop();

  // Starts (resumes) a stopped timer from where it left off.
  void Start();

  bool IsRunning() const;

 private:
  // Returns ticks elapsed since start_tick_ was last anchored.
  int64_t Elapsed() const;

  int64_t start_tick_;
  // Total ticks accumulated across stop/start cycles.
  int64_t accumulated_ticks_;
  bool running_;
};

template <TickSource T>
Stopwatch<T>::Stopwatch()
    : start_tick_(static_cast<int64_t>(T::Tick())),
      accumulated_ticks_(0),
      running_(true) {}

template <TickSource T>
Stopwatch<T>::Stopwatch(const NoInitClass&) {}

template <TickSource T>
int64_t Stopwatch<T>::Value() const {
  int64_t value = accumulated_ticks_;
  if (running_) {
    value += Elapsed();
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
    accumulated_ticks_ += Elapsed();
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
int64_t Stopwatch<T>::Elapsed() const {
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
  // Starts counting down from `set` ticks.
  explicit Timer(int64_t set = 0);

  // No-init constructor for save/load serialization.
  Timer(const NoInitClass&);

  // Returns the ticks remaining, or 0 if the countdown has finished.
  int64_t Value() const;

  // Resets the countdown to `duration` ticks and restarts it.
  void Set(int64_t duration);

  // Restarts the countdown with the same duration.
  void Restart();

  // Stops (pauses) the countdown.
  void Stop();

  // Starts (resumes) a stopped countdown from where it left off.
  void Start();

  bool IsRunning() const;
  bool IsFinished() const;
  bool HasTimeLeft() const;

 private:
  // Returns ticks elapsed since start_tick_ was last anchored.
  int64_t Elapsed() const;

  int64_t start_tick_;
  // Ticks remaining as of the last anchor point (Set, Stop, or construction).
  int64_t delay_time_;
  bool running_;
};

template <TickSource T>
Timer<T>::Timer(int64_t set)
    : start_tick_(static_cast<int64_t>(T::Tick())),
      delay_time_(set),
      running_(true) {}

template <TickSource T>
Timer<T>::Timer(const NoInitClass&) {}

template <TickSource T>
int64_t Timer<T>::Value() const {
  int64_t remain = delay_time_;
  if (running_) {
    int64_t elapsed = Elapsed();
    if (elapsed < remain) {
      return remain - elapsed;
    }
    return 0;
  }
  return remain;
}

template <TickSource T>
void Timer<T>::Set(int64_t duration) {
  start_tick_ = static_cast<int64_t>(T::Tick());
  delay_time_ = duration;
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
    delay_time_ = Value();
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
  return Value() == 0;
}

template <TickSource T>
bool Timer<T>::HasTimeLeft() const {
  return Value() != 0;
}

template <TickSource T>
int64_t Timer<T>::Elapsed() const {
  return static_cast<int64_t>(T::Tick()) - start_tick_;
}

#endif  // CNC_RED_ALERT_TECH_FTIMER_H_
