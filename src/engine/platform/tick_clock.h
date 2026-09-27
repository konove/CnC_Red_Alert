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

// File: The game's clocks: TickTimer, a 60 Hz tick counter driven by an SDL
// timer, and the millisecond wall clock. The Stopwatch and Timer templates in
// engine/platform/ftimer.h measure time against them.

#ifndef CNC_RED_ALERT_ENGINE_PLATFORM_TICK_CLOCK_H_
#define CNC_RED_ALERT_ENGINE_PLATFORM_TICK_CLOCK_H_

#include <atomic>
#include <cstdint>

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

// Returns the tick timer's count, 60 ticks per second, or 0 before
// InitTickTimer(). The zero point means nothing on its own: callers compare
// two readings to measure how much time has passed.
inline int64_t SystemTicks() {
  return g_tick_timer == nullptr ? 0 : g_tick_timer->TickCount();
}

#endif  // CNC_RED_ALERT_ENGINE_PLATFORM_TICK_CLOCK_H_
