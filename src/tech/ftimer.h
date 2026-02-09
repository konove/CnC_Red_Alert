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
// BasicTimerClass<T>  -- counts up from a starting value.
// TTimerClass<T>      -- adds stop/start (pause/resume) to BasicTimerClass.
// CDTimerClass<T>     -- counts down toward zero with stop/start support.
//
// The tick-source class T must provide `T::operator()() const` returning a
// monotonically increasing tick count (typically int64_t or uint64_t).
//
// These classes are serialized via raw memcpy (see heap.cc), so all members
// must be trivially copyable. Do not use std::optional or other non-trivial
// types as members.

#ifndef CNC_RED_ALERT_TECH_FTIMER_H_
#define CNC_RED_ALERT_TECH_FTIMER_H_

#include <cstdint>

#include "tech/noinit.h"

// A timer that watches a constant-rate tick source T and counts upward.
//
// The tick source is stored by value, so BasicTimerClass can cascade: an
// instance of BasicTimerClass<T> itself satisfies the tick-source interface,
// allowing layered timer hierarchies.
template <class T>
class BasicTimerClass {
 public:
  // Starts the timer with an initial elapsed value of `set` ticks.
  BasicTimerClass(int64_t set = 0);

  // No-init constructor for save/load serialization.
  BasicTimerClass(const NoInitClass&);

  ~BasicTimerClass() = default;
  BasicTimerClass(const BasicTimerClass&) = default;
  BasicTimerClass& operator=(const BasicTimerClass&) = default;
  BasicTimerClass(BasicTimerClass&&) = default;
  BasicTimerClass& operator=(BasicTimerClass&&) = default;

  // Returns the number of ticks elapsed since the timer was started/reset.
  int64_t Value() const;

  // Allows this timer to serve as a tick source for other timer templates.
  int64_t operator()() const;

 protected:
  T Timer;          // Tick source (ticks at constant rate).
  int64_t Started;  // Tick value when the timer was started/reset.
};

template <class T>
BasicTimerClass<T>::BasicTimerClass(const NoInitClass&) {}

// Anchors Started so that Value() initially returns `set`.
template <class T>
BasicTimerClass<T>::BasicTimerClass(int64_t set)
    : Started(static_cast<int64_t>(Timer()) - set) {}

template <class T>
int64_t BasicTimerClass<T>::Value() const {
  return static_cast<int64_t>(Timer()) - Started;
}

template <class T>
int64_t BasicTimerClass<T>::operator()() const {
  return static_cast<int64_t>(Timer()) - Started;
}

// A timer that extends BasicTimerClass with stop/start (pause/resume).
//
// When stopped, the timer freezes at its current value. When restarted, it
// resumes counting from where it left off. If stop/start is not needed, use
// BasicTimerClass directly.
template <class T>
class TTimerClass : public BasicTimerClass<T> {
 public:
  // Starts the timer with an initial elapsed value of `set` ticks.
  TTimerClass(int64_t set = 0);

  // No-init constructor for save/load serialization.
  TTimerClass(const NoInitClass& x);

  ~TTimerClass() = default;
  TTimerClass(const TTimerClass&) = default;
  TTimerClass& operator=(const TTimerClass&) = default;
  TTimerClass(TTimerClass&&) = default;
  TTimerClass& operator=(TTimerClass&&) = default;

  // Returns the current elapsed tick count, accounting for paused time.
  int64_t Value() const;

  // Allows this timer to serve as a tick source for other timer templates.
  int64_t operator()() const;

  // Resets the timer to count up from `set`, keeping it active.
  TTimerClass& operator=(int64_t set);

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

template <class T>
TTimerClass<T>::TTimerClass(const NoInitClass& x) : BasicTimerClass<T>(x) {}

template <class T>
TTimerClass<T>::TTimerClass(int64_t set)
    : BasicTimerClass<T>(set), Accumulated(0), Active(true) {}

template <class T>
int64_t TTimerClass<T>::Value() const {
  int64_t value = Accumulated;
  if (Active) {
    value += BasicTimerClass<T>::Value();
  }
  return value;
}

template <class T>
int64_t TTimerClass<T>::operator()() const {
  return Value();
}

template <class T>
TTimerClass<T>& TTimerClass<T>::operator=(int64_t set) {
  this->Started = static_cast<int64_t>(this->Timer());
  Accumulated = set;
  Active = true;
  return *this;
}

template <class T>
void TTimerClass<T>::Stop() {
  if (Active) {
    Accumulated += BasicTimerClass<T>::Value();
    Active = false;
  }
}

template <class T>
void TTimerClass<T>::Start() {
  if (!Active) {
    this->Started = static_cast<int64_t>(this->Timer());
    Active = true;
  }
}

template <class T>
bool TTimerClass<T>::Is_Active() const {
  return Active;
}

// A countdown timer that counts down from a set duration toward zero.
//
// When the value reaches zero the timer is "finished". The timer supports
// stop/start (pause/resume). Assigning a new duration via operator= resets
// and restarts the countdown.
template <class T>
class CDTimerClass : public BasicTimerClass<T> {
 public:
  // Starts counting down from `set` ticks.
  CDTimerClass(int64_t set = 0);

  // No-init constructor for save/load serialization.
  CDTimerClass(const NoInitClass& x);

  ~CDTimerClass() = default;
  CDTimerClass(const CDTimerClass&) = default;
  CDTimerClass& operator=(const CDTimerClass&) = default;
  CDTimerClass(CDTimerClass&&) = default;
  CDTimerClass& operator=(CDTimerClass&&) = default;

  // Returns the ticks remaining, or 0 if the countdown has finished.
  int64_t Value() const;

  // Allows this timer to serve as a tick source for other timer templates.
  int64_t operator()() const;

  // Resets the countdown to `duration` ticks and restarts it.
  CDTimerClass& operator=(int64_t duration);

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

template <class T>
CDTimerClass<T>::CDTimerClass(const NoInitClass& x) : BasicTimerClass<T>(x) {}

template <class T>
CDTimerClass<T>::CDTimerClass(int64_t set)
    : BasicTimerClass<T>(0), DelayTime(set), Active(true) {}

template <class T>
int64_t CDTimerClass<T>::Value() const {
  int64_t remain = DelayTime;
  if (Active) {
    int64_t elapsed = BasicTimerClass<T>::Value();
    if (elapsed < remain) {
      return remain - elapsed;
    }
    return 0;
  }
  return remain;
}

template <class T>
int64_t CDTimerClass<T>::operator()() const {
  return Value();
}

template <class T>
CDTimerClass<T>& CDTimerClass<T>::operator=(int64_t duration) {
  this->Started = static_cast<int64_t>(this->Timer());
  DelayTime = duration;
  Active = true;
  return *this;
}

template <class T>
void CDTimerClass<T>::Stop() {
  if (Active) {
    DelayTime = Value();
    Active = false;
  }
}

template <class T>
void CDTimerClass<T>::Start() {
  if (!Active) {
    this->Started = static_cast<int64_t>(this->Timer());
    Active = true;
  }
}

template <class T>
bool CDTimerClass<T>::Is_Active() const {
  return Active;
}

template <class T>
bool CDTimerClass<T>::IsFinished() const {
  return Value() == 0;
}

template <class T>
bool CDTimerClass<T>::HasTimeLeft() const {
  return Value() != 0;
}

#endif  // CNC_RED_ALERT_TECH_FTIMER_H_
