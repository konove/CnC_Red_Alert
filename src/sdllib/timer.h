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

#include <SDL_timer.h>

#include <atomic>
#include <cstdint>

extern bool TimerSystemOn;

/*=========================================================================*/
typedef enum BaseTimerEnum {
  BT_SYSTEM,  // System timer (60 / second).
  BT_USER     // User controllable timer (? / second).
} BaseTimerEnum;

class TimerClass {
 public:
  // Constructor.  Timers set before low level init has been done will not
  // be able to be 'Started' or 'on' until timer system is in place.
  TimerClass(BaseTimerEnum timer = BT_SYSTEM, bool start = false);
  ~TimerClass() = default;
  TimerClass(const TimerClass&) = default;
  TimerClass& operator=(const TimerClass&) = default;
  TimerClass(TimerClass&&) = default;
  TimerClass& operator=(TimerClass&&) = default;

  //
  long Set(long value, bool start = true);  // Set initial timer value.
  long Stop();                              // Pause timer.
  long Start();                             // Resume timer.
  long Reset(bool start = true);            // Reset timer to zero.
  long Time();                              // Fetch current timer value.

 protected:
  long Started = 0;      // Time last started (0 == not paused).
  long Accumulated = 0;  //	Total accumulated ticks.

 private:
  BaseTimerEnum TickType;
  long Get_Ticks();
};

inline long TimerClass::Reset(bool start) { return Set(0, start); }

class CountDownTimerClass : private TimerClass {
 public:
  // Constructor.  Timers set before low level init has been done will not
  // be able to be 'Started' or 'on' until timer system is in place.
  CountDownTimerClass(BaseTimerEnum timer, long set, bool on = false);
  CountDownTimerClass(BaseTimerEnum timer = BT_SYSTEM, bool on = false);
  ~CountDownTimerClass() = default;
  CountDownTimerClass(const CountDownTimerClass&) = default;
  CountDownTimerClass& operator=(const CountDownTimerClass&) = default;
  CountDownTimerClass(CountDownTimerClass&&) = default;
  CountDownTimerClass& operator=(CountDownTimerClass&&) = default;

  // Public functions
  long Set(long set, bool start = true);  // Set count down value.
  long Reset(bool start = true);          // Reset timer to zero.
  long Stop();                            // Pause timer.
  long Start();                           // Resume timer.
  long Time();                            // Fetch current count down value.

 protected:
  long DelayTime;  // Ticks remaining before countdown timer expires.
};

inline long CountDownTimerClass::Stop() {
  TimerClass::Stop();
  return Time();
}

inline long CountDownTimerClass::Start() {
  TimerClass::Start();
  return Time();
}

inline long CountDownTimerClass::Reset(bool start) {
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
  int64_t TickCount() const {
    return tick_count_.load(std::memory_order_relaxed);
  }

 private:
  // Handle for SDL timer event.
  SDL_TimerID timer_id_;

  // Tick count, updated from SDL timer thread.
  std::atomic<int64_t> tick_count_{0};
};

uint32_t Get_Time_Ms();

extern TickTimer* g_tick_timer;

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// externs
/////////////////////////////////////////////
#ifdef TD
extern TimerClass TickCount;
#endif

#endif  // CNC_RED_ALERT_SDLLIB_TIMER_H_
