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

/***************************************************************************
 **     C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S       **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Timer Class Functions                    *
 *                                                                         *
 *                    File Name : TIMER.H                                  *
 *                                                                         *
 *                   Programmer : Scott K. Bowen                           *
 *                                                                         *
 *                   Start Date : July 6, 1994                             *
 *                                                                         *
 *                  Last Update : July 12, 1994   [SKB]                    *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef CNC_RED_ALERT_SDLLIB_TIMER_H_
#define CNC_RED_ALERT_SDLLIB_TIMER_H_

#include <atomic>
#include <cstdint>

extern bool TimerSystemOn;

class TimerClass {
 public:
  // Constructor.  Timers set before low level init has been done will not
  // be able to be 'Started' or 'on' until timer system is in place.
  explicit TimerClass(bool start = false) noexcept;
  ~TimerClass() = default;
  TimerClass(const TimerClass&) = default;
  TimerClass& operator=(const TimerClass&) = default;
  TimerClass(TimerClass&&) = default;
  TimerClass& operator=(TimerClass&&) = default;

  // Set initial timer value.
  int64_t Set(int64_t value, bool start = true);
  // Pause timer
  int64_t Stop();
  // Resume timer.
  int64_t Start();
  // Reset timer to zero.
  int64_t Reset(bool start = true) { return Set(0, start); }
  // Fetch current timer value.
  int64_t Time();

 protected:
  int64_t Started = 0;      // Time last started (0 == not paused).
  int64_t Accumulated = 0;  // Total accumulated ticks.

 private:
  static int64_t Get_Ticks();
};

class CountDownTimerClass : TimerClass {
 public:
  // Constructor.  Timers set before low level init has been done will not
  // be able to be 'Started' or 'on' until timer system is in place.
  explicit CountDownTimerClass(int64_t set, bool on = false) noexcept;
  explicit CountDownTimerClass(bool on = false) noexcept;
  ~CountDownTimerClass() = default;
  CountDownTimerClass(const CountDownTimerClass&) = default;
  CountDownTimerClass& operator=(const CountDownTimerClass&) = default;
  CountDownTimerClass(CountDownTimerClass&&) = default;
  CountDownTimerClass& operator=(CountDownTimerClass&&) = default;

  void Set(int64_t value, bool start = true);  // Set count down value.
  int64_t Reset(bool start = true);            // Reset timer to zero.
  int64_t Stop();                              // Pause timer.
  int64_t Start();                             // Resume timer.
  int64_t Time();  // Fetch current count down value.

 protected:
  int64_t DelayTime = 0;  // Ticks remaining before countdown timer expires.
};

inline int64_t CountDownTimerClass::Stop() {
  TimerClass::Stop();
  return Time();
}

inline int64_t CountDownTimerClass::Start() {
  TimerClass::Start();
  return Time();
}

inline int64_t CountDownTimerClass::Reset(bool start) {
  return TimerClass::Reset(start);
}

class TickTimer {
 public:
  explicit TickTimer(int tick_rate = 60);
  ~TickTimer();
  TickTimer(const TickTimer&) = delete;
  TickTimer& operator=(const TickTimer&) = delete;
  TickTimer(TickTimer&&) = delete;
  TickTimer& operator=(TickTimer&&) = delete;

  // Increments the tick counter. Called from the SDL timer callback thread.
  void UpdateTickCount() {
    tick_count_.fetch_add(1, std::memory_order_relaxed);
  }

  // Returns the current tick count.
  [[nodiscard]] int64_t TickCount() const {
    return tick_count_.load(std::memory_order_relaxed);
  }

 private:
  // Handle for SDL timer event.
  int timer_id_;

  // Tick count, updated from SDL timer thread.
  std::atomic<int64_t> tick_count_{0};
};

uint32_t Get_Time_Ms();

extern TickTimer* g_tick_timer;

// Initializes the global tick timer at the given rate. Must be called before
// any code reads g_tick_timer.
void InitTickTimer(int tick_rate = 60);

// Shuts down the global tick timer. Safe to call if never initialized or
// already shut down.
void ShutdownTickTimer();

#endif  // CNC_RED_ALERT_SDLLIB_TIMER_H_
