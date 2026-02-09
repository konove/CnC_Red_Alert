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

// Benchmarking utilities built on top of the CPU timestamp counter.

#ifndef CNC_RED_ALERT_TECH_BENCH_H_
#define CNC_RED_ALERT_TECH_BENCH_H_

#include <cstdint>

#include "tech/ftimer.h"
#include "tech/mpu.h"

// Tick source that reads the CPU timestamp counter (TSC).
class PentiumTimerClass {
 public:
  int64_t Tick() const {
    return static_cast<int64_t>(Get_CPU_Clock() >> 4);
  }
};

// Tracks elapsed time with a running average across multiple events.
//
// Typical use is to benchmark a process that occurs many times. By tracking
// an average, inconsistencies in a particular run are smoothed out.
class Benchmark {
 public:
  Benchmark() = default;

  void Begin(bool reset = false);
  void End();

  void Reset();
  int64_t Value() const;
  int64_t Count() const { return TotalCount; }

 private:
  // Maximum number of events in the running average. Older events drop off
  // once this count is reached.
  enum { MAXIMUM_EVENT_COUNT = 256 };

  BasicTimerClass<PentiumTimerClass> Clock;  // Timer for clocking events.
  int64_t Average = 0;     // Total time of all events tracked so far.
  int64_t Counter = 0;     // Number of events tracked so far.
  int64_t TotalCount = 0;  // Absolute total events (may exceed Counter).
};

#endif  // CNC_RED_ALERT_TECH_BENCH_H_
