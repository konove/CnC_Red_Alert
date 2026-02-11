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
// Ticker<T>    -- counts up from a starting value.
// Stopwatch<T> -- adds stop/start (pause/resume) to Ticker.
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

// Concept to ensure the timer source provides a numeric tick or timestamp.
template <typename T>
concept TickSource = requires {
  { T::Tick() } -> std::convertible_to<int64_t>;
};

// A timer that watches a constant-rate tick source T and counts upward.
template <TickSource T>
class Ticker {
 public:
  // Starts the timer with an initial elapsed value of `set` ticks.
  explicit Ticker(int64_t set = 0);

  // No-init constructor for save/load serialization.
  Ticker(const NoInitClass&);

  ~Ticker() = default;
  Ticker(const Ticker&) = default;
  Ticker& operator=(const Ticker&) = default;
  Ticker(Ticker&&) = default;
  Ticker& operator=(Ticker&&) = default;

  // Returns the number of ticks elapsed since the timer was started/reset.
  int64_t Value() const;

  // Resets the timer so that Value() returns `set`.
  Ticker& operator=(int64_t set);

 protected:
  int64_t Started;  // Tick value when the timer was started/reset.
};

template <TickSource T>
Ticker<T>::Ticker(const NoInitClass&) {}

// Anchors Started so that Value() initially returns `set`.
template <TickSource T>
Ticker<T>::Ticker(int64_t set)
    : Started(static_cast<int64_t>(T::Tick()) - set) {}

template <TickSource T>
int64_t Ticker<T>::Value() const {
  return static_cast<int64_t>(T::Tick()) - Started;
}

template <TickSource T>
Ticker<T>& Ticker<T>::operator=(int64_t set) {
  Started = static_cast<int64_t>(T::Tick()) - set;
  return *this;
}

// A timer that extends Ticker with stop/start (pause/resume).
//
// When stopped, the timer freezes at its current value. When restarted, it
// resumes counting from where it left off. If stop/start is not needed, use
// Ticker directly.
template <TickSource T>
class Stopwatch : public Ticker<T> {
 public:
  // Starts the timer with an initial elapsed value of `set` ticks.
  explicit Stopwatch(int64_t set = 0);

  // No-init constructor for save/load serialization.
  Stopwatch(const NoInitClass& x);

  ~Stopwatch() = default;
  Stopwatch(const Stopwatch&) = default;
  Stopwatch& operator=(const Stopwatch&) = default;
  Stopwatch(Stopwatch&&) = default;
  Stopwatch& operator=(Stopwatch&&) = default;

  // Returns the current elapsed tick count, accounting for paused time.
  int64_t Value() const;

  // Resets the timer to count up from `set`, keeping it active.
  Stopwatch& operator=(int64_t set);

  // Stops (pauses) the timer. Further ticks do not accumulate until Start().
  void Stop();

  // Starts (resumes) a stopped timer from where it left off.
  void Start();

  // Returns true if the timer is currently counting (not stopped).
  bool Is_Active() const;

 private:
  int64_t Accumulated;  // Total ticks accumulated across stop/start cycles.
  bool Active;          // True when the timer is running.
};

template <TickSource T>
Stopwatch<T>::Stopwatch(const NoInitClass& x) : Ticker<T>(x) {}

template <TickSource T>
Stopwatch<T>::Stopwatch(int64_t set)
    : Ticker<T>(set), Accumulated(0), Active(true) {}

template <TickSource T>
int64_t Stopwatch<T>::Value() const {
  int64_t value = Accumulated;
  if (Active) {
    value += Ticker<T>::Value();
  }
  return value;
}

template <TickSource T>
Stopwatch<T>& Stopwatch<T>::operator=(int64_t set) {
  this->Started = static_cast<int64_t>(T::Tick());
  Accumulated = set;
  Active = true;
  return *this;
}

template <TickSource T>
void Stopwatch<T>::Stop() {
  if (Active) {
    Accumulated += Ticker<T>::Value();
    Active = false;
  }
}

template <TickSource T>
void Stopwatch<T>::Start() {
  if (!Active) {
    this->Started = static_cast<int64_t>(T::Tick());
    Active = true;
  }
}

template <TickSource T>
bool Stopwatch<T>::Is_Active() const {
  return Active;
}

// A countdown timer that counts down from a set duration toward zero.
//
// When the value reaches zero the timer is "finished". The timer supports
// stop/start (pause/resume). Assigning a new duration via operator= resets
// and restarts the countdown.
template <TickSource T>
class Timer : public Ticker<T> {
 public:
  // Starts counting down from `set` ticks.
  explicit Timer(int64_t set = 0);

  // No-init constructor for save/load serialization.
  Timer(const NoInitClass& x);

  ~Timer() = default;
  Timer(const Timer&) = default;
  Timer& operator=(const Timer&) = default;
  Timer(Timer&&) = default;
  Timer& operator=(Timer&&) = default;

  // Returns the ticks remaining, or 0 if the countdown has finished.
  int64_t Value() const;

  // Resets the countdown to `duration` ticks and restarts it.
  Timer& operator=(int64_t duration);

  // Stops (pauses) the countdown.
  void Stop();

  // Starts (resumes) a stopped countdown from where it left off.
  void Start();

  // Returns true if the countdown is currently running (not stopped).
  bool Is_Active() const;

  // Returns true if the countdown has reached zero.
  bool IsFinished() const;

  // Returns true if the countdown has not yet reached zero.
  bool HasTimeLeft() const;

 private:
  int64_t DelayTime;  // Ticks remaining before countdown timer expires.
  bool Active;        // True when the timer is running.
};

template <TickSource T>
Timer<T>::Timer(const NoInitClass& x) : Ticker<T>(x) {}

template <TickSource T>
Timer<T>::Timer(int64_t set) : Ticker<T>(0), DelayTime(set), Active(true) {}

template <TickSource T>
int64_t Timer<T>::Value() const {
  int64_t remain = DelayTime;
  if (Active) {
    int64_t elapsed = Ticker<T>::Value();
    if (elapsed < remain) {
      return remain - elapsed;
    }
    return 0;
  }
  return remain;
}

template <TickSource T>
Timer<T>& Timer<T>::operator=(int64_t duration) {
  this->Started = static_cast<int64_t>(T::Tick());
  DelayTime = duration;
  Active = true;
  return *this;
}

template <TickSource T>
void Timer<T>::Stop() {
  if (Active) {
    DelayTime = Value();
    Active = false;
  }
}

template <TickSource T>
void Timer<T>::Start() {
  if (!Active) {
    this->Started = static_cast<int64_t>(T::Tick());
    Active = true;
  }
}

template <TickSource T>
bool Timer<T>::Is_Active() const {
  return Active;
}

template <TickSource T>
bool Timer<T>::IsFinished() const {
  return Value() == 0;
}

template <TickSource T>
bool Timer<T>::HasTimeLeft() const {
  return Value() != 0;
}

#endif  // CNC_RED_ALERT_TECH_FTIMER_H_
